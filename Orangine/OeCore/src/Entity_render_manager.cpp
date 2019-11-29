#include "pch.h"
#include "Entity_render_manager.h"
#include "Scene_graph_manager.h"
#include "OeCore/Renderer_data.h"
#include "OeCore/Shadow_map_texture.h"
#include "OeCore/Material.h"
#include "OeCore/Entity.h"
#include "OeCore/Scene.h"

#include "OeCore/Light_component.h"
#include "OeCore/Render_light_data.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Entity_sorter.h"
#include "CommonStates.h"
#include "GeometricPrimitive.h"
#include "OeCore/Render_pass.h"

#include <set>
#include <functional>
#include <optional>
#include "OeCore/IMaterial_manager.h"
#include "OeCore/Morph_weights_component.h"
#include "OeCore/Skinned_mesh_component.h"
#include "OeCore/Mesh_utils.h"

using namespace oe;
using namespace internal;

using namespace DirectX;
using namespace std::literals;

using namespace std;

std::string Entity_render_manager::_name = "Entity_render_manager";

Renderer_animation_data g_emptyRenderableAnimationData = []() {
    auto rad = Renderer_animation_data();
    std::fill(rad.morphWeights.begin(), rad.morphWeights.end(), 0.0f);
    return rad;
}();

template<>
IEntity_render_manager* oe::create_manager(Scene & scene, std::shared_ptr<IMaterial_repository>& materialRepository)
{
    return new Entity_render_manager(scene, materialRepository);
}

inline DX::DeviceResources& Entity_render_manager::deviceResources() const
{
	return _scene.manager<ID3D_resources_manager>().deviceResources();
}

Entity_render_manager::Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository)
	: IEntity_render_manager(scene)
	, _materialRepository(std::move(materialRepository))
{
}

void Entity_render_manager::initialize()
{
}

void Entity_render_manager::shutdown()
{
}

const std::string& Entity_render_manager::name() const
{
    return _name;
}

void Entity_render_manager::tick()
{
	const auto& environmentMap = _scene.skyboxTexture();
	if (environmentMap.get() != _environmentIbl.skyboxTexture.get()) {
		const auto skyboxFileTexture = dynamic_cast<File_texture*>(environmentMap.get());
		if (!skyboxFileTexture)
			throw std::runtime_error("Skybox texture isn't a file texture!");

		const auto& skyboxFileTextureFilename = skyboxFileTexture->filename();
		if (skyboxFileTextureFilename.find_last_of(L".dds") != skyboxFileTextureFilename.length() - 1) {
			throw std::runtime_error("Skybox texture must be a .dds file");
		}

		const auto filenamePrefix = skyboxFileTextureFilename.substr(0, skyboxFileTextureFilename.length() - 4);

		_environmentIbl.skyboxTexture = environmentMap;
		_environmentIbl.iblBrdfTexture = std::make_shared<File_texture>(filenamePrefix + L"Brdf.dds");
		_environmentIbl.iblDiffuseTexture = std::make_shared<File_texture>(filenamePrefix + L"DiffuseHDR.dds");
		_environmentIbl.iblSpecularTexture = std::make_shared<File_texture>(filenamePrefix + L"SpecularHDR.dds");

		if (_renderLightData_lit) {
			_renderLightData_lit->setEnvironmentIblMap(nullptr, nullptr, nullptr);
		}
	}

	if (_renderLightData_lit &&
		_environmentIbl.iblDiffuseTexture &&
		!_renderLightData_lit->environmentMapDiffuse()) 
	{
		_environmentIbl.iblBrdfTexture->load(deviceResources().GetD3DDevice());
		_environmentIbl.iblDiffuseTexture->load(deviceResources().GetD3DDevice());
		_environmentIbl.iblSpecularTexture->load(deviceResources().GetD3DDevice());

		_renderLightData_lit->setEnvironmentIblMap(
			_environmentIbl.iblBrdfTexture,
			_environmentIbl.iblDiffuseTexture,
			_environmentIbl.iblSpecularTexture);
	}
}

