#include "pch.h"
#include "PrimitiveMeshDataFactory.h"
#include "MeshData.h"
#include "DirectXTexP.h"

using namespace OE;

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

std::shared_ptr<MeshData> PrimitiveMeshDataFactory::createQuad(float width, float height) const
{
	const float halfWidth = width * 0.5f;
	const float halfHeight = height * 0.5f;
	return createQuad(Rect(-halfWidth, -halfHeight, halfWidth, halfHeight));
}

std::shared_ptr<MeshData> PrimitiveMeshDataFactory::createQuad(const Rect &rect) const
{
	auto md = std::make_shared<MeshData>();

	{
		float right = rect.left + rect.width,
			top = rect.bottom + rect.height;
		const std::vector<Vertex> vertices = {
			{ rect.left, top,         0 },
			{ right,     top,         0 },
			{ rect.left, rect.bottom, 0 },
			{ right,     rect.bottom, 0 }
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
