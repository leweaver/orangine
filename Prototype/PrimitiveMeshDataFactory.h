#pragma once
#include "MeshData.h"

namespace OE {
	class MeshData;
	class MeshIndexBufferAccessor;
	class MeshVertexBufferAccessor;

	class PrimitiveMeshDataFactory
	{
	public:
		PrimitiveMeshDataFactory();
		~PrimitiveMeshDataFactory();

		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size) const;
		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size, const DirectX::SimpleMath::Vector3 &positionOffset) const;

		std::unique_ptr<MeshVertexBufferAccessor> generateNormalBuffer(const MeshIndexBufferAccessor &indexBufferAccessor, const MeshVertexBufferAccessor &vertexBufferAccessor) const;
		std::unique_ptr<MeshVertexBufferAccessor> generateTangentBuffer(const MeshIndexBufferAccessor &indexBufferAccessor, const MeshVertexBufferAccessor &vertexBufferAccessor) const;
	};
}
