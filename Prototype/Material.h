#pragma once
#include "DeviceResources.h"
#include "MeshDataComponent.h"

#include <string>
#include <set>

namespace OE {
	class RendererData;
	class Texture;

	class Material
	{
		ID3D11VertexShader *m_vertexShader;
		ID3D11PixelShader  *m_pixelShader;
		ID3D11InputLayout  *m_inputLayout;
		ID3D11Buffer       *m_constantBuffer;
		bool m_errorState;

	public:
		Material();
		virtual ~Material();

		/**
		 * Populates the given array (assumed to be empty) with the vertex attributes that this material requires. Populated in-order.
		 */
		virtual void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const;

		void release();

		// Binds material textures on to the GPU
		bool render(const RendererData &rendererData, const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &viewMatrix, const DirectX::XMMATRIX &projMatrix, const DX::DeviceResources& deviceResources);

		// Unbinds textures
		void unbind(const DX::DeviceResources& deviceResources);

	protected:
		struct ShaderCompileSettings
		{
			std::wstring filename;
			std::string entryPoint;
			std::set<std::string> defines;
			std::set<std::string> includes;
		};

		static void throwShaderError(HRESULT hr, ID3D10Blob *errorMessage, const ShaderCompileSettings &compileSettings);
		
		virtual DXGI_FORMAT format(VertexAttribute attribute);
		virtual UINT inputSlot(VertexAttribute attribute);
		
		virtual ShaderCompileSettings vertexShaderSettings() const;
		virtual ShaderCompileSettings pixelShaderSettings() const;
		bool createVertexShader(ID3D11Device *device);
		bool createPixelShader(ID3D11Device *device);
		
		virtual bool createConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) = 0;
		virtual void updateConstantBuffer(const DirectX::XMMATRIX &worldMatrix, const DirectX::XMMATRIX &viewMatrix,
			const DirectX::XMMATRIX &projMatrix, ID3D11DeviceContext *context, ID3D11Buffer *buffer) = 0;

		virtual void setContextSamplers(const DX::DeviceResources &deviceResources) = 0;
		virtual void unsetContextSamplers(const DX::DeviceResources &deviceResources) {}

		bool ensureSamplerState(const DX::DeviceResources &deviceResources, Texture &texture, ID3D11SamplerState **d3D11SamplerState);
	};

}
