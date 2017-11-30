#pragma once
#include "DeviceResources.h"

namespace OE {
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

		void Release();
		bool Render(const DX::DeviceResources& deviceResources);

	protected:
		static void ThrowShaderError(HRESULT hr, ID3D10Blob* errorMessage, const wchar_t* shaderFilename);
	};

}
