#include "pch.h"
#include "Entity_render_manager.h"
#include "Scene_graph_manager.h"
#include "Renderer_data.h"
#include "Shadow_map_texture.h"
#include "Material.h"
#include "Entity.h"
#include "Scene.h"

#include "Light_component.h"
#include "Render_light_data.h"
#include "Renderable_component.h"
#include "Camera_component.h"
#include "Entity_sorter.h"
#include "CommonStates.h"
#include "GeometricPrimitive.h"

#include <set>
#include <array>
#include <functional>
#include <optional>
#include "Render_pass.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;
using namespace std::literals;

using namespace std;

inline DX::DeviceResources& Entity_render_manager::deviceResources() const
{
	return _scene.manager<ID3D_resources_manager>().deviceResources();
}

Entity_render_manager::Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository)
	: IEntity_render_manager(scene)
	, _materialRepository(std::move(materialRepository))
	, _renderLights({})
	, _renderLightData_unlit(nullptr)
	, _renderLightData_lit(nullptr)
{
}

void Entity_render_manager::initialize()
{
}

void Entity_render_manager::shutdown()
{
}

void Entity_render_manager::createDeviceDependentResources(DX::DeviceResources& /*deviceResources*/)
{	
	auto device = deviceResources().GetD3DDevice();

	// Material specific rendering properties
	_renderLightData_unlit = std::make_unique<decltype(_renderLightData_unlit)::element_type>(device);
	_renderLightData_lit = std::make_unique<decltype(_renderLightData_lit)::element_type>(device);
}

void Entity_render_manager::createWindowSizeDependentResources(DX::DeviceResources& /*deviceResources*/, HWND /*window*/, int width, int height)
{
}

