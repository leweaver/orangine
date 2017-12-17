#include "pch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <sstream>

#include "Material.h"
#include "entityRepository.h"
#include "glTFMeshLoader.h"
#include "RenderableComponent.h"
#include "MaterialRepository.h"


using namespace tinygltf;
using namespace OE;

std::map<VertexAttribute, std::string> gltfMappingToAttributeMap = {
	{ VertexAttribute::VA_POSITION, "POSITION" },
	{ VertexAttribute::VA_NORMAL, "NORMAL" },
	{ VertexAttribute::VA_TANGENT, "TANGENT" },
	{ VertexAttribute::VA_COLOR, "COLOR" },
	{ VertexAttribute::VA_TEXCOORD_0, "TEXCOORD_0" }
};

struct LoaderData
{
	LoaderData(Model &model) : m_model(model) {}

	Model &m_model;
	std::map<size_t, std::shared_ptr<MeshBuffer>> m_accessorIdxToMeshVertexBuffers;
};

std::unique_ptr<MeshVertexBufferAccessor> ReadVertexBuffer(const Model& model,
	VertexAttribute vertexAttribute,
	const std::vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData);

std::unique_ptr<MeshIndexBufferAccessor> ReadIndexBuffer(const Model& model,
	const std::vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData);

std::shared_ptr<Entity> CreateEntity(const Node &node, EntityRepository &entityRepository, MaterialRepository &materialRepository, LoaderData &loaderData);
std::shared_ptr<MeshBuffer> CreateBuffer(size_t elementSize, size_t elementCount, const std::vector<std::uint8_t> &sourceData, size_t sourceStride, size_t sourceOffset);
std::shared_ptr<MeshBuffer> CreateBuffer(UINT elementSize, UINT elementCount, const std::vector<std::uint8_t> &sourceData, UINT sourceStride, UINT sourceOffset);


void glTFMeshLoader::GetSupportedFileExtensions(std::vector<std::string> &extensions) const
{
	extensions.push_back("gltf");
}

std::vector<std::shared_ptr<Entity>> glTFMeshLoader::LoadFile(const std::string& filename, EntityRepository &entityRepository, MaterialRepository &materialRepository) const
{
	std::vector<std::shared_ptr<Entity>> entities;
	Model model;
	TinyGLTF loader;
	std::string err;

	const bool ret = loader.LoadASCIIFromFile(&model, &err, filename);
	//bool ret = loader.LoadBinaryFromFile(&model, &err, argv[1]); // for binary glTF(.glb) 
	if (!err.empty()) {
		throw std::runtime_error(err);
	}

	if (!ret) {
		throw std::runtime_error("Failed to parse glTF: unknown error.");
	}

	if (model.defaultScene >= model.scenes.size() || model.defaultScene < 0)
		throw std::runtime_error("Failed to parse glTF: defaultScene points to an invalid scene index");

	const auto &scene = model.scenes[model.defaultScene];
	LoaderData loaderData(model);
	for (int nodeIdx : scene.nodes) 
	{
		const Node &rootNode = model.nodes.at(nodeIdx);
		entities.push_back(CreateEntity(rootNode, entityRepository, materialRepository, loaderData));
	}

	return entities;
}

std::unique_ptr<OE::Material> CreateMaterial(const Primitive& prim, MaterialRepository &materialRepository, LoaderData &loaderData)
{
	const tinygltf::Material &gltfMaterial = loaderData.m_model.materials.at(prim.material);
	auto material = materialRepository.Instantiate(gltfMaterial.name);

	// TODO: Material Parameters
	return material;
}

