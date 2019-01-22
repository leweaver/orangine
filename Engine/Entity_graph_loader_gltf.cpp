#include "pch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <tiny_gltf.h>
#include <wincodec.h>

#include <functional>

#include "Material.h"
#include "Entity_repository.h"
#include "Entity_graph_loader_gltf.h"
#include "Renderable_component.h"
#include "Material_repository.h"
#include "PBR_material.h"
#include "Texture.h"
#include "Primitive_mesh_data_factory.h"
#include "Mesh_utils.h"
#include "FileUtils.h"

using namespace std;
using namespace tinygltf;
using namespace oe;

map<Vertex_attribute, string> g_gltfMappingToAttributeMap = {
	{ Vertex_attribute::Position, "POSITION" },
	{ Vertex_attribute::Normal, "NORMAL" },
	{ Vertex_attribute::Tangent, "TANGENT" },
	{ Vertex_attribute::Color, "COLOR" },
	{ Vertex_attribute::Texcoord_0, "TEXCOORD_0" }
};

struct LoaderData
{
	LoaderData(Model& model, string&& baseDir, IWICImagingFactory *imagingFactory, bool calculateBounds)
		: model(model)
		, baseDir(std::move(baseDir))
		, imagingFactory(imagingFactory)
		, calculateBounds(calculateBounds)
	{}

	Model& model;
	string baseDir;
	IWICImagingFactory* imagingFactory;
	map<size_t, shared_ptr<Mesh_buffer>> accessorIdxToMeshBuffers;
	bool calculateBounds;
};

unique_ptr<Mesh_vertex_buffer_accessor> read_vertex_buffer(const Model& model,
	Vertex_attribute vertexAttribute,
	vector<Accessor>::size_type accessorIndex,
	LoaderData& loaderData);

unique_ptr<Mesh_index_buffer_accessor> read_index_buffer(const Model& model,
	vector<Accessor>::size_type accessorIndex,
	LoaderData& loaderData);

shared_ptr<Entity> create_entity(const Node& node, IEntity_repository& entityRepository, IMaterial_repository& materialRepository, LoaderData& loaderData);

string g_pbrPropertyName_BaseColorFactor = "baseColorFactor";
string g_pbrPropertyName_BaseColorTexture = "baseColorTexture";
string g_pbrPropertyName_MetallicFactor = "metallicFactor";
string g_pbrPropertyName_RoughnessFactor = "roughnessFactor";
string g_pbrPropertyName_EmissiveFactor = "emissiveFactor";
string g_pbrPropertyName_MetallicRoughnessTexture = "metallicRoughnessTexture";
string g_pbrPropertyName_AlphaMode = "alphaMode";
string g_pbrPropertyName_AlphaCutoff = "alphaCutoff";

string g_pbrPropertyValue_AlphaMode_Opaque = "OPAQUE";
string g_pbrPropertyValue_AlphaMode_Mask = "MASK";
string g_pbrPropertyValue_AlphaMode_Blend = "BLEND";

Entity_graph_loader_gltf::Entity_graph_loader_gltf()
{
	// Create the COM imaging factory
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_imagingFactory)));
}

void Entity_graph_loader_gltf::getSupportedFileExtensions(vector<string>& extensions) const
{
	extensions.emplace_back("gltf");
}

vector<shared_ptr<Entity>> Entity_graph_loader_gltf::loadFile(string_view filename, 
	IEntity_repository& entityRepository, 
	IMaterial_repository& materialRepository,
	bool calculateBounds) const
{
	vector<shared_ptr<Entity>> entities;
	Model model;
	TinyGLTF loader;
	string err;

	const auto filenameStr = string(filename);
	auto gltfAscii = get_file_contents(filenameStr.c_str());
	auto baseDir = GetBaseDir(filenameStr.c_str());

	const auto ret = loader.LoadASCIIFromString(&model, &err, gltfAscii.c_str(), gltfAscii.length(), baseDir);
	if (!err.empty()) {
		throw runtime_error(err);
	}

	if (!ret) {
		throw runtime_error("Failed to parse glTF: unknown error.");
	}

	if (model.defaultScene >= static_cast<int>(model.scenes.size()) || model.defaultScene < 0)
		throw runtime_error("Failed to parse glTF: defaultScene points to an invalid scene index");

	const auto& scene = model.scenes[model.defaultScene];
	if (baseDir.empty())
		baseDir = ".";
	LoaderData loaderData(model, string(baseDir), _imagingFactory.Get(), calculateBounds);
	
	for (auto nodeIdx : scene.nodes) 
	{
		const auto& rootNode = model.nodes.at(nodeIdx);
		entities.push_back(create_entity(rootNode, entityRepository, materialRepository, loaderData));
	}

	return entities;
}

