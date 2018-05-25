#pragma once
#include "DeviceResources.h"
#include "Mesh_data_component.h"

#include "SimpleMath.h"
#include <string>
#include <set>

namespace oe {
	struct Renderer_data;
	class Texture;

	enum class Material_alpha_mode
	{
		Opaque,
		Mask,
		Blend
	};

	class Material
	{
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
		virtual void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const;
		bool requiresRecompile() const { return _requiresRecompile; }

		void release();

		// Binds material textures on to the GPU
		bool render(const Renderer_data& rendererData, 
			const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix, 
			const DirectX::SimpleMath::Matrix& projMatrix, 
			const DX::DeviceResources& deviceResources);

		// Unbinds textures
		void unbind(const DX::DeviceResources& deviceResources);

		Material_alpha_mode getAlphaMode() const { return _alphaMode; }
		void setAlphaMode(Material_alpha_mode mode) { _alphaMode = mode; }

	protected:
		struct Shader_compile_settings
		{
			std::wstring filename;
			std::string entryPoint;
			std::map<std::string, std::string> defines;
			std::set<std::string> includes;
		};

		static std::string createShaderError(HRESULT hr, ID3D10Blob* errorMessage, const Shader_compile_settings& compileSettings);
		
		virtual DXGI_FORMAT format(Vertex_attribute attribute);
		virtual UINT inputSlot(Vertex_attribute attribute);

		void markRequiresRecomplie() { _requiresRecompile = true; }
		
		/*
		 * Initialization
		 */ 
		virtual Shader_compile_settings vertexShaderSettings() const;
		virtual Shader_compile_settings pixelShaderSettings() const;
		bool createVertexShader(ID3D11Device* device);
		bool createPixelShader(ID3D11Device* device);
		
		virtual bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) { return true; }
		virtual void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, 
			ID3D11DeviceContext* context, 
			ID3D11Buffer* buffer) {};
		virtual void createBlendState(ID3D11Device* device, ID3D11BlendState*& blendState);

		virtual bool createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) { return true; }
		virtual void updatePSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, 
			ID3D11DeviceContext* context, 
			ID3D11Buffer* buffer) {};

		// This method is the entry point for generating the shader. It will determine the constant layout, 
		// create the shader resource view & sampler arrays.
		virtual void createShaderResources(const DX::DeviceResources& deviceResources) {};

		/* 
		 * Per Frame
		 */ 
		virtual void setContextSamplers(const DX::DeviceResources& deviceResources) {}
		virtual void unsetContextSamplers(const DX::DeviceResources& deviceResources) {}

		bool ensureSamplerState(const DX::DeviceResources& deviceResources, Texture& texture, ID3D11SamplerState** d3D11SamplerState);

	private:
		ID3D11VertexShader* _vertexShader;
		ID3D11PixelShader*  _pixelShader;
		ID3D11InputLayout*  _inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _vsConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _psConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11BlendState> _blendState;
		bool _errorState;
		bool _requiresRecompile;

		Material_alpha_mode _alphaMode;
	};

}