void Entity_render_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{	
	auto device = deviceResources().GetD3DDevice();

	// Material specific rendering properties
	_renderLightData_unlit = std::make_unique<decltype(_renderLightData_unlit)::element_type>(device);
	_renderLightData_lit = std::make_unique<decltype(_renderLightData_lit)::element_type>(device);
}

void Entity_render_manager::destroyDeviceDependentResources()
{
	_renderLightData_unlit.reset();
	_renderLightData_lit.reset();
}

void Entity_render_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND /*window*/, int width, int height)
{
}

void Entity_render_manager::destroyWindowSizeDependentResources()
{
}

template<uint8_t TMax_lights>
bool addLightToRenderLightData(const Entity& lightEntity, Render_light_data_impl<TMax_lights>& renderLightData)
{
	const auto directionalLight = lightEntity.getFirstComponentOfType<Directional_light_component>();
	if (directionalLight)
	{
		const auto lightDirection = SimpleMath::Vector3::Transform(SimpleMath::Vector3::Forward, lightEntity.worldRotation());
		const auto shadowData = dynamic_cast<Shadow_map_texture_array_slice*>(directionalLight->shadowData().get());

		if (shadowData != nullptr) {
			auto shadowMapDepth = shadowData->casterVolume().Extents.z;
			auto shadowMapBias = directionalLight->shadowMapBias();
			return renderLightData.addDirectionalLight(lightDirection, 
				directionalLight->color(),
				directionalLight->intensity(), 
				*shadowData,
				shadowMapDepth,
				shadowMapBias);
		} else if (directionalLight->shadowData().get() != nullptr) {
			throw std::logic_error("Directional lights only support texture array shadow maps.");
		}
		return renderLightData.addDirectionalLight(lightDirection, directionalLight->color(), directionalLight->intensity());
	}

	const auto pointLight = lightEntity.getFirstComponentOfType<Point_light_component>();
	if (pointLight)
		return renderLightData.addPointLight(lightEntity.worldPosition(), pointLight->color(), pointLight->intensity());

	const auto ambientLight = lightEntity.getFirstComponentOfType<Ambient_light_component>();
	if (ambientLight)
		return renderLightData.addAmbientLight(ambientLight->color(), ambientLight->intensity());
	return false;
}

BoundingFrustumRH Entity_render_manager::createFrustum(const Camera_component& cameraComponent)
{
	const auto viewport = deviceResources().GetScreenViewport();
	const auto aspectRatio = viewport.Width / viewport.Height;

	const auto projMatrix = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		cameraComponent.fov(),
		aspectRatio,
		cameraComponent.nearPlane(),
		cameraComponent.farPlane());
	auto frustum = BoundingFrustumRH(projMatrix);
	const auto& entity = cameraComponent.entity();
	frustum.Origin = entity.worldPosition();
	frustum.Orientation = entity.worldRotation();

	return frustum;
}

