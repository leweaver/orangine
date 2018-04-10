#pragma once
#include "MeshData.h"
#include "SimpleMath.h"

namespace OE {
	class MeshData;
	struct MeshIndexBufferAccessor;
	struct MeshVertexBufferAccessor;

	class PrimitiveMeshDataFactory
	{
	public:
		PrimitiveMeshDataFactory();
		~PrimitiveMeshDataFactory();

		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size) const;
		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size, const DirectX::SimpleMath::Vector3 &positionOffset) const;

		void generateNormals(const MeshIndexBufferAccessor &indexBufferAccessor,
			const MeshVertexBufferAccessor &positionBufferAccessor,
			MeshVertexBufferAccessor &normalBufferAccessor) const;

		void generateTangents(const MeshIndexBufferAccessor &indexBufferAccessor, 
			const MeshVertexBufferAccessor &positionBufferAccessor,
			MeshVertexBufferAccessor &tangentBufferAccessor) const;
	};
}
