#pragma once
#include "Material.h"

namespace OE {
	class ClearGBufferMaterial : public Material
	{
	public:

		void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const override;

	protected:
		UINT inputSlot(VertexAttribute attribute) override;

		ShaderCompileSettings vertexShaderSettings() const override;
		ShaderCompileSettings pixelShaderSettings() const override;
	};
}
