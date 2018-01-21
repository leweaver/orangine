#include "pch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <tiny_gltf.h>
#include <wincodec.h>

#include "Material.h"
#include "EntityRepository.h"
#include "glTFMeshLoader.h"
#include "RenderableComponent.h"
#include "MaterialRepository.h"
#include "PBRMaterial.h"
#include "TextureImpl.h"

using namespace std;
using namespace tinygltf;
using namespace OE;

map<VertexAttribute, string> g_gltfMappingToAttributeMap = {
	{ VertexAttribute::VA_POSITION, "POSITION" },
	{ VertexAttribute::VA_NORMAL, "NORMAL" },
	{ VertexAttribute::VA_TANGENT, "TANGENT" },
	{ VertexAttribute::VA_COLOR, "COLOR" },
	{ VertexAttribute::VA_TEXCOORD_0, "TEXCOORD_0" }
};

struct LoaderData
{
	LoaderData(Model &model, const string &baseDir, IWICImagingFactory *imagingFactory) 
		: model(model)
		, baseDir(baseDir)
		, imagingFactory(imagingFactory)
	{}

	Model &model;
	string baseDir;
	IWICImagingFactory* imagingFactory;
	map<size_t, shared_ptr<MeshBuffer>> accessorIdxToMeshVertexBuffers;
};

unique_ptr<MeshVertexBufferAccessor> read_vertex_buffer(const Model& model,
	VertexAttribute vertexAttribute,
	vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData);

unique_ptr<MeshIndexBufferAccessor> read_index_buffer(const Model& model,
	vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData);

shared_ptr<Entity> create_entity(const Node &node, EntityRepository &entityRepository, MaterialRepository &materialRepository, LoaderData &loaderData);
shared_ptr<MeshBuffer> create_buffer_s(size_t elementSize, size_t elementCount, const vector<uint8_t> &sourceData, size_t sourceStride, size_t sourceOffset);
shared_ptr<MeshBuffer> create_buffer(UINT elementSize, UINT elementCount, const vector<uint8_t> &sourceData, UINT sourceStride, UINT sourceOffset);

string g_pbrPropertyName_BaseColorFactor = "baseColorFactor";
string g_pbrPropertyName_BaseColorTexture = "baseColorTexture";
string g_pbrPropertyName_MetallicFactor = "metallicFactor";
string g_pbrPropertyName_RoughnessFactor = "roughnessFactor";
string g_pbrPropertyName_MetallicRoughnessTexture = "metallicRoughnessTexture";

glTFMeshLoader::glTFMeshLoader()
{
	// Create the COM imaging factory
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_imagingFactory));
	assert(SUCCEEDED(hr));
}

void glTFMeshLoader::GetSupportedFileExtensions(vector<string> &extensions) const
{
	extensions.emplace_back("gltf");
}

vector<shared_ptr<Entity>> glTFMeshLoader::LoadFile(const string& filename, EntityRepository &entityRepository, MaterialRepository &materialRepository) const
{
	vector<shared_ptr<Entity>> entities;
	Model model;
	TinyGLTF loader;
	string err;

	const bool ret = loader.LoadASCIIFromFile(&model, &err, filename);
	//bool ret = loader.LoadBinaryFromFile(&model, &err, argv[1]); // for binary glTF(.glb) 
	if (!err.empty()) {
		throw runtime_error(err);
	}

	if (!ret) {
		throw runtime_error("Failed to parse glTF: unknown error.");
	}

	if (model.defaultScene >= static_cast<int>(model.scenes.size()) || model.defaultScene < 0)
		throw runtime_error("Failed to parse glTF: defaultScene points to an invalid scene index");

	const auto &scene = model.scenes[model.defaultScene];
	string baseDir = tinygltf::GetBaseDir(filename);
	if (baseDir.empty())
		baseDir = ".";
	LoaderData loaderData(model, baseDir, m_imagingFactory.Get());
	
	for (int nodeIdx : scene.nodes) 
	{
		const Node &rootNode = model.nodes.at(nodeIdx);
		entities.push_back(create_entity(rootNode, entityRepository, materialRepository, loaderData));
	}

	return entities;
}