shared_ptr<oe::Texture> try_create_texture(const LoaderData& loaderData, const tinygltf::Material& gltfMaterial, const std::string& textureName)
{
	int gltfTextureIndex = -1;

	// Look for PBR textures in 'values'
	auto paramPos = gltfMaterial.values.find(textureName);
	if (paramPos != gltfMaterial.values.end()) {
		const auto& param = paramPos->second;
		const auto indexPos = param.json_double_value.find("index");
		if (indexPos == param.json_double_value.end())
			throw runtime_error("missing property 'index' from " + textureName);

		gltfTextureIndex = static_cast<int>(indexPos->second);
	}
	else
	{
		paramPos = gltfMaterial.additionalValues.find(textureName);
		if (paramPos == gltfMaterial.additionalValues.end())
			return nullptr;

		const auto& param = paramPos->second;
		const auto indexPos = param.json_double_value.find("index");
		if (indexPos == param.json_double_value.end())
			throw runtime_error("missing property 'index' from " + textureName);

		gltfTextureIndex = static_cast<int>(indexPos->second);
	}

	assert(loaderData.model.textures.size() < INT_MAX);
	if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(loaderData.model.textures.size()))
		throw runtime_error("invalid texture index '"+to_string(gltfTextureIndex)+"' for " + textureName);

	const auto& gltfTexture = loaderData.model.textures.at(gltfTextureIndex);
	if (gltfTexture.sampler >= 0) {
		// TODO: Read the sampler for sampling details and store on our texture
		const auto& gltfSampler = loaderData.model.samplers.at(gltfTexture.sampler);
	} 
	else
	{
		// todo: create default sampler 
	}

	const auto& gltfImage = loaderData.model.images.at(gltfTexture.source);

	if (!gltfImage.uri.empty())
	{
		const auto filename = utf8_decode(loaderData.baseDir + "\\" + gltfImage.uri);

		return make_shared<File_texture>(wstring(filename));
		/*
		Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
		HRESULT hr = loaderData.imagingFactory->CreateDecoderFromFilename(
			filename.c_str(),				 // Image to be decoded 
			nullptr,                         // Do not prefer a particular vendor 
			GENERIC_READ,                    // Desired read access to the file 
			WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed 
			&decoder                         // Pointer to the decoder 
		);

		// Retrieve the first frame of the image from the decoder 
		IWICBitmapFrameDecode *pFrame = nullptr;
		if (SUCCEEDED(hr))
		{
			hr = decoder->GetFrame(0, &pFrame);
		}

		//Step 3: Format convert the frame to 32bppPBGRA 
		Microsoft::WRL::ComPtr<IWICFormatConverter> formatConverter;
		if (SUCCEEDED(hr))
		{
			hr = loaderData.imagingFactory->CreateFormatConverter(&formatConverter);
		}
		if (SUCCEEDED(hr))
		{
			hr = formatConverter->Initialize(
				pFrame,                          // Input bitmap to convert 
				GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format 
				WICBitmapDitherTypeNone,         // Specified dither pattern 
				nullptr,                         // Specify a particular palette  
				0.f,                             // Alpha threshold 
				WICBitmapPaletteTypeCustom       // Palette translation type 
			);
		}

		UINT width, height;
		if (SUCCEEDED(hr))
		{
			hr = pFrame->GetSize(&width, &height);
		}
		if (SUCCEEDED(hr))
		{
			UINT stride = 4 * sizeof(uint8_t);
			UINT rowStride = stride * width;
			UINT bufferSize = rowStride * height;
			unique_ptr<uint8_t> buffer = unique_ptr<uint8_t>(new uint8_t[bufferSize]);
			
			// Not really required; null should be equivolent
			hr = formatConverter->CopyPixels(nullptr, rowStride, bufferSize, buffer.get());
			if (SUCCEEDED(hr))
			{
				return make_shared<OE::Texture>(width, height, stride, bufferSize, buffer);
			}
		}
		*/
	}
	else if (!gltfImage.mimeType.empty())
	{
		throw runtime_error("not implemented");
	}
	else
	{
		throw runtime_error("image " + to_string(gltfTexture.source) + "must have either a uri or mimeType");
	}
}