std::shared_ptr<Entity> CreateEntity(const Node &node, EntityRepository &entityRepository, MaterialRepository &materialRepository, LoaderData &loaderData)
{
	std::shared_ptr<Entity> rootEntity = entityRepository.Instantiate(node.name);

	if (node.mesh > -1) {
		const Mesh &mesh = loaderData.m_model.meshes.at(node.mesh);
		for (int primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
		{
			std::shared_ptr<Entity> primitiveEntity = entityRepository.Instantiate(node.name + " primitive " + std::to_string(primIdx));
			primitiveEntity->SetParent(*rootEntity.get());
			MeshDataComponent &meshData = primitiveEntity->AddComponent<MeshDataComponent>();

			try {
				const auto &prim = mesh.primitives.at(primIdx);

				std::unique_ptr<OE::Material> material = CreateMaterial(prim, materialRepository, loaderData);

				// Read Index
				try
				{
					auto accessor = ReadIndexBuffer(loaderData.m_model, prim.indices, loaderData);

					// TODO: either remove count from the accessor, or from the meshData struct.
					meshData.m_indexCount = accessor->m_count;
					meshData.m_indexBufferAccessor = move(accessor);
				}
				catch (const std::exception &e)
				{
					throw std::runtime_error(std::string("Error in index buffer: ") + e.what());
				}

				// First, look though the attributes and extract accessor information
				// Look for the things that the material requires
				std::vector<VertexAttribute> materialAttributes;
				material->GetVertexAttributes(materialAttributes);

				// First, we need to see how many vertices there are.
				{
					const std::string &gltfPositionAttributeName = gltfMappingToAttributeMap[VertexAttribute::VA_POSITION];
					const auto gltfAttributePos = prim.attributes.find(gltfPositionAttributeName);
					if (gltfAttributePos == prim.attributes.end())
						throw std::logic_error("Missing " + gltfPositionAttributeName + " attribute");

					size_t dataLen = loaderData.m_model.accessors.at(gltfAttributePos->second).count;
					assert(dataLen < UINT32_MAX);

					meshData.m_vertexCount = static_cast<UINT>(dataLen);
				}

				// TODO: We could consider interleaving things in a single buffer? Not sure if there is a benefit?

				for (const auto &requiredAttr : materialAttributes) {
					const auto &mappingPos = gltfMappingToAttributeMap.find(requiredAttr);
					if (mappingPos == gltfMappingToAttributeMap.end()) {
						throw std::logic_error("gltf loader does not support attribute: " + VertexAttributeMeta::str(requiredAttr));
					}


					const std::string &gltfAttributeName = mappingPos->second;
					const auto &primAttrPos = prim.attributes.find(gltfAttributeName);
					if (primAttrPos == prim.attributes.end()) {
						// Can we fill in this stream with something else?
						// if (normal) { generate normals }
						// if (tangent) { generate tangents }
						const size_t stride = VertexAttributeMeta::elementSize(requiredAttr);
						auto buffer = std::make_shared<MeshBuffer>(stride * meshData.m_vertexCount);
						std::memset(buffer->m_data, 0, buffer->m_dataSize);

						// TODO: Consider logging a warning that we defaulted values here.
						//throw std::logic_error("glTF Mesh does not have required attribute: " + gltfAttributeName);
						auto accessor = std::make_unique<MeshVertexBufferAccessor>(buffer, requiredAttr, meshData.m_vertexCount, static_cast<UINT>(stride), 0);
						meshData.m_vertexBufferAccessors.push_back(move(accessor));
					}
					else {
						try {
							std::unique_ptr<MeshVertexBufferAccessor> accessor = ReadVertexBuffer(loaderData.m_model, requiredAttr, primAttrPos->second, loaderData);
							meshData.m_vertexBufferAccessors.push_back(move(accessor));
						}
						catch (const std::exception &e)
						{
							throw std::runtime_error("Error in attribute " + gltfAttributeName + ": " + e.what());
						}
					}
				}

				// Add this component last, to make sure there wasn't an error loading!
				auto &renderableComponent = primitiveEntity->AddComponent<RenderableComponent>();
				renderableComponent.SetMaterial(material);
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error(std::string("Primitive[") + std::to_string(primIdx) + "] is malformed. (" + e.what() + ")");
			}
		}
	}

	for (auto childIdx : node.children)
	{
		const Node &childNode = loaderData.m_model.nodes.at(childIdx);
		auto childEntity = CreateEntity(childNode, entityRepository, materialRepository, loaderData);
		childEntity->SetParent(*rootEntity.get());
	}

	return rootEntity;
}

std::unique_ptr<MeshVertexBufferAccessor> ReadVertexBuffer(const Model& model,
	VertexAttribute vertexAttribute,
	const std::vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw std::domain_error("accessor[" + std::to_string(accessorIndex) + "] out of range.");
	const auto &accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= model.bufferViews.size())
		throw std::domain_error("bufferView[" + std::to_string(accessor.bufferView) + "] out of range.");
	const auto &bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.buffer >= model.buffers.size())
		throw std::domain_error("buffer[" + std::to_string(bufferView.buffer) + "] out of range.");
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
		throw std::runtime_error("Unknown accessor type: " + std::to_string(accessor.type));

	const size_t byteWidth = elementSize * accessor.count;
	if (byteWidth < bufferView.byteLength)
		throw std::runtime_error("required byteWidth (" + std::to_string(byteWidth) + ") is larger than bufferView byteLength (" + std::to_string(bufferView.byteLength));
	if (expectedElementSize != elementSize)
	{
		throw std::runtime_error("cannot process attribute " +
			gltfMappingToAttributeMap[vertexAttribute] +
			", element size must be " + std::to_string(expectedElementSize) +
			" but was " + std::to_string(elementSize) + ".");
	}

	const size_t stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;
	
	// Have we already created an identical buffer? If so, re-use it.
	std::shared_ptr<MeshBuffer> meshBuffer;
	const auto pos = loaderData.m_accessorIdxToMeshVertexBuffers.find(accessorIndex);

	if (pos != loaderData.m_accessorIdxToMeshVertexBuffers.end())
		meshBuffer = pos->second;
	else
	{
		// Copy the data.
		meshBuffer = CreateBuffer(elementSize, accessor.count, buffer.data, stride, bufferView.byteOffset);
		loaderData.m_accessorIdxToMeshVertexBuffers[accessorIndex] = meshBuffer;
	}

	return std::make_unique<MeshVertexBufferAccessor>(meshBuffer, vertexAttribute, static_cast<UINT>(accessor.count), static_cast<UINT>(elementSize), 0);
}