void createMissingVertexAttributes(std::shared_ptr<Mesh_data> meshData, 
    const std::vector<Vertex_attribute_element>& requiredAttributes,
    const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes)
{
    // TODO: Fix the normal/binormal/tangent loading code.
    auto generateNormals = false;
    auto generateTangents = false;
    auto generateBiTangents = false;

    auto layout = meshData->vertexLayout.vertexLayout();
    for (const auto& requiredAttributeElement : requiredAttributes) {
        const auto requiredAttribute = requiredAttributeElement.semantic;
        if (std::any_of(layout.begin(), layout.end(), [requiredAttribute](const auto& vae) { return vae.semantic == requiredAttribute; }))
            continue;

        if (std::find(vertexMorphAttributes.begin(), vertexMorphAttributes.end(), requiredAttribute) != vertexMorphAttributes.end())
            continue;

        // We only support generation of normals, tangents, bi-tangent
        uint32_t numComponents = 3;
        if (requiredAttribute == Vertex_attribute_semantic{ Vertex_attribute::Normal, 0 })
            generateNormals = true;
        else if (requiredAttribute == Vertex_attribute_semantic{ Vertex_attribute::Tangent, 0 }) {
            numComponents = 4;
            generateTangents = true;
        }
        else if (requiredAttribute == Vertex_attribute_semantic{ Vertex_attribute::Bi_Tangent, 0 })
            generateBiTangents = true;
        else {
            throw std::logic_error("Mesh does not have required attribute: "s.append(
                Vertex_attribute_meta::vsInputName(requiredAttribute))
            );
        }

        // Create the missing accessor
        const auto vertexCount = meshData->getVertexCount();
        const auto elementStride = sizeof(float) * numComponents;
        meshData->vertexBufferAccessors[requiredAttribute] = make_unique<Mesh_vertex_buffer_accessor>(
            make_shared<Mesh_buffer>(elementStride * vertexCount),
            Vertex_attribute_element { 
                requiredAttribute, 
                numComponents == 3 ? Element_type::Vector3 : Element_type::Vector4, 
                Element_component::Float 
            },
            static_cast<uint32_t>(vertexCount),
            static_cast<uint32_t>(elementStride),
            0
            );
    }

    if (generateNormals)
    {
        LOG(WARNING) << "Generating missing normals";
        Primitive_mesh_data_factory::generateNormals(
            *meshData->indexBufferAccessor,
            *meshData->vertexBufferAccessors[{Vertex_attribute::Position, 0}],
            *meshData->vertexBufferAccessors[{Vertex_attribute::Normal, 0}]);
    }

    if (generateTangents || generateBiTangents)
    {
        LOG(WARNING) << "Generating missing Tangents and/or Bi-tangents";
        Primitive_mesh_data_factory::generateTangents(meshData);
    }
}