unique_ptr<oe::Material> create_material(const Primitive& prim, IMaterial_repository& materialRepository, LoaderData& loaderData)
{
	const tinygltf::Material& gltfMaterial = loaderData.model.materials.at(prim.material);
	//auto material = materialRepository.instantiate(std::string("PBR_material"));
	auto material = std::make_unique<PBR_material>();

	const auto withParam = [gltfMaterial](const string& name, std::function<void(const string&, const Parameter&)> found, std::function<void(const string&)> notFound) {
		auto paramPos = gltfMaterial.values.find(name);
		if (paramPos != gltfMaterial.values.end()) {
			found(name, paramPos->second);
			return true;
		}

		paramPos = gltfMaterial.additionalValues.find(name);
		if (paramPos != gltfMaterial.additionalValues.end()) {
			found(name, paramPos->second);
			return true;
		}

		notFound(name);
		return false;
	};

	const auto withScalarParam = [&withParam, &material](const string& name, float defaultValue, function<void(float)> setter) {
		withParam(name, [&setter](auto name, auto param) {
			if (param.number_array.size() == 1) {
				setter(static_cast<float>(param.number_array[0]));
			}
			else {
				throw runtime_error("Failed to parse glTF: expected material " + name + " to be scalar");
			}
		}, [&setter, defaultValue](auto) {
			setter(defaultValue);
		});
	};

	const auto withColorParam = [&withParam, &material](const string& name, DirectX::SimpleMath::Color defaultValue, function<void(DirectX::SimpleMath::Color)> setter) {
		withParam(name, [&setter](auto name, auto param) {
			if (param.number_array.size() == 3) {
				setter(DirectX::SimpleMath::Color(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2]),
					1.0f
				));
			}
			else if(param.number_array.size() == 4) {
				setter(DirectX::SimpleMath::Color(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2]),
					static_cast<float>(param.number_array[3])
				));
			}
			else
				throw runtime_error("Failed to parse glTF: expected material " + name + " to be a 3 or 4 element array");
		}, [&setter, defaultValue](auto) {
			setter(defaultValue);
		});
	};

	// Numeric/Color Parameters
	withScalarParam(g_pbrPropertyName_MetallicFactor, 1.0f, bind(&PBR_material::setMetallicFactor, material.get(), std::placeholders::_1));
	withScalarParam(g_pbrPropertyName_RoughnessFactor, 1.0f, bind(&PBR_material::setRoughnessFactor, material.get(), std::placeholders::_1));
	withScalarParam(g_pbrPropertyName_AlphaCutoff, 0.5f, bind(&PBR_material::setAlphaCutoff, material.get(), std::placeholders::_1));
	withColorParam(g_pbrPropertyName_BaseColorFactor, DirectX::SimpleMath::Color(DirectX::Colors::White), bind(&PBR_material::setBaseColor, material.get(), std::placeholders::_1));
	withColorParam(g_pbrPropertyName_EmissiveFactor, DirectX::SimpleMath::Color(DirectX::Colors::Black), bind(&PBR_material::setEmissiveFactor, material.get(), std::placeholders::_1));
	
	// Alpha Mode
	withParam(g_pbrPropertyName_AlphaMode, [&material](const string& name, const Parameter& param) {
		if (param.string_value == g_pbrPropertyValue_AlphaMode_Mask)
			material->setAlphaMode(Material_alpha_mode::Mask);
		else if (param.string_value == g_pbrPropertyValue_AlphaMode_Blend)
			material->setAlphaMode(Material_alpha_mode::Blend);
		else if (param.string_value == g_pbrPropertyValue_AlphaMode_Opaque)
			material->setAlphaMode(Material_alpha_mode::Opaque);
		else {
			throw runtime_error("Failed to parse glTF: expected material " + name + " to be one of:" +
				g_pbrPropertyValue_AlphaMode_Mask + ", " +
				g_pbrPropertyValue_AlphaMode_Blend + ", " + 
				g_pbrPropertyValue_AlphaMode_Opaque
			);
		}
	}, [&material](const string&) {
		material->setAlphaMode(Material_alpha_mode::Opaque);
	});

	// PBR Textures
	material->setBaseColorTexture(try_create_texture(loaderData, gltfMaterial, "baseColorTexture"));
	material->setMetallicRoughnessTexture(try_create_texture(loaderData, gltfMaterial, "metallicRoughnessTexture"));
	material->setOcclusionTexture(try_create_texture(loaderData, gltfMaterial, "occlusionTexture"));
	material->setEmissiveTexture(try_create_texture(loaderData, gltfMaterial, "emissiveTexture"));

	// Material Textures
	material->setNormalTexture(try_create_texture(loaderData, gltfMaterial, "normalTexture"));

	return material;
}

