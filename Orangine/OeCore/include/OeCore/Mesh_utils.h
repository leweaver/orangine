#pragma once

#include <dxgiformat.h>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <vector>

#include "Renderer_enum.h"

namespace oe {
	struct Mesh_buffer;
    struct Mesh_index_buffer_accessor;
	class Entity_filter;
	class Entity;
}

namespace oe::mesh_utils {
	inline uint32_t convert_index_value(Element_component srcComponent, const std::uint8_t* srcIndexBuffer)
	{
		switch (srcComponent)
		{
        case Element_component::Unsigned_Byte:
			return static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(srcIndexBuffer));
		case Element_component::Unsigned_Short:
			return static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(srcIndexBuffer));
		case Element_component::Unsigned_Int:
			return static_cast<uint32_t>(*reinterpret_cast<const uint32_t*>(srcIndexBuffer));
		case Element_component::Signed_Byte:
		{
			auto* signedValue = reinterpret_cast<const int8_t*>(srcIndexBuffer);
			assert(*signedValue >= 0);
			return static_cast<uint32_t>(*signedValue);
		}
		case Element_component::Signed_Short:
		{
			auto* signedValue = reinterpret_cast<const int16_t*>(srcIndexBuffer);
			assert(*signedValue >= 0);
			return static_cast<uint32_t>(*signedValue);
		}
		case Element_component::Signed_Int:
		{
			auto* signedValue = reinterpret_cast<const int32_t*>(srcIndexBuffer);
			assert(*signedValue >= 0);
			return static_cast<uint32_t>(*signedValue);
		}
		default:
			throw std::runtime_error("convertIndexValue requires a source with a valid index buffer format (8/16/32 bit SINT/UINT).");
		}
	}
	
	std::shared_ptr<Mesh_buffer> create_buffer(UINT elementSize, UINT elementCount, const std::vector<uint8_t>& sourceData, UINT sourceStride, UINT sourceOffset);
	std::shared_ptr<Mesh_buffer> create_buffer_s(size_t elementSize, size_t elementCount, const std::vector<uint8_t>& sourceData, size_t sourceStride, size_t sourceOffset);
    std::unique_ptr<Mesh_index_buffer_accessor> create_index_buffer(const std::vector<uint8_t>& sourceData, UINT srcElementCount, Element_component elementComponent, UINT sourceStride, UINT sourceOffset);
    DXGI_FORMAT getDxgiFormat(Element_type type, Element_component component);
    
	DirectX::BoundingOrientedBox aabbForEntities(const Entity_filter& entities,
		const DirectX::SimpleMath::Quaternion& orientation,
		std::function<bool(const Entity&)> predicate);
}