void Entity_render_manager::destroyDeviceDependentResources()
{
	_renderLightData_unlit.reset();
	_renderLightData_lit.reset();
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
		const auto lightDirection = Vector3::Transform(Vector3::Forward, lightEntity.worldRotation());
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

	const auto projMatrix = Matrix::CreatePerspectiveFieldOfView(
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

void Entity_render_manager::renderEntity(Renderable_component& renderable,
	const Render_pass::Camera_data& cameraData,
	const Light_provider::Callback_type& lightDataProvider,
	const Render_pass_blend_mode blendMode)
{
	if (!renderable.visible())
		return;

	const auto& entity = renderable.entity();

	try {
		const auto material = renderable.material().get();
		assert(material != nullptr);
	
		const Renderer_data* rendererData = renderable.rendererData().get();
		if (rendererData == nullptr) {

			const auto meshDataComponent = entity.getFirstComponentOfType<Mesh_data_component>();
			if (meshDataComponent == nullptr || meshDataComponent->meshData() == nullptr)
			{
				// There is no mesh data (it may still be loading!), we can't render.
				return;
			}
			const auto& meshData = meshDataComponent->meshData();

			LOG(INFO) << "Creating renderer data for entity " << entity.getName() << " (ID " << entity.getId() << ")";

			std::vector<Vertex_attribute> vertexAttributes;
			material->vertexAttributes(vertexAttributes);

			auto rendererDataPtr = createRendererData(*meshData, vertexAttributes);
			rendererData = rendererDataPtr.get();
			renderable.setRendererData(std::move(rendererDataPtr));
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

		drawRendererData(
			cameraData,
			entity.worldTransform(),
			*rendererData,
			blendMode,
			*renderLightData,
			*material,
			renderable.wireframe());
	}
	catch (std::runtime_error& e)
	{
		renderable.setVisible(false);
		LOG(WARNING) << "Failed to render mesh on entity " << entity.getName() << " (ID " << entity.getId() << "): " << e.what();
	}
}

void Entity_render_manager::renderRenderable(Renderable& renderable,
	const Matrix& worldMatrix,
	float radius,
	const Render_pass::Camera_data& cameraData,
	const Light_provider::Callback_type& lightDataProvider,
	Render_pass_blend_mode blendMode,
	bool wireFrame)
{
	auto material = renderable.material.get();

	if (renderable.rendererData == nullptr) {
		if (renderable.meshData == nullptr) {
			// There is no mesh data (it may still be loading!), we can't render.
			return;
		}

		LOG(INFO) << "Creating renderer data for renderable";

		std::vector<Vertex_attribute> vertexAttributes;
		material->vertexAttributes(vertexAttributes);

		auto rendererData = createRendererData(*renderable.meshData, vertexAttributes);
		renderable.rendererData = move(rendererData);
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

	drawRendererData(
		cameraData,
		worldMatrix,
		*renderable.rendererData,
		blendMode,
		*renderLightData,
		*material,
		wireFrame);
}

void Entity_render_manager::loadRendererDataToDeviceContext(const Renderer_data& rendererData)
{
	const auto deviceContext = deviceResources().GetD3DDeviceContext();
	
	// Send the buffers to the input assembler
	if (!rendererData.vertexBuffers.empty()) {
		auto& vertexBuffers = rendererData.vertexBuffers;
		const auto numBuffers = vertexBuffers.size();
		if (numBuffers > 1) {
			_bufferArraySet.bufferArray.clear();
			_bufferArraySet.strideArray.clear();
			_bufferArraySet.offsetArray.clear();

			for (std::vector<Mesh_buffer_accessor>::size_type i = 0; i < numBuffers; ++i) {
				const D3D_buffer_accessor* vertexBufferDesc = rendererData.vertexBuffers[i].get();
				_bufferArraySet.bufferArray.push_back(vertexBufferDesc->buffer->d3dBuffer);
				_bufferArraySet.strideArray.push_back(vertexBufferDesc->stride);
				_bufferArraySet.offsetArray.push_back(vertexBufferDesc->offset);
			}

			deviceContext->IASetVertexBuffers(0, static_cast<UINT>(numBuffers), _bufferArraySet.bufferArray.data(), _bufferArraySet.strideArray.data(), _bufferArraySet.offsetArray.data());
		}
		else {
			const D3D_buffer_accessor* vertexBufferDesc = rendererData.vertexBuffers[0].get();
			deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDesc->buffer->d3dBuffer, &vertexBufferDesc->stride, &vertexBufferDesc->offset);
		}

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
	const Matrix& worldTransform,
	const Renderer_data& rendererData,
	Render_pass_blend_mode blendMode,
	const Render_light_data& renderLightData,
	Material& material,
	bool wireFrame)
{
	if (rendererData.vertexBuffers.empty())
		return;

	auto& commonStates = _scene.manager<ID3D_resources_manager>().commonStates();

	loadRendererDataToDeviceContext(rendererData);

	auto& d3DDeviceResources = deviceResources();
	const auto context = d3DDeviceResources.GetD3DDeviceContext();

	// Set the rasteriser state
	if (wireFrame)
		context->RSSetState(commonStates.Wireframe());
	else
		context->RSSetState(commonStates.CullClockwise());

	// Render the triangles
	material.bind(blendMode, renderLightData, d3DDeviceResources, cameraData.enablePixelShader);

	material.render(rendererData, renderLightData,
		worldTransform, cameraData.viewMatrix, cameraData.projectionMatrix, 
		d3DDeviceResources);

	material.unbind(d3DDeviceResources);
}

std::unique_ptr<Renderer_data> Entity_render_manager::createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const
{
	auto rendererData = std::make_unique<Renderer_data>();

	switch (meshData.m_meshIndexType) {
	case Mesh_index_type::Triangles:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case Mesh_index_type::Lines:
		rendererData->topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		throw std::exception("Unsupported mesh topology");
	}

	// Create D3D Index Buffer
	if (meshData.indexBufferAccessor) {
		rendererData->indexCount = meshData.indexBufferAccessor->count;

		rendererData->indexBufferAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData(*meshData.indexBufferAccessor->buffer, D3D11_BIND_INDEX_BUFFER),
			meshData.indexBufferAccessor->stride,
			meshData.indexBufferAccessor->offset);
		rendererData->indexFormat = meshData.indexBufferAccessor->format;

		std::string name("Index Buffer (count: " + std::to_string(rendererData->indexCount) + ")");
		rendererData->indexBufferAccessor->buffer->d3dBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
	}
	else 
	{
		// TODO: Simply log a warning?
		rendererData->indexCount = 0;
		throw std::runtime_error("CreateRendererData: Missing index buffer");
	}

	// Create D3D vertex buffers
	rendererData->vertexCount = meshData.getVertexCount();
	for (auto vertexAttr : vertexAttributes)
	{
		const auto accessorPos = meshData.vertexBufferAccessors.find(vertexAttr);
		if (accessorPos == meshData.vertexBufferAccessors.end())
			throw std::runtime_error("CreateRendererData: Missing vertex attribute: "s.append(Vertex_attribute_meta::semanticName(vertexAttr)));

		const auto& meshAccessor = accessorPos->second;
		auto d3DAccessor = std::make_unique<D3D_buffer_accessor>(
			createBufferFromData(*meshAccessor->buffer, D3D11_BIND_VERTEX_BUFFER),
			meshAccessor->stride, 
			meshAccessor->offset);

		rendererData->vertexBuffers.push_back(std::move(d3DAccessor));
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
		std::vector<Vertex_attribute> vertexAttributes;
		renderable.material->vertexAttributes(vertexAttributes);
		renderable.rendererData = createRendererData(*renderable.meshData, vertexAttributes);
	}

	return renderable;
}

void Entity_render_manager::clearRenderStats()
{
	_renderStats = {};
}

std::shared_ptr<D3D_buffer> Entity_render_manager::createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const
{
	// Create the vertex buffer.
	auto d3dBuffer = std::make_shared<D3D_buffer>();

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = static_cast<UINT>(buffer.dataSize);
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the sub-resource data.
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = buffer.data;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	// Get a reference
	const auto hr = deviceResources().GetD3DDevice()->CreateBuffer(&bufferDesc, &initData, &d3dBuffer->d3dBuffer);
	assert(!FAILED(hr));

	return d3dBuffer;
}

std::unique_ptr<Material> Entity_render_manager::loadMaterial(const std::string& materialName) const
{
	return _materialRepository->instantiate(materialName);
}