void setEntityTransform(Entity&  entity, const Node&  node)
{
	if (node.matrix.size() == 16)
	{
		float elements[16];
		for (auto i = 0; i < 16; ++i)
			elements[i] = static_cast<float>(node.matrix[i]);
		auto trs = DirectX::SimpleMath::Matrix(elements);

		DirectX::SimpleMath::Vector3 scale;
		DirectX::SimpleMath::Quaternion rotation;
		DirectX::SimpleMath::Vector3 translation;
		trs.Decompose(scale, rotation, translation);

		entity.setScale(scale);
		entity.setRotation(rotation);
		entity.setPosition(translation);
	}
	else
	{
		if (node.translation.size() == 3)
		{
			entity.setPosition({
					static_cast<float>(node.translation[0]),
					static_cast<float>(node.translation[1]),
					static_cast<float>(node.translation[2])
				});
		}
		if (node.rotation.size() == 4)
		{
			entity.setRotation({
				static_cast<float>(node.rotation[0]),
				static_cast<float>(node.rotation[1]),
				static_cast<float>(node.rotation[2]),
				static_cast<float>(node.rotation[3])
				});
		}
		if (node.scale.size() == 3)
		{
			entity.setScale({
				static_cast<float>(node.scale[0]),
				static_cast<float>(node.scale[1]),
				static_cast<float>(node.scale[2])
				});
		}
	}
}