void Entity_render_manager::renderEntity(Renderable_component& renderableComponent,
	const Render_pass::Camera_data& cameraData,
	const Light_provider::Callback_type& lightDataProvider,
	const Render_pass_blend_mode blendMode)
{
	if (!renderableComponent.visible())
		return;

	const auto& entity = renderableComponent.entity();
    
	try {
		const auto material = renderableComponent.material();
        if (!material) {
            throw std::runtime_error("Missing material on entity");
        }

        const auto meshDataComponent = entity.getFirstComponentOfType<Mesh_data_component>();
        if (meshDataComponent == nullptr || meshDataComponent->meshData() == nullptr)
        {
            // There is no mesh data (it may still be loading!), we can't render.
            return;
        }

        const auto& meshData = meshDataComponent->meshData();

		auto rendererData = renderableComponent.rendererData().get();
		if (rendererData == nullptr) {
			LOG(INFO) << "Creating renderer data for entity " << entity.getName() << " (ID " << entity.getId() << ")";

            // Note we get the flags for the case where all features are enabled, to make sure we load all the data streams.
            const auto flags = material->configFlags(Renderer_features_enabled(), blendMode, meshDataComponent->meshData()->vertexLayout);
			const auto vertexInputs = material->vertexInputs(flags);
            const auto vertexSettings = material->vertexShaderSettings(flags);
            
			auto rendererDataPtr = createRendererData(meshData, vertexInputs, vertexSettings.morphAttributes);
			rendererData = rendererDataPtr.get();
			renderableComponent.setRendererData(std::move(rendererDataPtr));
            renderableComponent.setMaterialContext(std::make_unique<Material_context>());
		}

		const auto lightMode = material->lightMode();
		Render_light_data* renderLightData;
		if (Material_light_mode::Lit == lightMode) {
			renderLightData = _renderLightData_lit.get();

			// Ask the caller what lights are affecting this entity.
			_renderLights.clear();
			_renderLightData_lit->clear();
			lightDataProvider(entity.boundSphere(), _renderLights, _renderLightData_lit->maxLights());
			if (_renderLights.size() > _renderLightData_lit->maxLights()) {
				throw std::logic_error("Light_provider::Callback_type added too many lights to entity");
			}

			for (auto& lightEntity : _renderLights) {
				if (!addLightToRenderLightData(*lightEntity, *_renderLightData_lit)) {
					throw std::logic_error("Failed to add light to light data");
				}
			}

			_renderLightData_lit->updateBuffer(deviceResources().GetD3DDeviceContext());
		}
		else {
			renderLightData = _renderLightData_unlit.get();
		}

        // Morphed animation?
        const auto morphWeightsComponent = entity.getFirstComponentOfType<Morph_weights_component>();
        if (morphWeightsComponent != nullptr) {
            std::transform(morphWeightsComponent->morphWeights().begin(),
                morphWeightsComponent->morphWeights().end(),
                _rendererAnimationData.morphWeights.begin(),
                [](double v) {return static_cast<float>(v); }
            );
            std::fill(_rendererAnimationData.morphWeights.begin() + morphWeightsComponent->morphWeights().size(), _rendererAnimationData.morphWeights.end(), 0.0f);
        }

        // Skinned mesh?
        const auto skinnedMeshComponent = entity.getFirstComponentOfType<Skinned_mesh_component>();
        //Matrix const* worldTransform;
        const SimpleMath::Matrix* worldTransform;
        const auto skinningEnabled = _scene.manager<IMaterial_manager>().rendererFeatureEnabled().skinnedAnimation;
        if (skinningEnabled && skinnedMeshComponent != nullptr) {

            // Don't need the transform root here, just read the world transforms.
            const auto& joints = skinnedMeshComponent->joints();
            const auto& inverseBindMatrices = skinnedMeshComponent->inverseBindMatrices();
            if (joints.size() != inverseBindMatrices.size())
                throw std::runtime_error("Size of joints and inverse bone transform arrays must match.");

            if (inverseBindMatrices.size() > _rendererAnimationData.boneTransformConstants.size()) {
                throw std::runtime_error("Maximum number of bone transforms exceeded: " +
                    std::to_string(inverseBindMatrices.size()) + " > " +
                    std::to_string(_rendererAnimationData.boneTransformConstants.size()));
            }

            if (skinnedMeshComponent->skeletonTransformRoot())
                worldTransform = &skinnedMeshComponent->skeletonTransformRoot()->worldTransform();
            else
                worldTransform = &entity.worldTransform();
                
			SimpleMath::Matrix invWorld2;
            worldTransform->Invert(invWorld2);
			auto invWorld = toVectorMathMat4(invWorld2);

            for (size_t i = 0; i < joints.size(); ++i) {
                const auto joint = joints[i];
				const auto jointWorldTransform = toVectorMathMat4(joint->worldTransform());
                auto jointToRoot = invWorld * jointWorldTransform;
                const auto inverseBoneTransform = inverseBindMatrices[i];

                _rendererAnimationData.boneTransformConstants[i] = jointToRoot * inverseBoneTransform;

            }
            _rendererAnimationData.numBoneTransforms = static_cast<uint32_t>(joints.size());
        }
        else {
            _rendererAnimationData.numBoneTransforms = 0;
            worldTransform = &entity.worldTransform();
        }
        
		drawRendererData(
			cameraData,
            *worldTransform,
			*rendererData,
			blendMode,
			*renderLightData,
			material,
            meshData->vertexLayout,
            *renderableComponent.materialContext(),
            _rendererAnimationData,
			renderableComponent.wireframe());
        
	}
	catch (std::runtime_error& e)
	{
		renderableComponent.setVisible(false);
		LOG(WARNING) << "Failed to render mesh on entity " << entity.getName() << " (ID " << entity.getId() << "): " << e.what();
	}
}

