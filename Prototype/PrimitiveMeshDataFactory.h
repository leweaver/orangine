#pragma once

namespace OE {
	class MeshData;

	class PrimitiveMeshDataFactory
	{
	public:
		PrimitiveMeshDataFactory();
		~PrimitiveMeshDataFactory();

		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size) const;
		std::shared_ptr<MeshData> createQuad(const DirectX::SimpleMath::Vector2 &size, const DirectX::SimpleMath::Vector3 &positionOffset) const;
	};
}
