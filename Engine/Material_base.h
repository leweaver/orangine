#pragma once
#include "Material.h"
#include "Utils.h"
#include <array>

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
		Material_base(
			Material_alpha_mode alphaMode = Material_alpha_mode::Opaque,
			Material_face_cull_mode faceCullMode = Material_face_cull_mode::Backface
		)
			: Material(alphaMode, faceCullMode)
		{
			static_assert(std::is_same_v<TVertex_shader_constants, Vertex_constant_buffer_empty> || sizeof(TVertex_shader_constants) % 16 == 0);
			static_assert(std::is_same_v<TPixel_shader_constants, Pixel_constant_buffer_empty> || sizeof(TPixel_shader_constants) % 16 == 0);
		}

		bool createVSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) override
		{
			// If the constant buffer doesn't actually have any fields, don't bother creating one.
			if constexpr (std::is_same_v<TVertex_shader_constants, Vertex_constant_buffer_empty>) { return true; }

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(TVertex_shader_constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &_constantsVs;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

			auto name = materialType() + " Vertex CB";
			DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

			return true;
		}

		bool createPSConstantBuffer(ID3D11Device* device, ID3D11Buffer*& buffer) override
		{
			// If the constant buffer doesn't actually have any fields, don't bother creating one.
			if constexpr (std::is_same_v<TPixel_shader_constants, Pixel_constant_buffer_empty>) { 
				return true; 
			}

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(TPixel_shader_constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &_constantsVs;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &buffer));

			auto name = materialType() + " Pixel CB";
			DX::ThrowIfFailed(buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str()));

			return true;
		}

		void vertexAttributes(std::vector<Vertex_attribute>& vertexAttributes) const override
		{
			if constexpr (sizeof...(TVertex_attr) > 0) {
				addVertexAttribute<TVertex_attr...>(vertexAttributes);
			}
		}

		template<Vertex_attribute TThis, Vertex_attribute... TNext>
		void addVertexAttribute(std::vector<Vertex_attribute>& vertexAttributes) const
		{
			vertexAttributes.push_back(TThis);

			if constexpr (sizeof...(TNext) > 0) {
				addVertexAttribute<TNext...>(vertexAttributes);
			}
		}

		void updateVSConstantBuffer(const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context,
			ID3D11Buffer* buffer) override final
		{
			if constexpr (std::is_assignable_v<Vertex_constant_buffer_base, TVertex_shader_constants>) {
				// Note that HLSL matrices are Column Major (as opposed to Row Major in DirectXMath) - so we need to transpose everything.
				_constantsVs.worldViewProjection = XMMatrixMultiplyTranspose(worldMatrix, XMMatrixMultiply(viewMatrix, projMatrix));
			}

			updateVSConstantBufferValues(_constantsVs, worldMatrix, viewMatrix, projMatrix);

			context->UpdateSubresource(buffer, 0, nullptr, &_constantsVs, 0, 0);
		}

		void updatePSConstantBuffer(const Render_light_data& renderlightData,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
			ID3D11DeviceContext* context,
			ID3D11Buffer* buffer) override final
		{
			updatePSConstantBufferValues(_constantsPs, renderlightData, worldMatrix, viewMatrix, projMatrix);

			context->UpdateSubresource(buffer, 0, nullptr, &_constantsPs, 0, 0);
		}
				
	protected:

		uint32_t inputSlot(Vertex_attribute attribute) override
		{
			return getMatchingIndex<TVertex_attr...>(attribute);
		}

		template<Vertex_attribute TThis, Vertex_attribute... TNext>
		uint32_t getMatchingIndex(Vertex_attribute vertexAttribute) const
		{
			constexpr auto index = sizeof...(TVertex_attr) - sizeof...(TNext) - 1;
			if (vertexAttribute == TThis) { return index; }

			if constexpr (sizeof...(TNext) > 0) {
				return getMatchingIndex<TNext...>(vertexAttribute);
			}
			else
			{
				std::stringstream ss;
				ss << materialType() << " does not support Vertex_attribute: " << Vertex_attribute_meta::semanticName(vertexAttribute);
				throw std::logic_error(ss.str());
			}
		}

		Shader_compile_settings vertexShaderSettings() const override
		{
			std::wstringstream ss;
			ss << shaderPath() << utf8_decode(materialType()) << L"_VS.hlsl";
			auto settings = Material::vertexShaderSettings();
			settings.filename = ss.str();
			return settings;
		}

		Shader_compile_settings pixelShaderSettings() const override
		{
			std::wstringstream ss;
			ss << shaderPath() << utf8_decode(materialType()) << L"_PS.hlsl";
			auto settings = Material::pixelShaderSettings();
			settings.filename = ss.str();
			return settings;
		}

		virtual void updateVSConstantBufferValues(TVertex_shader_constants& constants,
			const DirectX::SimpleMath::Matrix& matrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix) 
		{};

		virtual void updatePSConstantBufferValues(TPixel_shader_constants& constants,
			const Render_light_data& renderlightData,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix)
		{};

	private:
		TVertex_shader_constants _constantsVs;
		TPixel_shader_constants _constantsPs;

	};
}
