#pragma once
#include "DeviceResources.h"
#include "MeshDataComponent.h"

namespace OE {
	class RendererData;
	class Material
	{
	private:

		ID3D11VertexShader *m_vertexShader;
		ID3D11PixelShader  *m_pixelShader;
		ID3D11InputLayout  *m_inputLayout;
		ID3D11Buffer       *m_constantBuffer;
		bool m_errorState;

	public:
		Material();
		~Material();

		/**
		 * Populates the given array (assumed to be empty) with the vertex attributes that this material requires. Populated in-order.
		 */
		void GetVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const;

		void Release();
		bool Render(const RendererData &rendererData, const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &viewMatrix, const DirectX::XMMATRIX &projMatrix, const DX::DeviceResources& deviceResources);

	protected:
		static void ThrowShaderError(HRESULT hr, ID3D10Blob* errorMessage, const wchar_t* shaderFilename);
		
		virtual const DXGI_FORMAT Material::format(VertexAttribute attribute);
		virtual UINT Material::inputSlot(VertexAttribute attribute);
	};

}