void Entity_render_manager::renderRenderable(Renderable& renderable,
	const SimpleMath::Matrix& worldMatrix,
	float radius,
	const Render_pass::Camera_data& cameraData,
	const Light_provider::Callback_type& lightDataProvider,
	Render_pass_blend_mode blendMode,
	bool wireFrame)
{
	auto material = renderable.material;

	if (renderable.rendererData == nullptr) {
		if (renderable.meshData == nullptr) {
			// There is no mesh data (it may still be loading!), we can't render.
			return;
		}

		LOG(INFO) << "Creating renderer data for renderable";

        // Note we get the flags for the case where all features are enabled, to make sure we load all the data streams.
        const auto flags = material->configFlags(Renderer_features_enabled(), blendMode, renderable.meshData->vertexLayout);
        const auto vertexInputs = material->vertexInputs(flags);
        const auto vertexSettings = material->vertexShaderSettings(flags);

		auto rendererData = createRendererData(renderable.meshData, vertexInputs, vertexSettings.morphAttributes);
		renderable.rendererData = move(rendererData);
	}
    if (renderable.materialContext == nullptr) {
        renderable.materialContext = std::make_unique<Material_context>();
    }

	const auto lightMode = material->lightMode();
	Render_light_data* renderLightData;
	if (Material_light_mode::Lit == lightMode) {
		renderLightData = _renderLightData_lit.get();

		// Ask the caller what lights are affecting this entity.
		_renderLights.clear();
		_renderLightData_lit->clear();
		BoundingSphere lightTarget;
		lightTarget.Center = worldMatrix.Translation();
		lightTarget.Radius = radius;
		lightDataProvider(lightTarget, _renderLights, _renderLightData_lit->maxLights());
		if (_renderLights.size() > _renderLightData_lit->maxLights()) {
			throw std::logic_error("Light_provider::Callback_type added too many lights to entity");
		}

		for (auto& lightEntity : _renderLights) {
			if (!addLightToRenderLightData(*lightEntity, *_renderLightData_lit)) {
				throw std::logic_error("Failed to add light to light data");
			}
		}

		_renderLightData_lit->updateBuffer(deviceResources().GetD3DDeviceContext());
	}
	else {
		renderLightData = _renderLightData_unlit.get();
	}

    auto rendererAnimationData = renderable.rendererAnimationData.get();
    if (rendererAnimationData == nullptr)
        rendererAnimationData = &g_emptyRenderableAnimationData;

	drawRendererData(
		cameraData,
		worldMatrix,
		*renderable.rendererData,
		blendMode,
		*renderLightData,
		material,
        renderable.meshData->vertexLayout,
        *renderable.materialContext,
        *rendererAnimationData,
		wireFrame);
}

void Entity_render_manager::loadRendererDataToDeviceContext(const Renderer_data& rendererData, const Material_context& context)
{
	const auto deviceContext = deviceResources().GetD3DDeviceContext();
    
	// Send the buffers to the input assembler in the order that the material context requires.
	if (!rendererData.vertexBuffers.empty()) {
		_bufferArraySet.bufferArray.clear();
		_bufferArraySet.strideArray.clear();
		_bufferArraySet.offsetArray.clear();

        for (const auto vsInput : context.compiledMaterial->vsInputs) {
            const auto accessorPos = rendererData.vertexBuffers.find(vsInput.semantic);
            assert(accessorPos != rendererData.vertexBuffers.end());
            const auto& accessor = accessorPos->second;
            _bufferArraySet.bufferArray.push_back(accessor->buffer->d3dBuffer);
            _bufferArraySet.strideArray.push_back(accessor->stride);
            _bufferArraySet.offsetArray.push_back(accessor->offset);
        }

		deviceContext->IASetVertexBuffers(0, 
            static_cast<UINT>(_bufferArraySet.bufferArray.size()), 
            _bufferArraySet.bufferArray.data(), 
            _bufferArraySet.strideArray.data(), 
            _bufferArraySet.offsetArray.data()
        );

		// Set the type of primitive that should be rendered from this vertex buffer.
		deviceContext->IASetPrimitiveTopology(rendererData.topology);

		if (rendererData.indexBufferAccessor != nullptr) {
			// Set the index buffer to active in the input assembler so it can be rendered.
			const auto indexAccessor = rendererData.indexBufferAccessor.get();
			deviceContext->IASetIndexBuffer(indexAccessor->buffer->d3dBuffer, rendererData.indexFormat, indexAccessor->offset);
		}
	}
}