shared_ptr<Entity> create_entity(const Node& node, IEntity_repository& entityRepository, IMaterial_repository& materialRepository, LoaderData& loaderData)
{
	auto rootEntity = entityRepository.instantiate(node.name);

	LOG(G3LOG_DEBUG) << "Creating entity for glTF node '" << node.name << "'";

	// Transform
	setEntityTransform(*rootEntity, node);
	
	// Create MeshData
	if (node.mesh > -1) {
		const auto& mesh = loaderData.model.meshes.at(node.mesh);
		const auto primitiveCount = mesh.primitives.size();
		for (size_t primIdx = 0; primIdx < primitiveCount; ++primIdx)
		{
			const auto primitiveName = mesh.name + " primitive " + to_string(primIdx);
			LOG(G3LOG_DEBUG) << "Creating entity for glTF mesh " << primitiveName;
			auto primitiveEntity = entityRepository.instantiate(primitiveName);

			primitiveEntity->setParent(*rootEntity.get());
			auto& meshDataComponent = primitiveEntity->addComponent<Mesh_data_component>();
			auto meshData = std::make_shared<Mesh_data>();
			meshDataComponent.setMeshData(meshData);

			try {
				const auto& prim = mesh.primitives.at(primIdx);

				auto material = create_material(prim, materialRepository, loaderData);

				// Read Index
				try
				{
					auto accessor = read_index_buffer(loaderData.model, prim.indices, loaderData);
					meshData->indexBufferAccessor = move(accessor);
				}
				catch (const exception& e)
				{
					throw runtime_error(string("Error in index buffer: ") + e.what());
				}

				// First, look though the attributes and extract accessor information
				// Look for the things that the material requires
				vector<Vertex_attribute> materialAttributes;
				material->vertexAttributes(materialAttributes);

				// First, we need to see how many vertices there are.
				uint32_t vertexCount;
				{
					const auto& gltfPositionAttributeName = g_gltfMappingToAttributeMap[Vertex_attribute::Position];
					const auto gltfAttributePos = prim.attributes.find(gltfPositionAttributeName);
					if (gltfAttributePos == prim.attributes.end())
						throw logic_error("Missing " + gltfPositionAttributeName + " attribute");

					const auto dataLen = loaderData.model.accessors.at(gltfAttributePos->second).count;
					assert(dataLen < UINT32_MAX);

					vertexCount = static_cast<UINT>(dataLen);
				}

				// TODO: We could consider interleaving things in a single buffer? Not sure if there is a benefit?

				bool generateNormals = false,
					generateTangents = false,
					generateBitangents = false;
				for (const auto& requiredAttr : materialAttributes) {
					const auto& mappingPos = g_gltfMappingToAttributeMap.find(requiredAttr);
					if (mappingPos == g_gltfMappingToAttributeMap.end()) {
						throw logic_error("gltf loader does not support attribute: "s.append(Vertex_attribute_meta::str(requiredAttr)));
					}
					
					const auto& gltfAttributeName = mappingPos->second;
					const auto& primAttrPos = prim.attributes.find(gltfAttributeName);
					if (primAttrPos == prim.attributes.end()) {
						// We only support generation of normals, tangents, bitangent
						if (requiredAttr == Vertex_attribute::Normal)
							generateNormals = true;
						else if (requiredAttr == Vertex_attribute::Tangent)
							generateTangents = true; 
						else if (requiredAttr == Vertex_attribute::Bi_Tangent)
							generateBitangents = true;
						else
							throw logic_error("glTF Mesh does not have required attribute: "s.append(gltfAttributeName));

						// Create the missing accessor
						const auto elementStride = Vertex_attribute_meta::elementSize(requiredAttr);
						meshData->vertexBufferAccessors[requiredAttr] = make_unique<Mesh_vertex_buffer_accessor>(
							make_shared<Mesh_buffer>(elementStride * vertexCount), requiredAttr,
							static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(elementStride), 0);
					}
					else {
						try {
							auto accessor = read_vertex_buffer(loaderData.model, requiredAttr, primAttrPos->second, loaderData);
							meshData->vertexBufferAccessors[requiredAttr] = move(accessor);
						}
						catch (const exception& e)
						{
							throw runtime_error("Error in attribute " + gltfAttributeName + ": " + e.what());
						}
					}
				}

				if (generateNormals)
				{
					LOG(WARNING) << "Generating missing normals";
					Primitive_mesh_data_factory::generateNormals(
						*meshData->indexBufferAccessor, 
						*meshData->vertexBufferAccessors[Vertex_attribute::Position],
						*meshData->vertexBufferAccessors[Vertex_attribute::Normal]);
				}

				if (generateTangents || generateBitangents)
				{
					LOG(WARNING) << "Generating missing Tangents and/or Bitangents";
                    Primitive_mesh_data_factory::generateTangents(meshData);
				}

				// Add this component last, to make sure there wasn't an error loading!
				auto& renderableComponent = primitiveEntity->addComponent<Renderable_component>();
				renderableComponent.setMaterial(move(material));

				// Calculate bounds?
				if (loaderData.calculateBounds) {
					DirectX::BoundingSphere boundingSphere;

					const auto& vertexBufferAccessor = meshData->vertexBufferAccessors[Vertex_attribute::Position];
					DirectX::BoundingSphere::CreateFromPoints(boundingSphere,
						vertexBufferAccessor->count,
						reinterpret_cast<DirectX::XMFLOAT3*>(vertexBufferAccessor->buffer->data),
						vertexBufferAccessor->stride);

					primitiveEntity->setBoundSphere(boundingSphere);
				}
			}
			catch (const exception& e)
			{
				throw runtime_error(string("Primitive[") + to_string(primIdx) + "] is malformed. (" + e.what() + ")");
			}
		}
	}

	for (auto childIdx : node.children)
	{
		const auto& childNode = loaderData.model.nodes.at(childIdx);
		auto childEntity = create_entity(childNode, entityRepository, materialRepository, loaderData);
		childEntity->setParent(*rootEntity.get());
	}

	return rootEntity;
}

