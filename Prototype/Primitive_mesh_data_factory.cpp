#include "pch.h"
#include "Primitive_mesh_data_factory.h"
#include "Mesh_data.h"
#include "SimpleMath.h"
#include "Mikk_tspace_triangle_mesh_interface.h"

using namespace oe;
using namespace DirectX::SimpleMath;

struct Vertex
{
	float x, y, z;
};

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createQuad(const Vector2 &size) const
{
	return createQuad(size, Vector3(-size.x * 0.5f, -size.y * 0.5f, 0.0f));
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createQuad(const Vector2 &size, const Vector3 &positionOffset) const
{
	auto md = std::make_shared<Mesh_data>();

	{
		const auto 
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

		const auto srcSize = sizeof(Vertex) * vertices.size();
		assert(srcSize < INT32_MAX);

		auto meshBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcSize));
		memcpy_s(meshBuffer->data, meshBuffer->dataSize, vertices.data(), srcSize);

		auto meshBufferAccessor = std::make_unique<Mesh_vertex_buffer_accessor>(meshBuffer, Vertex_attribute::Position, 
			static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(sizeof(Vertex)), 0);

		md->vertexBufferAccessors[Vertex_attribute::Position] = move(meshBufferAccessor);
	}

	{
		const std::vector<uint16_t> indices = {
			0, 2, 1,
			1, 2, 3
		};

		const auto srcSize = sizeof(Vertex) * indices.size();
		assert(srcSize < INT32_MAX);

		auto meshBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcSize));
		memcpy_s(meshBuffer->data, meshBuffer->dataSize, indices.data(), srcSize);

		auto meshBufferAccessor = std::make_unique<Mesh_index_buffer_accessor>(meshBuffer, DXGI_FORMAT_R16_UINT, 
			static_cast<uint32_t>(indices.size()), static_cast<uint32_t>(sizeof(uint16_t)), 0);

		md->indexBufferAccessor = std::move(meshBufferAccessor);
	}

	return md;
}

void Primitive_mesh_data_factory::generateNormals(
	const Mesh_index_buffer_accessor &indexBufferAccessor, 
	const Mesh_vertex_buffer_accessor &positionBufferAccessor,
	Mesh_vertex_buffer_accessor &normalBufferAccessor) const
{
	if (indexBufferAccessor.count % 3 != 0)
		throw std::logic_error("Expected index buffer to have a count that is a multiple of 3.");

	if (positionBufferAccessor.attribute != Vertex_attribute::Position)
		throw std::runtime_error("Given vertex buffer accessor must have type VA_POSITION");

	if (normalBufferAccessor.attribute != Vertex_attribute::Normal)
		throw std::runtime_error("Given normal buffer accessor must have type VA_NORMAL");

	if (normalBufferAccessor.count != positionBufferAccessor.count)
		throw std::runtime_error("Given position and normal buffer accessors must have the same count");
	
	const auto indexBufferStart = indexBufferAccessor.buffer->data + indexBufferAccessor.offset;
	const auto positionBufferStart = positionBufferAccessor.buffer->data + positionBufferAccessor.offset;
	const auto normalBufferStart = normalBufferAccessor.buffer->data + normalBufferAccessor.offset;

	assert(normalBufferAccessor.stride >= sizeof(Vector3));
	const auto normalBufferEnd = normalBufferStart + normalBufferAccessor.count * normalBufferAccessor.stride + sizeof(Vector3);
		
	// Iterate over each triangle, creating a flat normal.
	if (indexBufferAccessor.format == DXGI_FORMAT_R16_UINT)
	{		
		assert(sizeof(uint16_t) <= indexBufferAccessor.stride);
		const uint16_t numVertices = positionBufferAccessor.count;

		for (uint32_t idx = 0; idx < indexBufferAccessor.count;)
		{
			const auto& i0 = *reinterpret_cast<const uint16_t*>(indexBufferStart + idx * indexBufferAccessor.stride);
			const auto& i1 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.stride);
			const auto& i2 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.stride);
			++idx;

			if (i0 > numVertices || i1 > numVertices || i2 > numVertices) {
				throw std::logic_error("IndexBuffer index near " + std::to_string(idx - 3) +
					" out of range of given vertexBuffer (numVertices=" + std::to_string(numVertices) + ")");
			}

			const auto& p0 = *reinterpret_cast<const Vector3*>(positionBufferStart + i0 * positionBufferAccessor.stride);
			const auto& p1 = *reinterpret_cast<const Vector3*>(positionBufferStart + i1 * positionBufferAccessor.stride);
			const auto& p2 = *reinterpret_cast<const Vector3*>(positionBufferStart + i2 * positionBufferAccessor.stride);

			auto normalBufferPos = normalBufferStart + i0 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto& n0 = *reinterpret_cast<Vector3*>(normalBufferPos);
			normalBufferPos = normalBufferStart + i1 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto& n1 = *reinterpret_cast<Vector3*>(normalBufferPos);
			normalBufferPos = normalBufferStart + i2 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto& n2 = *reinterpret_cast<Vector3*>(normalBufferPos);
			
			// Create two vectors from which we calculate the normal
			const auto v0 = p1 - p0;
			const auto v1 = p2 - p0;
			
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

void Primitive_mesh_data_factory::generateTangents(std::shared_ptr<Mesh_data> meshData) const
{
	Mikk_tspace_triangle_mesh_interface generator(meshData);
	SMikkTSpaceContext context;

	context.m_pUserData = generator.userData();
	context.m_pInterface = generator.getInterface();
	if (!genTangSpaceDefault(&context)) {
		LOG(WARNING) << "Failed to generate tangents!";
	}
}
