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

		bool createConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) override;
		void updateConstantBuffer(const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &viewMatrix,
			const DirectX::XMMATRIX &projMatrix, ID3D11DeviceContext *context, ID3D11Buffer *buffer) override;
		void setContextSamplers(const DX::DeviceResources &deviceResources) override;
	};
}
