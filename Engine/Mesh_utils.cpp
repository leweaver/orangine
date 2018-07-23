#include "pch.h"

#include "Mesh_utils.h"
#include "Mesh_data.h"

namespace oe::mesh_utils {

	std::shared_ptr<Mesh_buffer> create_buffer(UINT elementSize, UINT elementCount, const std::vector<uint8_t>& sourceData, UINT sourceStride, UINT sourceOffset)
	{
		assert(sourceOffset <= sourceData.size());

		UINT byteWidth = elementCount * elementSize;
		assert(sourceOffset + byteWidth <= sourceData.size());

		auto meshBuffer = std::make_shared<Mesh_buffer>(byteWidth);
		{
			uint8_t *dest = meshBuffer->data;
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

	std::shared_ptr<Mesh_buffer> create_buffer_s(size_t elementSize, size_t elementCount, const std::vector<uint8_t>& sourceData, size_t sourceStride, size_t sourceOffset)
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

	std::tuple<std::shared_ptr<Mesh_buffer>, DXGI_FORMAT> create_index_buffer(const std::vector<uint8_t>& sourceData, UINT srcElementCount, DXGI_FORMAT srcFormat, UINT sourceStride, UINT sourceOffset)
	{
		assert(sourceOffset <= sourceData.size());

		constexpr auto dstElementSize = sizeof(uint32_t);
		UINT dstByteWidth = srcElementCount * dstElementSize;

		assert(sourceOffset + dstByteWidth <= sourceData.size());

		auto meshBuffer = std::make_shared<Mesh_buffer>(dstByteWidth);
		{
			auto *dest = reinterpret_cast<uint32_t*>(meshBuffer->data);
			const auto *src = sourceData.data() + sourceOffset;
			for (UINT i = 0; i < srcElementCount; ++i) {
				dest[i] = convertIndexValue(srcFormat, src);
				src += sourceStride;
			}
		}

		return { meshBuffer, DXGI_FORMAT_R32_UINT };
	}
}