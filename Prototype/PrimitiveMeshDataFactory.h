#pragma once

#include "SimpleTypes.h"

namespace OE {
	class MeshData;

	class PrimitiveMeshDataFactory
	{
	public:
		PrimitiveMeshDataFactory();
		~PrimitiveMeshDataFactory();

		std::shared_ptr<MeshData> createQuad(float width, float height) const;
		std::shared_ptr<MeshData> createQuad(const Rect &rect) const;
	};
}