void Entity_render_manager::drawRendererData(
	const Render_pass::Camera_data& cameraData,
	const SimpleMath::Matrix& worldTransform,
	Renderer_data& rendererData,
	Render_pass_blend_mode blendMode,
	const Render_light_data& renderLightData,
	std::shared_ptr<Material> material,
    const Mesh_vertex_layout& meshVertexLayout,
    Material_context& materialContext,
    Renderer_animation_data& rendererAnimationData,
	bool wireFrame)
{
	if (rendererData.failedRendering || rendererData.vertexBuffers.empty())
		return;

	auto& commonStates = _scene.manager<ID3D_resources_manager>().commonStates();

	auto& d3DDeviceResources = deviceResources();
	const auto context = d3DDeviceResources.GetD3DDeviceContext();

	// Set the rasteriser state
	if (wireFrame)
		context->RSSetState(commonStates.Wireframe());
	else if (material->faceCullMode() == Material_face_cull_mode::Back_Face)
		context->RSSetState(commonStates.CullClockwise());
	else if (material->faceCullMode() == Material_face_cull_mode::Front_Face)
		context->RSSetState(commonStates.CullCounterClockwise());
	else if (material->faceCullMode() == Material_face_cull_mode::None)
		context->RSSetState(commonStates.CullNone());

	// Render the triangles
    auto& materialManager = _scene.manager<IMaterial_manager>();
	try {

        // Make sure that material has is up to date, so that materialManager can simply call
        // `ensureCompilerPropertiesHash` instead of non-const `calculateCompilerPropertiesHash`
        material->calculateCompilerPropertiesHash();

        // What accessor types does this rendererData have?
        materialManager.bind(
            materialContext,
            material,
            meshVertexLayout,
            renderLightData,
            blendMode,
            cameraData.enablePixelShader
        );

        loadRendererDataToDeviceContext(rendererData, materialContext);
        
        materialManager.render(rendererData, worldTransform, rendererAnimationData, cameraData);

        materialManager.unbind();

	} catch (std::exception& ex) {
		rendererData.failedRendering = true;
		LOG(WARNING) << "Failed to drawRendererData, marking failedRendering to true. (" << ex.what() << ")";
	}
}

