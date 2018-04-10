#include "pch.h"
#include "PrimitiveMeshDataFactory.h"
#include "MeshData.h"
#include "SimpleMath.h"

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

void PrimitiveMeshDataFactory::generateNormals(
	const MeshIndexBufferAccessor &indexBufferAccessor, 
	const MeshVertexBufferAccessor &positionBufferAccessor,
	MeshVertexBufferAccessor &normalBufferAccessor) const
{
	if (indexBufferAccessor.m_count % 3 != 0)
		throw std::logic_error("Expected index buffer to have a count that is a multiple of 3.");

	if (positionBufferAccessor.m_attribute != VertexAttribute::VA_POSITION)
		throw std::runtime_error("Given vertex buffer accessor must have type VA_POSITION");

	if (normalBufferAccessor.m_attribute != VertexAttribute::VA_NORMAL)
		throw std::runtime_error("Given normal buffer accessor must have type VA_NORMAL");

	if (normalBufferAccessor.m_count != positionBufferAccessor.m_count)
		throw std::runtime_error("Given position and normal buffer accessors must have the same count");
	
	const uint8_t *indexBufferStart = indexBufferAccessor.m_buffer->m_data + indexBufferAccessor.m_offset;
	const uint8_t *positionBufferStart = positionBufferAccessor.m_buffer->m_data + positionBufferAccessor.m_offset;
	uint8_t *normalBufferStart = normalBufferAccessor.m_buffer->m_data + normalBufferAccessor.m_offset;

	assert(normalBufferAccessor.m_stride >= sizeof(Vector3));
	uint8_t *normalBufferEnd = normalBufferStart + normalBufferAccessor.m_count * normalBufferAccessor.m_stride + sizeof(Vector3);
		
	// Iterate over each triangle, creating a flat normal.
	if (indexBufferAccessor.m_format == DXGI_FORMAT_R16_UINT)
	{		
		assert(sizeof(uint16_t) <= indexBufferAccessor.m_stride);
		const uint16_t numVertices = positionBufferAccessor.m_count;

		for (uint32_t idx = 0; idx < indexBufferAccessor.m_count;)
		{
			const uint16_t &i0 = *reinterpret_cast<const uint16_t*>(indexBufferStart + idx * indexBufferAccessor.m_stride);
			const uint16_t &i1 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.m_stride);
			const uint16_t &i2 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.m_stride);
			++idx;

			if (i0 > numVertices || i1 > numVertices || i2 > numVertices)
				throw std::logic_error("IndexBuffer index near "+to_string(idx - 3) + " out of range of given vertexBuffer (numVertices=" + to_string(numVertices) + ")");

			const Vector3 &p0 = *reinterpret_cast<const Vector3*>(positionBufferStart + i0 * positionBufferAccessor.m_stride);
			const Vector3 &p1 = *reinterpret_cast<const Vector3*>(positionBufferStart + i1 * positionBufferAccessor.m_stride);
			const Vector3 &p2 = *reinterpret_cast<const Vector3*>(positionBufferStart + i2 * positionBufferAccessor.m_stride);

			uint8_t *normalBufferPos = normalBufferStart + i0 * normalBufferAccessor.m_stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			Vector3 &n0 = *reinterpret_cast<Vector3*>(normalBufferPos);
			normalBufferPos = normalBufferStart + i1 * normalBufferAccessor.m_stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			Vector3 &n1 = *reinterpret_cast<Vector3*>(normalBufferPos);
			normalBufferPos = normalBufferStart + i2 * normalBufferAccessor.m_stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			Vector3 &n2 = *reinterpret_cast<Vector3*>(normalBufferPos);
			
			// Create two vectors from which we calculate the normal
			const Vector3 v0 = p1 - p0;
			const Vector3 v1 = p2 - p0;
			
			// Calculate flat normal. (Counter clockwise winding order)
			n0 = v0.Cross(v1);
			n0.Normalize();
			n1 = n2 = n0;

			/*
			 * Test values:
			 * p0 = 0, 0, 0
			 * p1 = 1, 0, 0
			 * p2 = 0, 0, -1
			 * v0 = 1, 0, 0
			 * v1 = 0, 0, -1
			 * n  = 0, 1, 0
			 */
		}
	}
}

void PrimitiveMeshDataFactory::generateTangents(const MeshIndexBufferAccessor &indexBufferAccessor, 
	const MeshVertexBufferAccessor &positionBufferAccessor, 
	MeshVertexBufferAccessor &tangentBufferAccessor) const
{
	throw std::runtime_error("Not implemented");
}
