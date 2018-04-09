#include "pch.h"
#include "PrimitiveMeshDataFactory.h"
#include "MeshData.h"

using namespace OE;
using namespace DirectX::SimpleMath;

struct Vertex
{
	float x, y, z;
};

PrimitiveMeshDataFactory::PrimitiveMeshDataFactory()
{
}

PrimitiveMeshDataFactory::~PrimitiveMeshDataFactory()
{
}

std::shared_ptr<MeshData> PrimitiveMeshDataFactory::createQuad(const Vector2 &size) const
{
	return createQuad(size, Vector3(-size.x * 0.5f, -size.y * 0.5f, 0.0f));
}

std::shared_ptr<MeshData> PrimitiveMeshDataFactory::createQuad(const Vector2 &size, const Vector3 &positionOffset) const
{
	auto md = std::make_shared<MeshData>();

	{
		const float 
			top    = positionOffset.y + size.y,
			right  = positionOffset.x + size.x,
			bottom = positionOffset.y,
			left   = positionOffset.x;
		const std::vector<Vertex> vertices = {
			{ left,  top,    positionOffset.z },
			{ right, top,    positionOffset.z },
			{ left,  bottom, positionOffset.z },
			{ right, bottom, positionOffset.z }
		};

		size_t srcSize = sizeof(Vertex) * vertices.size();
		assert(srcSize < INT32_MAX);

		auto meshBuffer = std::make_shared<MeshBuffer>(static_cast<int>(srcSize));
		memcpy_s(meshBuffer->m_data, meshBuffer->m_dataSize, vertices.data(), srcSize);

		auto meshBufferAccessor = std::make_unique<MeshVertexBufferAccessor>(meshBuffer, VertexAttribute::VA_POSITION, 
			static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(sizeof(Vertex)), 0);

		md->m_vertexBufferAccessors[VertexAttribute::VA_POSITION] = move(meshBufferAccessor);
	}

	{
		const std::vector<uint16_t> indices = {
			0, 2, 1,
			1, 2, 3
		};

		size_t srcSize = sizeof(Vertex) * indices.size();
		assert(srcSize < INT32_MAX);

		auto meshBuffer = std::make_shared<MeshBuffer>(static_cast<int>(srcSize));
		memcpy_s(meshBuffer->m_data, meshBuffer->m_dataSize, indices.data(), srcSize);

		auto meshBufferAccessor = std::make_unique<MeshIndexBufferAccessor>(meshBuffer, DXGI_FORMAT_R16_UINT, 
			static_cast<uint32_t>(indices.size()), static_cast<uint32_t>(sizeof(uint16_t)), 0);

		md->m_indexBufferAccessor = std::move(meshBufferAccessor);
	}

	return md;
}

std::unique_ptr<MeshVertexBufferAccessor> PrimitiveMeshDataFactory::generateNormalBuffer(
	const MeshIndexBufferAccessor &indexBufferAccessor, const MeshVertexBufferAccessor &vertexBufferAccessor) const
{
	if (indexBufferAccessor.m_count % 3 != 0)
		throw std::logic_error("Expected index buffer to have a count that is a multiple of 3.");
	
	// Iterate over each triangle, creating a flat normal.
	indexBufferAccessor.m_buffer->m_data
	for (int idx = 0; idx < indexBufferAccessor.m_count; ++idx)
	{
		indexBufferAccessor
	}
}

std::unique_ptr<MeshVertexBufferAccessor> PrimitiveMeshDataFactory::generateTangentBuffer(
	const MeshIndexBufferAccessor &indexBufferAccessor, const MeshVertexBufferAccessor &vertexBufferAccessor) const
{
}