std::unique_ptr<MeshIndexBufferAccessor> ReadIndexBuffer(const Model& model,
	const std::vector<Accessor>::size_type accessorIndex,
	LoaderData &loaderData)
{
	if (accessorIndex >= model.accessors.size())
		throw std::domain_error("accessor[" + std::to_string(accessorIndex) + "] out of range.");
	const auto &accessor = model.accessors[accessorIndex];

	if (accessor.bufferView >= model.bufferViews.size())
		throw std::domain_error("bufferView[" + std::to_string(accessor.bufferView) + "] out of range.");
	const auto &bufferView = model.bufferViews.at(accessor.bufferView);

	if (bufferView.buffer >= model.buffers.size())
		throw std::domain_error("buffer[" + std::to_string(bufferView.buffer) + "] out of range.");
	const auto &buffer = model.buffers.at(bufferView.buffer);
	
	if (accessor.type != TINYGLTF_TYPE_SCALAR)
		throw std::runtime_error("Index buffer must be scalar");

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
		throw std::runtime_error("Unknown index buffer format: " + std::to_string(accessor.componentType));
	}

	const size_t elementSize = sizeof(UINT);
	const size_t byteWidth = elementSize * accessor.count;
	if (byteWidth < bufferView.byteLength)
		throw std::runtime_error("required byteWidth (" + std::to_string(byteWidth) + ") is larger than bufferView byteLength (" + std::to_string(bufferView.byteLength));
	
	// Copy the data.
	const size_t stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;	

	// Have we already created an identical buffer? If so, re-use it.
	std::shared_ptr<MeshBuffer> meshBuffer;
	const auto pos = loaderData.m_accessorIdxToMeshVertexBuffers.find(accessorIndex);

	if (pos != loaderData.m_accessorIdxToMeshVertexBuffers.end())
		meshBuffer = pos->second;
	else
	{
		// Copy the data.
		meshBuffer = CreateBuffer(elementSize, accessor.count, buffer.data, stride, bufferView.byteOffset);
		loaderData.m_accessorIdxToMeshVertexBuffers[accessorIndex] = meshBuffer;
	}
	
	return std::make_unique<MeshIndexBufferAccessor>(meshBuffer, format, static_cast<UINT>(accessor.count), static_cast<UINT>(elementSize), 0);
}

std::shared_ptr<MeshBuffer> CreateBuffer(size_t elementSize, size_t elementCount, const std::vector<std::uint8_t> &sourceData, size_t sourceStride, size_t sourceOffset) 
{
	assert(elementSize <= UINT32_MAX);
	assert(elementCount < UINT32_MAX);
	assert(sourceStride <= UINT32_MAX);
	assert(sourceOffset < UINT32_MAX);

	return CreateBuffer(
		static_cast<UINT>(elementSize),
		static_cast<UINT>(elementCount),
		sourceData,
		static_cast<UINT>(sourceStride),
		static_cast<UINT>(sourceOffset)
	);
}

std::shared_ptr<MeshBuffer> CreateBuffer(UINT elementSize, UINT elementCount, const std::vector<std::uint8_t> &sourceData, UINT sourceStride, UINT sourceOffset)
{
	assert(sourceOffset <= sourceData.size());

	UINT byteWidth = elementCount * elementSize;
	assert(sourceOffset + byteWidth <= sourceData.size());

	std::shared_ptr<MeshBuffer> meshBuffer = std::make_shared<MeshBuffer>(byteWidth);
	{
		std::uint8_t *dest = meshBuffer->m_data;
		const std::uint8_t *src = sourceData.data() + sourceOffset;
		
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
	/*
	// Validate the binding flags
	D3D11_BIND_FLAG bindFlags;
	if (bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)
		bindFlags = D3D11_BIND_INDEX_BUFFER;
	else if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER)
		bindFlags = D3D11_BIND_VERTEX_BUFFER;
	else {
		throw std::runtime_error("Unknown bufferView target. Must be either: " +
			std::to_string(TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) +
			" or " +
			std::to_string(TINYGLTF_TARGET_ARRAY_BUFFER));
	}

	if (meshBuffers.size() < bufferView.buffer + 1)
		meshBuffers.resize(bufferView.buffer + 1);

	if (meshBuffers[bufferView.buffer] == nullptr) {
		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // TODO: Dynamic if animation required on any bufferView?
		bufferDesc.ByteWidth = static_cast<unsigned int>(byteWidth);
		bufferDesc.BindFlags = bindFlags;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = buffer.data.data();
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;


		meshBuffers[bufferView.buffer] = std::make_shared<MeshBuffer>();
		meshBuffers[bufferView.buffer]->m_gltfbuffer = bufferView.buffer;
		meshBuffers[bufferView.buffer]->m_bufferDesc = bufferDesc;
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &meshBuffers[bufferView.buffer]->m_d3dBuffer);
		assert(!FAILED(hr));

	}

	auto vbAccessor = std::make_unique<MeshBufferAccessor>();
	vbAccessor->m_buffer = meshBuffers[bufferView.buffer];
	size_t stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;
	assert(stride < UINT32_MAX);

	vbAccessor->m_stride = static_cast<unsigned int>(stride);

	return vbAccessor;
}
*/