shared_ptr<OE::Texture> try_create_texture(const LoaderData &loaderData, const tinygltf::Material &gltfMaterial, const std::string &textureName)
{
	const auto paramPos = gltfMaterial.values.find(textureName);
	if (paramPos == gltfMaterial.values.end())
		return nullptr;

	const auto &param = paramPos->second;
	const auto indexPos = param.json_double_value.find("index");
	if (indexPos == param.json_double_value.end())
		throw runtime_error("missing property 'index' from " + textureName);

	const auto gltfTextureIndex = static_cast<int>(indexPos->second);
	assert(loaderData.model.textures.size() < INT_MAX);
	if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(loaderData.model.textures.size()))
		throw runtime_error("invalid texture index '"+to_string(gltfTextureIndex)+"' for " + textureName);

	const auto &gltfTexture = loaderData.model.textures.at(gltfTextureIndex);
	// TODO: Read the sampler for sampling details and store on our texture
	const auto &gltfSampler = loaderData.model.samplers.at(gltfTexture.sampler);
	const auto &gltfImage = loaderData.model.images.at(gltfTexture.source);

	if (!gltfImage.uri.empty())
	{
		const auto filename = utf8_decode(loaderData.baseDir + "\\" + gltfImage.uri);

		return make_shared<OE::FileTexture>(filename, true);
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

unique_ptr<OE::Material> create_material(const Primitive& prim, MaterialRepository &materialRepository, LoaderData &loaderData)
{
	const tinygltf::Material &gltfMaterial = loaderData.model.materials.at(prim.material);
	auto material = materialRepository.instantiate<PBRMaterial>();


	// base color
	{
		const auto paramPos = gltfMaterial.values.find(g_pbrPropertyName_BaseColorFactor);
		if (paramPos != gltfMaterial.values.end()) {
			const auto &param = paramPos->second;
			if (param.number_array.size() == 4) {
				const auto color = DirectX::XMVectorSet(
					static_cast<float>(param.number_array[0]),
					static_cast<float>(param.number_array[1]),
					static_cast<float>(param.number_array[2]),
					static_cast<float>(param.number_array[3])
				);
				material->setBaseColor(color);
			}
			else
				throw runtime_error("Failed to parse glTF: expected material baseColorFactor to be an array of 4 numbers");
		}
	}

	// base color texture
	{
		material->setBaseColorTexture(try_create_texture(loaderData, gltfMaterial, "baseColorTexture"));
	}

	// TODO: Other material parameters; roughness etc
	return material;
}

shared_ptr<Entity> create_entity(const Node &node, EntityRepository &entityRepository, MaterialRepository &materialRepository, LoaderData &loaderData)
{
	shared_ptr<Entity> rootEntity = entityRepository.Instantiate(node.name);

	if (node.mesh > -1) {
		const Mesh &mesh = loaderData.model.meshes.at(node.mesh);
		auto numPrims = mesh.primitives.size();
		for (size_t primIdx = 0; primIdx < numPrims; ++primIdx)
		{
			shared_ptr<Entity> primitiveEntity = entityRepository.Instantiate(node.name + " primitive " + to_string(primIdx));
			primitiveEntity->SetParent(*rootEntity.get());
			MeshDataComponent &meshDataComponent = primitiveEntity->AddComponent<MeshDataComponent>();
			auto meshData = std::make_shared<MeshData>();
			meshDataComponent.setMeshData(meshData);

			try {
				const auto &prim = mesh.primitives.at(primIdx);

				unique_ptr<OE::Material> material = create_material(prim, materialRepository, loaderData);

				// Read Index
				try
				{
					auto accessor = read_index_buffer(loaderData.model, prim.indices, loaderData);
					meshData->m_indexBufferAccessor = move(accessor);
				}
				catch (const exception &e)
				{
					throw runtime_error(string("Error in index buffer: ") + e.what());
				}

				// First, look though the attributes and extract accessor information
				// Look for the things that the material requires
				vector<VertexAttribute> materialAttributes;
				material->getVertexAttributes(materialAttributes);

				// First, we need to see how many vertices there are.
				uint32_t vertexCount;
				{
					const string &gltfPositionAttributeName = g_gltfMappingToAttributeMap[VertexAttribute::VA_POSITION];
					const auto gltfAttributePos = prim.attributes.find(gltfPositionAttributeName);
					if (gltfAttributePos == prim.attributes.end())
						throw logic_error("Missing " + gltfPositionAttributeName + " attribute");

					const size_t dataLen = loaderData.model.accessors.at(gltfAttributePos->second).count;
					assert(dataLen < UINT32_MAX);

					vertexCount = static_cast<UINT>(dataLen);
				}

				// TODO: We could consider interleaving things in a single buffer? Not sure if there is a benefit?

				for (const auto &requiredAttr : materialAttributes) {
					const auto &mappingPos = g_gltfMappingToAttributeMap.find(requiredAttr);
					if (mappingPos == g_gltfMappingToAttributeMap.end()) {
						throw logic_error("gltf loader does not support attribute: " + VertexAttributeMeta::str(requiredAttr));
					}
					
					const string &gltfAttributeName = mappingPos->second;
					const auto &primAttrPos = prim.attributes.find(gltfAttributeName);
					if (primAttrPos == prim.attributes.end()) {
						// TODO: Fill in this stream with generated values?
						// if (normal) { generate normals }
						// if (tangent) { generate tangents }
						const size_t stride = VertexAttributeMeta::elementSize(requiredAttr);
						auto buffer = make_shared<MeshBuffer>(stride * vertexCount);
						memset(buffer->m_data, 0, buffer->m_dataSize);

						// TODO: Consider logging a warning that we defaulted values here.
						//throw logic_error("glTF Mesh does not have required attribute: " + gltfAttributeName);
						auto accessor = make_unique<MeshVertexBufferAccessor>(buffer, requiredAttr, vertexCount, static_cast<UINT>(stride), 0);
						meshData->m_vertexBufferAccessors[requiredAttr] = move(accessor);
					}
					else {
						try {
							unique_ptr<MeshVertexBufferAccessor> accessor = read_vertex_buffer(loaderData.model, requiredAttr, primAttrPos->second, loaderData);
							meshData->m_vertexBufferAccessors[requiredAttr] = move(accessor);
						}
						catch (const exception &e)
						{
							throw runtime_error("Error in attribute " + gltfAttributeName + ": " + e.what());
						}
					}
				}

				// Add this component last, to make sure there wasn't an error loading!
				auto &renderableComponent = primitiveEntity->AddComponent<RenderableComponent>();
				renderableComponent.SetMaterial(material);
			}
			catch (const exception &e)
			{
				throw runtime_error(string("Primitive[") + to_string(primIdx) + "] is malformed. (" + e.what() + ")");
			}
		}
	}

	for (auto childIdx : node.children)
	{
		const Node &childNode = loaderData.model.nodes.at(childIdx);
		auto childEntity = create_entity(childNode, entityRepository, materialRepository, loaderData);
		childEntity->SetParent(*rootEntity.get());
	}

	return rootEntity;
}

unique_ptr<MeshVertexBufferAccessor> read_vertex_buffer(const Model& model,
	VertexAttribute vertexAttribute,
	const vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw domain_error("accessor[" + to_string(accessorIndex) + "] out of range.");
	const auto &accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
		throw domain_error("bufferView[" + to_string(accessor.bufferView) + "] out of range.");
	const auto &bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.buffer >= static_cast<int>(model.buffers.size()))
		throw domain_error("buffer[" + to_string(bufferView.buffer) + "] out of range.");
	const auto &buffer = model.buffers.at(bufferView.buffer);

	// Read data from the glTF buffer into a new buffer.
	const size_t expectedElementSize = VertexAttributeMeta::elementSize(vertexAttribute);

	// Validate the elementSize and buffer size
	size_t elementSize;
	if (accessor.type == TINYGLTF_TYPE_SCALAR)
		elementSize = sizeof(unsigned int);
	else if (accessor.type == TINYGLTF_TYPE_VEC2)
		elementSize = sizeof(float) * 2;
	else if (accessor.type == TINYGLTF_TYPE_VEC3)
		elementSize = sizeof(float) * 3;
	else if (accessor.type == TINYGLTF_TYPE_VEC4)
		elementSize = sizeof(float) * 4;
	else
		throw runtime_error("Unknown accessor type: " + to_string(accessor.type));

	const size_t byteWidth = elementSize * accessor.count;
	if (byteWidth < bufferView.byteLength)
		throw runtime_error("required byteWidth (" + to_string(byteWidth) + ") is larger than bufferView byteLength (" + to_string(bufferView.byteLength));
	if (expectedElementSize != elementSize)
	{
		throw runtime_error("cannot process attribute " +
			g_gltfMappingToAttributeMap[vertexAttribute] +
			", element size must be " + to_string(expectedElementSize) +
			" but was " + to_string(elementSize) + ".");
	}

	const size_t stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;
	
	// Have we already created an identical buffer? If so, re-use it.
	shared_ptr<MeshBuffer> meshBuffer;
	const auto pos = loaderData.accessorIdxToMeshVertexBuffers.find(accessorIndex);

	if (pos != loaderData.accessorIdxToMeshVertexBuffers.end())
		meshBuffer = pos->second;
	else
	{
		// Copy the data.
		LOG(INFO) << "Reading vertex buffer data: " << g_gltfMappingToAttributeMap[vertexAttribute];
		meshBuffer = create_buffer_s(elementSize, accessor.count, buffer.data, stride, bufferView.byteOffset);
		loaderData.accessorIdxToMeshVertexBuffers[accessorIndex] = meshBuffer;
		/*
		 // Output stream data to the log
		if (accessor.type == TINYGLTF_TYPE_VEC2)
		{
			float *floatData = reinterpret_cast<float*>(meshBuffer->m_data);
			for (size_t i = 0; i < accessor.count; i++)
			{
				LOG(INFO) << to_string(i) << ": " << floatData[0] << ", " << floatData[1];
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

	return make_unique<MeshVertexBufferAccessor>(meshBuffer, vertexAttribute, static_cast<UINT>(accessor.count), static_cast<UINT>(elementSize), 0);
}

unique_ptr<MeshIndexBufferAccessor> read_index_buffer(const Model& model,
	const vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw domain_error("accessor[" + to_string(accessorIndex) + "] out of range.");
	const auto &accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= static_cast<int32_t>(model.bufferViews.size()))
		throw domain_error("bufferView[" + to_string(accessor.bufferView) + "] out of range.");
	const auto &bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.buffer >= static_cast<int>(model.buffers.size()))
		throw domain_error("buffer[" + to_string(bufferView.buffer) + "] out of range.");
	const auto &buffer = model.buffers.at(bufferView.buffer);
	
	if (accessor.type != TINYGLTF_TYPE_SCALAR)
		throw runtime_error("Index buffer must be scalar");

	DXGI_FORMAT format;
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		format = DXGI_FORMAT_R16_UINT;
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		format = DXGI_FORMAT_R16_SINT;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		format = DXGI_FORMAT_R32_UINT;
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		format = DXGI_FORMAT_R32_SINT;
		break;
	default:
		throw runtime_error("Unknown index buffer format: " + to_string(accessor.componentType));
	}

	const size_t elementSize = sizeof(UINT);
	const size_t byteWidth = elementSize * accessor.count;
	if (byteWidth < bufferView.byteLength)
		throw runtime_error("required byteWidth (" + to_string(byteWidth) + ") is larger than bufferView byteLength (" + to_string(bufferView.byteLength));
	
	// Copy the data.
	const size_t stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;	

	// Have we already created an identical buffer? If so, re-use it.
	shared_ptr<MeshBuffer> meshBuffer;
	const auto pos = loaderData.accessorIdxToMeshVertexBuffers.find(accessorIndex);

	if (pos != loaderData.accessorIdxToMeshVertexBuffers.end())
		meshBuffer = pos->second;
	else
	{
		// Copy the data.
		meshBuffer = create_buffer(
			static_cast<uint32_t>(elementSize), 
			static_cast<uint32_t>(accessor.count), 
			buffer.data, 
			static_cast<uint32_t>(stride), 
			static_cast<uint32_t>(bufferView.byteOffset));

		loaderData.accessorIdxToMeshVertexBuffers[accessorIndex] = meshBuffer;
	}
	
	return make_unique<MeshIndexBufferAccessor>(meshBuffer, format, static_cast<UINT>(accessor.count), static_cast<UINT>(elementSize), 0);
}

shared_ptr<MeshBuffer> create_buffer_s(size_t elementSize, size_t elementCount, const vector<uint8_t> &sourceData, size_t sourceStride, size_t sourceOffset) 
{
	assert(elementSize <= UINT32_MAX);
	assert(elementCount < UINT32_MAX);
	assert(sourceStride <= UINT32_MAX);
	assert(sourceOffset < UINT32_MAX);

	return create_buffer(
		static_cast<UINT>(elementSize),
		static_cast<UINT>(elementCount),
		sourceData,
		static_cast<UINT>(sourceStride),
		static_cast<UINT>(sourceOffset)
	);
}

shared_ptr<MeshBuffer> create_buffer(UINT elementSize, UINT elementCount, const vector<uint8_t> &sourceData, UINT sourceStride, UINT sourceOffset)
{
	assert(sourceOffset <= sourceData.size());

	UINT byteWidth = elementCount * elementSize;
	assert(sourceOffset + byteWidth <= sourceData.size());

	shared_ptr<MeshBuffer> meshBuffer = make_shared<MeshBuffer>(byteWidth);
	{
		uint8_t *dest = meshBuffer->m_data;
		const uint8_t *src = sourceData.data() + sourceOffset;

		if (sourceStride == elementSize) {
			memcpy_s(dest, byteWidth, src, byteWidth);
		}
		else {
			size_t destSize = byteWidth;
			for (UINT i = 0; i < elementCount; ++i) {
				memcpy_s(dest, byteWidth, src, elementSize);
				dest += elementSize;
				destSize -= elementSize;
				src += sourceStride;
			}
		}
	}

	return meshBuffer;
}
