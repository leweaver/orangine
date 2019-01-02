#pragma once

#include "Mesh_data_component.h"
#include "Renderer_enum.h"

#include "DeviceResources.h"
#include <SimpleMath.h>
#include <string>
#include <set>

namespace oe {
	class Render_light_data;
	struct Renderer_data;
	class Texture;
	
	class Material
	{
	public:
		Material(Material_alpha_mode alphaMode, Material_face_cull_mode faceCullMode);

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

		// Should be called prior to deletion - releases all resources.
		void release();

		// Compiles (if needed), binds pixel and vertex shaders, and textures
		void bind(Render_pass_blend_mode blendMode,
			const Render_light_data& renderLightData,
			const DX::DeviceResources& deviceResources,
			bool enablePixelShader);

		// Uploads shader constants, then renders
		bool render(const Renderer_data& rendererData,
			const Render_light_data& renderLightData, 
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			const DX::DeviceResources& deviceResources);

		// Unbinds textures
		void unbind(const DX::DeviceResources& deviceResources);

		Material_alpha_mode getAlphaMode() const { return _alphaMode; }
		void setAlphaMode(Material_alpha_mode mode) { 
			_alphaMode = mode;
			markRequiresRecomplie();
		}

		virtual Material_light_mode lightMode() { return Material_light_mode::Unlit; }
		virtual const std::string& materialType() const = 0;

		Material_face_cull_mode faceCullMode() { return _faceCullMode; }

	protected:

		struct Shader_compile_settings {
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
		
		virtual bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) = 0;
		virtual void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, 
			ID3D11DeviceContext* context, 
			ID3D11Buffer* buffer) {};

		virtual bool createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) = 0;
		virtual void updatePSConstantBuffer(const Render_light_data& renderlightData, const DirectX::SimpleMath::Matrix& worldMatrix, 
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix, 
			ID3D11DeviceContext* context, 
			ID3D11Buffer* buffer) {};

		// This method is the entry point for generating the shader. It will determine the constant layout, 
		// create the shader resource view & sampler arrays.
		virtual void createShaderResources(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData, Render_pass_blend_mode blendMode) = 0;
		
		/* 
		 * Per Frame
		 */ 
		virtual void setContextSamplers(const DX::DeviceResources& deviceResources, const Render_light_data& renderLightData) {}
		virtual void unsetContextSamplers(const DX::DeviceResources& deviceResources) {}

		bool ensureSamplerState(const DX::DeviceResources& deviceResources, 
			Texture& texture, 
			D3D11_TEXTURE_ADDRESS_MODE textureAddressMode, 
			ID3D11SamplerState** d3D11SamplerState);

		static std::wstring_view shaderPath() { return shader_path; }

	private:
		ID3D11VertexShader* _vertexShader;
		ID3D11PixelShader*  _pixelShader;
		ID3D11InputLayout*  _inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _vsConstantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _psConstantBuffer;
		bool _errorState;
		bool _requiresRecompile;

		Material_alpha_mode _alphaMode;
		Material_face_cull_mode _faceCullMode;

		static const std::wstring shader_path;
	};

}
