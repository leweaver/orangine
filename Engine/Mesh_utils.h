#pragma once

#include <dxgiformat.h>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <vector>

namespace oe {
	struct Mesh_buffer;
	class Entity_filter;
	class Entity;
}

namespace oe::mesh_utils {
	inline uint32_t convertIndexValue(DXGI_FORMAT srcFormat, const std::uint8_t* srcIndexBuffer)
	{
		switch (srcFormat)
		{
		case DXGI_FORMAT_R8_UINT:
			return static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(srcIndexBuffer));
		case DXGI_FORMAT_R16_UINT:
			return static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(srcIndexBuffer));
		case DXGI_FORMAT_R32_UINT:
			return static_cast<uint32_t>(*reinterpret_cast<const uint32_t*>(srcIndexBuffer));
		case DXGI_FORMAT_R8_SINT:
		{
			auto* signedvalue = reinterpret_cast<const int8_t*>(srcIndexBuffer);
			assert(*signedvalue >= 0);
			return static_cast<uint32_t>(*signedvalue);
		}
		case DXGI_FORMAT_R16_SINT:
		{
			auto* signedvalue = reinterpret_cast<const int16_t*>(srcIndexBuffer);
			assert(*signedvalue >= 0);
			return static_cast<uint32_t>(*signedvalue);
		}
		case DXGI_FORMAT_R32_SINT:
		{
			auto* signedvalue = reinterpret_cast<const int32_t*>(srcIndexBuffer);
			assert(*signedvalue >= 0);
			return static_cast<uint32_t>(*signedvalue);
		}
		default:
			throw std::runtime_error("convertIndexValue requires a source with a valid index buffer format (8/16/32 bit SINT/UINT).");
		}
	}
	
	std::shared_ptr<Mesh_buffer> create_buffer(UINT elementSize, UINT elementCount, const std::vector<uint8_t>& sourceData, UINT sourceStride, UINT sourceOffset);
	std::shared_ptr<Mesh_buffer> create_buffer_s(size_t elementSize, size_t elementCount, const std::vector<uint8_t>& sourceData, size_t sourceStride, size_t sourceOffset);
	std::tuple<std::shared_ptr<Mesh_buffer>, DXGI_FORMAT> create_index_buffer(const std::vector<uint8_t>& sourceData, UINT srcElementCount, DXGI_FORMAT srcFormat, UINT sourceStride, UINT sourceOffset);

	DirectX::BoundingOrientedBox aabbForEntities(const Entity_filter& entities,
		const DirectX::SimpleMath::Quaternion& orientation,
		std::function<bool(const Entity&)> predicate);
}