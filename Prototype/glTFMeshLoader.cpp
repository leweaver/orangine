#include "pch.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "glTFMeshLoader.h"
#include <any>
#include "DeviceResources.h"

using namespace tinygltf;
using namespace OE;

std::map<std::string, VertexAttribute> gltfAttributeMapping = {
	{ "POSITION", VA_POSITION },
	{ "NORMAL", VA_NORMAL },
	{ "TANGENT", VA_TANGENT },
	{ "TEXCOORD_0", VA_TEXCOORD_0 }
};

std::unique_ptr<VertexBufferAccessor> CreateD3DBuffer(const Model& model,
	const std::vector<Accessor>::size_type accessorIndex,
	std::vector<std::shared_ptr<VertexBuffer>> &vertexBuffers,
	const DX::DeviceResources &deviceResources);

void glTFMeshLoader::GetSupportedFileExtensions(std::vector<std::string> &extensions) const
{
	extensions.push_back("gltf");
}

std::unique_ptr<RendererData> glTFMeshLoader::LoadFile(const std::string& filename, const DX::DeviceResources &deviceResources) const
{
	Model model;
	TinyGLTF loader;
  	std::string err;
  
  	bool ret = loader.LoadASCIIFromFile(&model, &err, filename);
  	//bool ret = loader.LoadBinaryFromFile(&model, &err, argv[1]); // for binary glTF(.glb) 
  	if (!err.empty()) {
		throw std::runtime_error(err);
	}

	if (!ret) {
		throw std::runtime_error("Failed to parse glTF");
	}

	std::unique_ptr<RendererData> rendererData = std::make_unique<RendererData>();
	std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers;

	const auto &mesh = model.meshes[0];
	for (int primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
	{		
		try {
			const auto &prim = mesh.primitives.at(primIdx);

			// Read Index
			try 
			{
				// TODO: Yeuck.. need to make our structure have an accessor for indices too.
				throw std::exception("not implemented");
				//std::unique_ptr<VertexBufferAccessor> ibAccessor = CreateD3DBuffer(model, prim.indices, vertexBuffers, deviceResources);
			}
			catch (const std::exception &e)
			{
				throw std::runtime_error(std::string("Error in index buffer: ") + e.what());
			}

			// POSITION
			for (const auto &attr : prim.attributes) {
				const auto &mappingPos = gltfAttributeMapping.find(attr.first);
				if (mappingPos == gltfAttributeMapping.end())
					continue;

				try {
					rendererData->m_vertexBuffers.push_back(CreateD3DBuffer(model, attr.second, vertexBuffers, deviceResources));
				}
				catch (const std::exception &e)
				{
					throw std::runtime_error("Error in attribute " + mappingPos->first + ": " + e.what());
				}
			}
		} 
		catch (const std::exception &e)
		{
			throw std::runtime_error(std::string("Primitive[") + std::to_string(primIdx) + "] is malformed. (" + e.what() + ")");
		}

	}

	return rendererData;
}

std::unique_ptr<VertexBufferAccessor> CreateD3DBuffer(const Model& model, 
	const std::vector<Accessor>::size_type accessorIndex, 
	std::vector<std::shared_ptr<VertexBuffer>> &vertexBuffers,
	const DX::DeviceResources &deviceResources)
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
	
	// Validate the elementSize and buffer size
	size_t elementSize;
	if (accessor.type == TINYGLTF_TYPE_SCALAR)
		elementSize = sizeof(unsigned int);
	else if (accessor.type == TINYGLTF_TYPE_VEC2)
		elementSize = sizeof(float) * 2;
	else if (accessor.type == TINYGLTF_TYPE_VEC3)
		elementSize = sizeof(float) * 3;
	else if (accessor.type == TINYGLTF_TYPE_VEC3)
		elementSize = sizeof(float) * 4;
	else
		throw std::runtime_error("Unknown accessor type: " + std::to_string(accessor.type));
		
	const unsigned int byteWidth = elementSize * accessor.count;
	if (byteWidth <= bufferView.byteLength)
		throw std::runtime_error("required byteWidth ("+ std::to_string(byteWidth) +") is larger than bufferView byteLength (" + std::to_string(bufferView.byteLength));

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

	if (vertexBuffers.size() < bufferView.buffer)
		vertexBuffers.resize(bufferView.buffer + 1);

	if (vertexBuffers[bufferView.buffer] == nullptr) {
		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // TODO: Dynamic if animation required on any bufferView?
		bufferDesc.ByteWidth = byteWidth;
		bufferDesc.BindFlags = bindFlags;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = buffer.data.data();
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		ID3D11Buffer* d3dBuffer;
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &d3dBuffer);
		assert(!FAILED(hr));

		vertexBuffers[bufferView.buffer] = std::make_shared<VertexBuffer>(d3dBuffer);
	}

	auto vbAccessor = std::make_unique<VertexBufferAccessor>();
	vbAccessor->m_buffer = vertexBuffers[bufferView.buffer];
	vbAccessor->m_stride = bufferView.byteStride == 0 ? elementSize : bufferView.byteStride;

	return vbAccessor;
}