#include "pch.h"

#include "Mesh_utils.h"
#include "Mesh_data.h"
#include "Entity_filter.h"
#include "Entity.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace oe::mesh_utils {

	std::shared_ptr<Mesh_buffer> create_buffer(UINT elementSize, UINT elementCount, const std::vector<uint8_t>& sourceData, UINT sourceStride, UINT sourceOffset)
	{
		assert(sourceOffset <= sourceData.size());

		auto byteWidth = elementCount * elementSize;
		assert(sourceOffset + byteWidth <= sourceData.size());

		auto meshBuffer = std::make_shared<Mesh_buffer>(byteWidth);
		{
			auto* dest = meshBuffer->data;
			const auto* src = sourceData.data() + sourceOffset;

			if (sourceStride == elementSize) {
				memcpy_s(dest, byteWidth, src, byteWidth);
			}
			else {
				size_t destSize = byteWidth;
				for (auto i = 0u; i < elementCount; ++i) {
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
				dest[i] = convert_index_value(srcFormat, src);
				src += sourceStride;
			}
		}

		return { meshBuffer, DXGI_FORMAT_R32_UINT };
	}

	BoundingOrientedBox aabbForEntities(const Entity_filter& entities,
		const DirectX::SimpleMath::Quaternion& orientation,
		std::function<bool(const Entity&)> predicate)
	{
		// Extents, in light view space (as defined above)
		XMVECTOR minExtents = Vector3::Zero;
		XMVECTOR maxExtents = Vector3::Zero;

		auto firstExtentsCalc = true;
		const auto orientationMatrix = Matrix::CreateFromQuaternion(orientation);
		const auto orientationMatrixInv = XMMatrixInverse(nullptr, orientationMatrix);
		for (auto& entity : entities) {
			if (!predicate(*entity.get()))
				continue;

			const auto& boundSphere = entity->boundSphere();
            assert(boundSphere.Radius < INFINITY && boundSphere.Radius >= 0.0f);
			const auto boundWorldCenter = Vector3::Transform(boundSphere.Center, entity->worldTransform());
			const auto boundWorldEdge = Vector3::Transform(Vector3(boundSphere.Center) + Vector3(0, 0, boundSphere.Radius), entity->worldTransform());

			// Bounds, in light view space (as defined above)
			const auto boundCenter = Vector3::Transform(boundWorldCenter, orientationMatrixInv);
			const auto boundEdge = Vector3::Transform(boundWorldEdge, orientationMatrixInv);
			const auto boundRadius = XMVector3Length(XMVectorSubtract(boundEdge, boundCenter));

			if (firstExtentsCalc) {
				minExtents = XMVectorSubtract(boundCenter, boundRadius);
				maxExtents = XMVectorAdd(boundCenter, boundRadius);
				firstExtentsCalc = false;
			}
			else {
				minExtents = XMVectorMin(minExtents, XMVectorSubtract(boundCenter, boundRadius));
				maxExtents = XMVectorMax(maxExtents, XMVectorAdd(boundCenter, boundRadius));
			}
		}

		const auto halfVector = XMVectorSet(0.5f, 0.5f, 0.5f, 0.0f); // OPT: does the compiler optimize this?
		const auto center = Vector3::Transform(XMVectorMultiply(XMVectorAdd(maxExtents, minExtents), halfVector), orientationMatrix);
		const auto extents = XMVectorMultiply(XMVectorSubtract(maxExtents, minExtents), halfVector);
		return BoundingOrientedBox(Vector3(center), Vector3(extents), orientation);
	}
}