unique_ptr<Mesh_vertex_buffer_accessor> read_vertex_buffer(const Model& model,
	Vertex_attribute vertexAttribute,
	const vector<Accessor>::size_type accessorIndex,
	LoaderData& loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw domain_error("accessor[" + to_string(accessorIndex) + "] out of range.");
	const auto& accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
		throw domain_error("bufferView[" + to_string(accessor.bufferView) + "] out of range.");
	const auto& bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.target != 0 && bufferView.target != TINYGLTF_TARGET_ARRAY_BUFFER)
		throw runtime_error("Index bufferView must have type ARRAY_BUFFER (" + to_string(TINYGLTF_TARGET_ARRAY_BUFFER) + ")");

	if (bufferView.buffer >= static_cast<int>(model.buffers.size()))
		throw domain_error("buffer[" + to_string(bufferView.buffer) + "] out of range.");
	const auto& buffer = model.buffers.at(bufferView.buffer);

	// Read data from the glTF buffer into a new buffer.
	const auto expectedElementSize = Vertex_attribute_meta::elementSize(vertexAttribute);

	// Validate the elementSize and buffer size
	size_t sourceElementSize;
	if (accessor.type == TINYGLTF_TYPE_SCALAR)
		sourceElementSize = sizeof(unsigned int);
	else if (accessor.type == TINYGLTF_TYPE_VEC2)
		sourceElementSize = sizeof(float) * 2;
	else if (accessor.type == TINYGLTF_TYPE_VEC3)
		sourceElementSize = sizeof(float) * 3;
	else if (accessor.type == TINYGLTF_TYPE_VEC4)
		sourceElementSize = sizeof(float) * 4;
	else
		throw runtime_error("Unknown accessor type: " + to_string(accessor.type));

	if (expectedElementSize != sourceElementSize)
	{
		throw runtime_error("cannot process attribute " +
			g_gltfMappingToAttributeMap[vertexAttribute] +
			", element size must be " + to_string(expectedElementSize) +
			" but was " + to_string(sourceElementSize) + ".");
	}
	
	// Determine the stride - if none is provided, default to sourceElementSize.
	const size_t sourceStride = bufferView.byteStride == 0 ? sourceElementSize : bufferView.byteStride;

	// does the data range defined by the accessor fit into the bufferView?
	if (accessor.byteOffset + accessor.count * sourceStride > bufferView.byteLength)
		throw runtime_error("Accessor " + to_string(accessorIndex) + " exceeds maximum size of bufferView " + to_string(accessor.bufferView));

	// does the data range defined by the accessor and bufferview fit into the buffer?
	const size_t bufferOffset = bufferView.byteOffset + accessor.byteOffset;
	if (bufferOffset + accessor.count * sourceStride > buffer.data.size())
		throw runtime_error("BufferView " + to_string(accessor.bufferView) + " exceeds maximum size of buffer " + to_string(bufferView.buffer));
		
	// Have we already created an identical buffer? If so, re-use it.
	shared_ptr<Mesh_buffer> meshBuffer;
	const auto pos = loaderData.accessorIdxToMeshBuffers.find(accessorIndex);

	if (pos != loaderData.accessorIdxToMeshBuffers.end())
		meshBuffer = pos->second;
	else
	{
		// Copy the data.
		LOG(G3LOG_DEBUG) << "Reading vertex buffer data: " << g_gltfMappingToAttributeMap[vertexAttribute];
		meshBuffer = mesh_utils::create_buffer_s(sourceElementSize, accessor.count, buffer.data, sourceStride, bufferOffset);
		loaderData.accessorIdxToMeshBuffers[accessorIndex] = meshBuffer;
		/*
		 // Output stream data to the log
		if (accessor.type == TINYGLTF_TYPE_VEC2)
		{
			float *floatData = reinterpret_cast<float*>(meshBuffer->m_data);
			for (size_t i = 0; i < accessor.count; i++)
			{
				LOG(G3LOG_DEBUG) << to_string(i) << ": " << floatData[0] << ", " << floatData[1];
				floatData += 2;
			}			
		}
		*/

		/*
		if (vertexAttribute == VertexAttribute::VA_POSITION && elementSize == 3 * sizeof(float))
		{
			float *vec3data = reinterpret_cast<float *>(meshBuffer->m_data);
			for (UINT i = 0; i < accessor.count; ++i) {
				// Invert the Z axis
				vec3data[2] *= -1;
				vec3data += 3;
			}			
		}
		*/
	}

	LOG(G3LOG_DEBUG) << "Creating Vertex Buffer Accessor.   type: " << Vertex_attribute_meta::semanticName(vertexAttribute) << "   count: " << accessor.count;

	return make_unique<Mesh_vertex_buffer_accessor>(meshBuffer, vertexAttribute, static_cast<UINT>(accessor.count), static_cast<UINT>(sourceElementSize), 0);
}

