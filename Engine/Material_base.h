#pragma once
#include "Material.h"
#include "Utils.h"

namespace oe {
	struct Vertex_constant_buffer_empty {};
	struct Vertex_constant_buffer_base {
		DirectX::SimpleMath::Matrix worldViewProjection;		
	};
	
	struct Pixel_constant_buffer_empty {};
	using Pixel_constant_buffer_base = Pixel_constant_buffer_empty;

	template<class TVertex_shader_constants, class TPixel_shader_constants, Vertex_attribute... TVertex_attr>
	class Material_base : public Material {
	public:
		explicit Material_base(
            uint8_t materialTypeIndex,
			Material_alpha_mode alphaMode = Material_alpha_mode::Opaque,
			Material_face_cull_mode faceCullMode = Material_face_cull_mode::Back_Face
		)
			: Material(materialTypeIndex, alphaMode, faceCullMode)
		{
			static_assert(std::is_same_v<TVertex_shader_constants, Vertex_constant_buffer_empty> || sizeof(TVertex_shader_constants) % 16 == 0);
			static_assert(std::is_same_v<TPixel_shader_constants, Pixel_constant_buffer_empty> || sizeof(TPixel_shader_constants) % 16 == 0);
		}

        std::shared_ptr<D3D_buffer> createVSConstantBuffer(ID3D11Device* device) const override
		{
			// If the constant buffer doesn't actually have any fields, don't bother creating one.
			if constexpr (std::is_same_v<TVertex_shader_constants, Vertex_constant_buffer_empty>) { 
			    return nullptr; 
			}

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(TVertex_shader_constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
            
            TVertex_shader_constants constantsVs;
			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &constantsVs;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

            ID3D11Buffer* buffer;
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

			auto name = materialType() + " Vertex CB";
			DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

			return std::make_shared<D3D_buffer>(buffer, bufferDesc.ByteWidth);
		}

        std::shared_ptr<D3D_buffer> createPSConstantBuffer(ID3D11Device* device) const override
		{
			// If the constant buffer doesn't actually have any fields, don't bother creating one.
			if constexpr (std::is_same_v<TPixel_shader_constants, Pixel_constant_buffer_empty>) { 
				return nullptr; 
			}

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(TPixel_shader_constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

            TPixel_shader_constants constantsPs;
			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &constantsPs;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

            ID3D11Buffer* buffer;
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

			auto name = materialType() + " Pixel CB";
			DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

            return std::make_shared<D3D_buffer>(buffer, bufferDesc.ByteWidth);
		}

        std::vector<Vertex_attribute_semantic> vertexInputs(const std::set<std::string>& flags) const override
		{
            std::vector<Vertex_attribute_semantic> vsInputs;
            if constexpr (sizeof...(TVertex_attr) > 0) {
                addVertexAttribute<TVertex_attr...>(vsInputs);
            }
            return vsInputs;
		}

		template<Vertex_attribute TThis, Vertex_attribute... TNext>
		void addVertexAttribute(std::vector<Vertex_attribute_semantic>& vertexAttributes) const
		{
            vertexAttributes.push_back({ TThis, 0 });

			if constexpr (sizeof...(TNext) > 0) {
				addVertexAttribute<TNext...>(vertexAttributes);
			}
		}

		void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
            const Renderer_animation_data& rendererAnimationData,
			ID3D11DeviceContext* context,
            D3D_buffer& buffer) const override final
		{
            TVertex_shader_constants constantsVs;
			if constexpr (std::is_assignable_v<Vertex_constant_buffer_base, TVertex_shader_constants>) {
				// Note that HLSL matrices are Column Major (as opposed to Row Major in DirectXMath) - so we need to transpose everything.
                constantsVs.worldViewProjection = XMMatrixMultiplyTranspose(worldMatrix, XMMatrixMultiply(viewMatrix, projMatrix));
			}
			updateVSConstantBufferValues(constantsVs, worldMatrix, viewMatrix, projMatrix, rendererAnimationData);

			context->UpdateSubresource(buffer.d3dBuffer, 0, nullptr, &constantsVs, 0, 0);
		}

		void updatePSConstantBuffer(
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context,
            D3D_buffer& buffer) const override final
		{
            TPixel_shader_constants constantsPs;
			updatePSConstantBufferValues(constantsPs, worldMatrix, viewMatrix, projMatrix);

			context->UpdateSubresource(buffer.d3dBuffer, 0, nullptr, &constantsPs, 0, 0);
		}
				
	protected:

		Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override
		{
			std::wstringstream ss;
			ss << shaderPath() << utf8_decode(materialType()) << L"_VS.hlsl";
			auto settings = Material::vertexShaderSettings(flags);
			settings.filename = ss.str();
			return settings;
		}

		Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override
		{
			std::wstringstream ss;
			ss << shaderPath() << utf8_decode(materialType()) << L"_PS.hlsl";
			auto settings = Material::pixelShaderSettings(flags);
			settings.filename = ss.str();
			return settings;
		}

		virtual void updateVSConstantBufferValues(TVertex_shader_constants& constants,
			const DirectX::SimpleMath::Matrix& matrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
            const Renderer_animation_data& rendererAnimationData) const
		{};

		virtual void updatePSConstantBufferValues(TPixel_shader_constants& constants,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix) const
		{};
    };
}
