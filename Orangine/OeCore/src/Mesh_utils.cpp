#include "pch.h"

#include "OeCore/Mesh_utils.h"
#include "OeCore/Mesh_data.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/Entity.h"

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

	std::unique_ptr<Mesh_index_buffer_accessor> create_index_buffer(const std::vector<uint8_t>& sourceData, UINT srcElementCount, Element_component srcComponent, UINT sourceStride, UINT sourceOffset)
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
				dest[i] = convert_index_value(srcComponent, src);
				src += sourceStride;
			}
		}

        return std::make_unique<Mesh_index_buffer_accessor>(
            meshBuffer, 
            Element_component::Unsigned_Int, 
            srcElementCount, 
            static_cast<uint32_t>(dstElementSize),
            0);
	}

    const std::map<Element_type, std::map<Element_component, DXGI_FORMAT>> g_elementTypeComponent_dxgiFormat = {
        { Element_type::Scalar, {
            { Element_component::Unsigned_Short, DXGI_FORMAT_R16_UINT },
            { Element_component::Signed_Short, DXGI_FORMAT_R16_SINT },
            { Element_component::Unsigned_Int, DXGI_FORMAT_R32_UINT },
            { Element_component::Signed_Int, DXGI_FORMAT_R32_SINT },
            { Element_component::Float, DXGI_FORMAT_R32_FLOAT },
            }
        },
        { Element_type::Vector2, {
            { Element_component::Unsigned_Short, DXGI_FORMAT_R16G16_UINT },
            { Element_component::Signed_Short, DXGI_FORMAT_R16G16_SINT },
            { Element_component::Unsigned_Int, DXGI_FORMAT_R32G32_UINT },
            { Element_component::Signed_Int, DXGI_FORMAT_R32G32_SINT },
            { Element_component::Float, DXGI_FORMAT_R32G32_FLOAT },
            }
        },
        { Element_type::Vector3, {
            { Element_component::Unsigned_Int, DXGI_FORMAT_R32G32B32_UINT },
            { Element_component::Signed_Int, DXGI_FORMAT_R32G32B32_SINT },
            { Element_component::Float, DXGI_FORMAT_R32G32B32_FLOAT },
            }
        },
        { Element_type::Vector4, {
            { Element_component::Unsigned_Short, DXGI_FORMAT_R16G16B16A16_UINT },
            { Element_component::Signed_Short, DXGI_FORMAT_R16G16B16A16_SINT },
            { Element_component::Unsigned_Int, DXGI_FORMAT_R32G32B32A32_UINT },
            { Element_component::Signed_Int, DXGI_FORMAT_R32G32B32A32_SINT },
            { Element_component::Float, DXGI_FORMAT_R32G32B32A32_FLOAT },
            }
        }
    };

    DXGI_FORMAT getDxgiFormat(Element_type type, Element_component component)
    {
        const auto typePos = g_elementTypeComponent_dxgiFormat.find(type);
        if (typePos == g_elementTypeComponent_dxgiFormat.end())
            throw std::runtime_error("Cannot convert " + elementTypeToString(type) + " to DXGI");
        const auto& typeMap = typePos->second;

        const auto componentPos = typeMap.find(component);
        if (componentPos == typeMap.end())
            throw std::runtime_error("Cannot convert " + elementTypeToString(type) + "/" + elementComponentToString(component) + " to DXGI");

        return componentPos->second;
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