unique_ptr<Mesh_index_buffer_accessor> read_index_buffer(const Model& model,
	const vector<Accessor>::size_type accessorIndex,
	LoaderData& loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw domain_error("accessor[" + to_string(accessorIndex) + "] out of range.");
	const auto& accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= static_cast<int32_t>(model.bufferViews.size()))
		throw domain_error("bufferView[" + to_string(accessor.bufferView) + "] out of range.");
	const auto& bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.buffer >= static_cast<int>(model.buffers.size()))
		throw domain_error("buffer[" + to_string(bufferView.buffer) + "] out of range.");
	const auto& buffer = model.buffers.at(bufferView.buffer);

	if (bufferView.target != 0 && bufferView.target != TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
		throw runtime_error("Index bufferView must have type ELEMENT_ARRAY_BUFFER (" + to_string(TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) + ")");
	
	if (accessor.type != TINYGLTF_TYPE_SCALAR)
		throw runtime_error("Index buffer must be scalar");

	DXGI_FORMAT format;
	size_t sourceElementSize;
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		format = DXGI_FORMAT_R8_UINT;
		sourceElementSize = sizeof(uint8_t);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		format = DXGI_FORMAT_R8_SINT;
		sourceElementSize = sizeof(int8_t);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		format = DXGI_FORMAT_R16_UINT;
		sourceElementSize = sizeof(uint16_t);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		format = DXGI_FORMAT_R16_SINT;
		sourceElementSize = sizeof(int16_t);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		format = DXGI_FORMAT_R32_UINT;
		sourceElementSize = sizeof(uint32_t);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		format = DXGI_FORMAT_R32_SINT;
		sourceElementSize = sizeof(int32_t);
		break;
	default:
		throw runtime_error("Unknown index buffer format: " + to_string(accessor.componentType));
	}
	
	// Determine the stride - if none is provided, default to sourceElementSize.
	const size_t sourceStride = bufferView.byteStride == 0 ? sourceElementSize : bufferView.byteStride;

	// does the data range defined by the accessor fit into the bufferView?
	if (accessor.byteOffset + accessor.count * sourceStride > bufferView.byteLength)
		throw runtime_error("Accessor " + to_string(accessorIndex) + " exceeds maximum size of bufferView " + to_string(accessor.bufferView));

	// does the data range defined by the accessor and bufferview fit into the buffer?
	const size_t bufferOffset = bufferView.byteOffset + accessor.byteOffset;
	if (bufferOffset + accessor.count * sourceStride > buffer.data.size())
		throw runtime_error("BufferView " + to_string(accessor.bufferView) + " exceeds maximum size of buffer " + to_string(bufferView.buffer));

	// Have we already created an identical buffer? If so, re-use it.
	shared_ptr<Mesh_buffer> meshBuffer;
	const auto pos = loaderData.accessorIdxToMeshBuffers.find(accessorIndex);

	if (pos != loaderData.accessorIdxToMeshBuffers.end())
	{
		LOG(G3LOG_DEBUG) << "Found existing MeshBuffer instance, re-using.";
		meshBuffer = pos->second;
	}
	else
	{
		LOG(G3LOG_DEBUG) << "Creating new MeshBuffer instance.";

		if (format == DXGI_FORMAT_R8_UINT || format == DXGI_FORMAT_R8_SINT) {
			// Transform the data to a known format
			auto [ indexMeshBuffer, indexFormat ] = mesh_utils::create_index_buffer(
				buffer.data,
				static_cast<uint32_t>(accessor.count),
				format,
				static_cast<uint32_t>(sourceStride),
				static_cast<uint32_t>(bufferOffset)
			);
			meshBuffer = indexMeshBuffer;
			format = indexFormat;
		}
		else {
			// Copy the data.
			meshBuffer = mesh_utils::create_buffer(
				static_cast<uint32_t>(sourceElementSize),
				static_cast<uint32_t>(accessor.count),
				buffer.data,
				static_cast<uint32_t>(sourceStride),
				static_cast<uint32_t>(bufferOffset));
		}
		loaderData.accessorIdxToMeshBuffers[accessorIndex] = meshBuffer;
	}
	
	LOG(G3LOG_DEBUG) << "Creating Index Buffer Accessor.   DXGI format: " << format << "   count: " << accessor.count;

	return make_unique<Mesh_index_buffer_accessor>(meshBuffer, format, static_cast<UINT>(accessor.count), static_cast<UINT>(sourceElementSize), 0);
}