std::unique_ptr<Renderer_data> Entity_render_manager::createRendererData(std::shared_ptr<Mesh_data> meshData, 
    const std::vector<Vertex_attribute_element>& vertexAttributes,
    const std::vector<Vertex_attribute_semantic>& vertexMorphAttributes) const
{
	auto rendererData = std::make_unique<Renderer_data>();

	switch (meshData->m_meshIndexType) {
	case Mesh_index_type::Triangles:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case Mesh_index_type::Lines:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		throw std::exception("Unsupported mesh topology");
	}

    createMissingVertexAttributes(meshData, vertexAttributes, vertexMorphAttributes);

	// Create D3D Index Buffer
	if (meshData->indexBufferAccessor) {
		rendererData->indexCount = meshData->indexBufferAccessor->count;

		rendererData->indexBufferAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData("Mesh data index buffer", *meshData->indexBufferAccessor->buffer, D3D11_BIND_INDEX_BUFFER),
			meshData->indexBufferAccessor->stride,
			meshData->indexBufferAccessor->offset);

        rendererData->indexFormat = mesh_utils::getDxgiFormat(Element_type::Scalar, meshData->indexBufferAccessor->component);

	    const auto name("Index Buffer (count: " + std::to_string(rendererData->indexCount) + ")");
		rendererData->indexBufferAccessor->buffer->d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else 
	{
		// TODO: Simply log a warning, or try to draw a non-indexed mesh
		rendererData->indexCount = 0;
		throw std::runtime_error("CreateRendererData: Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->vertexCount = meshData->getVertexCount();
	for (auto vertexAttrElement : vertexAttributes)
	{
        const auto& vertexAttr = vertexAttrElement.semantic;
        Mesh_vertex_buffer_accessor* meshAccessor = nullptr;
		auto vbAccessorPos = meshData->vertexBufferAccessors.find(vertexAttr);
        if (vbAccessorPos != meshData->vertexBufferAccessors.end()) {
            meshAccessor = vbAccessorPos->second.get();
        }
        else {
            // Since there is no guarantee that the order is the same in the requested attributes array, and the mesh data,
            // we need to do a search to find out the morph target index of this attribute/semantic.
            auto vertexMorphAttributesIdx = -1;
            auto morphTargetIdx = -1;
            for (size_t idx = 0; idx < vertexMorphAttributes.size(); ++idx) {
                const auto& vertexMorphAttribute = vertexMorphAttributes.at(idx);
                if (vertexMorphAttribute.attribute == vertexAttr.attribute) {
                    ++morphTargetIdx;

                    if (vertexMorphAttribute.semanticIndex == vertexAttr.semanticIndex) {
                        vertexMorphAttributesIdx = static_cast<int>(idx);
                        break;
                    }
                }
            }
            if (vertexMorphAttributesIdx == -1) {
                throw std::runtime_error("Could not find morph attribute in vertexMorphAttributes");
            }

            const size_t morphTargetLayoutSize = meshData->vertexLayout.morphTargetLayout().size();
            // What position in the mesh morph layout is this type? This will correspond with its position in the buffer accessors array
            auto morphLayoutOffset = -1;
            for (size_t idx = 0; idx < morphTargetLayoutSize; ++idx) {
                if (meshData->vertexLayout.morphTargetLayout()[idx].attribute == vertexAttr.attribute) {
                    morphLayoutOffset = static_cast<int>(idx);
                    break;
                }
            }
            if (morphLayoutOffset == -1) {
                throw std::runtime_error("Could not find morph attribute in mesh morph target layout");
            }
            
            meshAccessor = meshData->attributeMorphBufferAccessors.at(morphTargetIdx).at(morphLayoutOffset).get();
        }

        if (meshAccessor == nullptr) {
            throw std::runtime_error("CreateRendererData: Missing vertex attribute: "s.append(
                Vertex_attribute_meta::vsInputName(vertexAttr))
            );
        }

		auto d3DAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData("Mesh data vertex buffer", *meshAccessor->buffer, D3D11_BIND_VERTEX_BUFFER),
			meshAccessor->stride, 
			meshAccessor->offset);

        if (rendererData->vertexBuffers.find(vertexAttr) != rendererData->vertexBuffers.end()) {
            throw std::runtime_error("Mesh data contains vertex attribute "s + 
                Vertex_attribute_meta::vsInputName(vertexAttr) + 
                " more than once.");
        }
		rendererData->vertexBuffers[vertexAttr] = std::move(d3DAccessor);
	}

	return rendererData;
}

Renderable Entity_render_manager::createScreenSpaceQuad(std::shared_ptr<Material> material) const
{
	auto renderable = Renderable();
	if (renderable.meshData == nullptr)
		renderable.meshData = Primitive_mesh_data_factory::createQuad({ 2.0f, 2.0f }, { -1.f, -1.f, 0.f });

	if (renderable.material == nullptr)
		renderable.material = material;

	if (renderable.rendererData == nullptr)
	{
        // Note we get the flags for the case where all features are enabled, to make sure we load all the data streams.
        const auto flags = material->configFlags(Renderer_features_enabled(), Render_pass_blend_mode::Opaque, renderable.meshData->vertexLayout);
		std::vector<Vertex_attribute> vertexAttributes;
		const auto vertexInputs = renderable.material->vertexInputs(flags);
        const auto vertexSettings = material->vertexShaderSettings(flags);
		renderable.rendererData = createRendererData(renderable.meshData, vertexInputs, vertexSettings.morphAttributes);
	}

	return renderable;
}

void Entity_render_manager::clearRenderStats()
{
	_renderStats = {};
}


std::shared_ptr<D3D_buffer> Entity_render_manager::createBufferFromData(const std::string& bufferName, const Mesh_buffer& buffer, UINT bindFlags) const
{
    return createBufferFromData(bufferName, buffer.data, buffer.dataSize, bindFlags);
}

std::shared_ptr<D3D_buffer> Entity_render_manager::createBufferFromData(const std::string& bufferName, const uint8_t* data, size_t dataSize, UINT bindFlags) const
{
	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = static_cast<UINT>(dataSize);
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the sub-resource data.
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = data;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

    // Create the buffer
    ID3D11Buffer* d3dBuffer;
    ThrowIfFailed(deviceResources().GetD3DDevice()->CreateBuffer(&bufferDesc, &initData, &d3dBuffer));
    
    DX::ThrowIfFailed(d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(bufferName.size()), bufferName.c_str()));

	return std::make_shared<D3D_buffer>(d3dBuffer, dataSize);
}
