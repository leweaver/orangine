#pragma once
#include "DeviceResources.h"
#include "MeshDataComponent.h"

#include "SimpleMath.h"
#include <string>
#include <set>

namespace OE {
	class RendererData;
	class Texture;

	enum class MaterialAlphaMode
	{
		OPAQUE,
		MASK,
		BLEND
	};

	class Material
	{
		ID3D11VertexShader *m_vertexShader;
		ID3D11PixelShader  *m_pixelShader;
		ID3D11InputLayout  *m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_vsConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_psConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
		bool m_errorState;
		bool m_requiresRecompile;

		MaterialAlphaMode m_alphaMode;

	public:
		Material();
		
		Material(const Material& other) = delete;
		Material(Material&& other) = delete;
		void operator=(const Material& other) = delete;
		void operator=(Material&& other) = delete;

		virtual ~Material();

		/**
		 * Populates the given array (assumed to be empty) with the vertex attributes that this material requires. Populated in-order.
		 */
		virtual void getVertexAttributes(std::vector<VertexAttribute> &vertexAttributes) const;
		bool requiresRecompile() const { return m_requiresRecompile; }

		void release();

		// Binds material textures on to the GPU
		bool render(const RendererData &rendererData, const DirectX::SimpleMath::Matrix &worldMatrix, 
			const DirectX::SimpleMath::Matrix &viewMatrix, 
			const DirectX::SimpleMath::Matrix &projMatrix, 
			const DX::DeviceResources& deviceResources);

		// Unbinds textures
		void unbind(const DX::DeviceResources& deviceResources);

		MaterialAlphaMode getAlphaMode() const
		{
			return m_alphaMode;
		}

		void setAlphaMode(MaterialAlphaMode mode)
		{
			m_alphaMode = mode;
		}

	protected:
		struct ShaderCompileSettings
		{
			std::wstring filename;
			std::string entryPoint;
			std::map<std::string, std::string> defines;
			std::set<std::string> includes;
		};

		static std::string Material::createShaderError(HRESULT hr, ID3D10Blob* errorMessage, const ShaderCompileSettings &compileSettings);
		
		virtual DXGI_FORMAT format(VertexAttribute attribute);
		virtual UINT inputSlot(VertexAttribute attribute);

		void markRequiresRecomplie() { m_requiresRecompile = true; }
		
		/*
		 * Initialization
		 */ 
		virtual ShaderCompileSettings vertexShaderSettings() const;
		virtual ShaderCompileSettings pixelShaderSettings() const;
		bool createVertexShader(ID3D11Device *device);
		bool createPixelShader(ID3D11Device *device);
		
		virtual bool createVSConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) { return true; }
		virtual void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix &worldMatrix, 
			const DirectX::SimpleMath::Matrix &viewMatrix,
			const DirectX::SimpleMath::Matrix &projMatrix, 
			ID3D11DeviceContext *context, 
			ID3D11Buffer *buffer) {};
		virtual void createBlendState(ID3D11Device *device, ID3D11BlendState *&blendState);

		virtual bool createPSConstantBuffer(ID3D11Device *device, ID3D11Buffer *&buffer) { return true; }
		virtual void updatePSConstantBuffer(const DirectX::SimpleMath::Matrix &worldMatrix, 
			const DirectX::SimpleMath::Matrix &viewMatrix,
			const DirectX::SimpleMath::Matrix &projMatrix, 
			ID3D11DeviceContext *context, 
			ID3D11Buffer *buffer) {};

		// This method is the entry point for generating the shader. It will determine the constant layout, 
		// create the shader resource view & sampler arrays.
		virtual void createShaderResources(const DX::DeviceResources &deviceResources) {};

		/* 
		 * Per Frame
		 */ 
		virtual void setContextSamplers(const DX::DeviceResources &deviceResources) {}
		virtual void unsetContextSamplers(const DX::DeviceResources &deviceResources) {}

		bool ensureSamplerState(const DX::DeviceResources &deviceResources, Texture &texture, ID3D11SamplerState **d3D11SamplerState);
	};

}
