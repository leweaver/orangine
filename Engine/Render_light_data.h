#pragma once

#include "Shadow_map_texture.h"

namespace oe
{
	class Render_light_data {
	public:
		ID3D11Buffer* buffer() const { return _constantBuffer.Get(); }

		ID3D11ShaderResourceView* environmentMapBrdfSRV() const
		{
			return _environmentIblMapBrdfSRV.Get();
		}
		ID3D11ShaderResourceView* environmentMapDiffuseSRV() const
		{
			return _environmentIblMapDiffuseSRV.Get();
		}
		ID3D11ShaderResourceView* environmentMapSpecularSRV() const
		{
			return _environmentIblMapSpecularSRV.Get();
		}
		ID3D11SamplerState* environmentMapSamplerState() const {
			return _environmentIblMapSamplerState.Get();
		}

	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _environmentIblMapBrdfSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _environmentIblMapDiffuseSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _environmentIblMapSpecularSRV;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _environmentIblMapSamplerState;
	};

	template<uint8_t TMax_lights>
	class Render_light_data_impl : public Render_light_data {
	public:

		enum class Light_type : int32_t
		{
			Directional,
			Point,
			Ambient
		};

		explicit Render_light_data_impl(ID3D11Device* device)
		{
			// DirectX constant buffers must be aligned to 16 byte boundaries (vector4)
			static_assert(sizeof(Light_constants) % 16 == 0);

			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(Light_constants);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA initData;
			initData.pSysMem = &_lightConstants;
			initData.SysMemPitch = 0;
			initData.SysMemSlicePitch = 0;

			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, &_constantBuffer));

			DirectX::SetDebugObjectName(_constantBuffer.Get(), L"Render_light_data constant buffer");
		}

		bool addPointLight(const DirectX::SimpleMath::Vector3& lightPosition, const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Point, lightPosition, encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		bool addDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Directional, lightDirection, encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		bool addDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, 
			const DirectX::SimpleMath::Color& color, 
			float intensity, 
			const Shadow_map_texture_array_slice& shadowMapTexture,
			float shadowMapDepth,
			float shadowMapBias)
		{
			typename Light_constants::Light_entry lightEntry = { 
				Light_type::Directional, 
				lightDirection, 
				encodeColor(color, intensity),
				static_cast<int32_t>(shadowMapTexture.arraySlice()),
				XMMatrixTranspose(shadowMapTexture.worldViewProjMatrix()),
				shadowMapDepth,
				shadowMapBias
			};

			if (_lightConstants.addLight(std::move(lightEntry))) {
				return true;
			}
			return false;
		}
		bool addAmbientLight(const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Ambient, DirectX::SimpleMath::Vector3::Zero, encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		void setEnvironmentIblMap(
			ID3D11ShaderResourceView* brdfSRV,
			ID3D11ShaderResourceView* diffuseSRV, 
			ID3D11ShaderResourceView* specularSRV, 
			ID3D11SamplerState* samplerState)
		{
			_environmentIblMapBrdfSRV = brdfSRV;
			_environmentIblMapDiffuseSRV = diffuseSRV;
			_environmentIblMapSpecularSRV = specularSRV;
			_environmentIblMapSamplerState = samplerState;
		}
		void updateBuffer(ID3D11DeviceContext* context)
		{
			context->UpdateSubresource(_constantBuffer.Get(), 0, nullptr, &_lightConstants, 0, 0);
		}
		void clear()
		{
			_lightConstants.clear();
		}
		bool empty() const
		{
			return _lightConstants.empty();
		}
		bool full() const
		{
			return _lightConstants.full();
		}
		static uint8_t maxLights() 
		{
			return TMax_lights;
		}

	private:

		class alignas(16) Light_constants {
		public:

			static constexpr int32_t shadow_map_disabled_index = -1;

			// sizeof must be a multiple of 16 for the shader arrays to behave correctly
			struct Light_entry {
				Light_type type = Light_type::Directional;
				DirectX::SimpleMath::Vector3 lightPositionDirection;
				DirectX::SimpleMath::Vector3 intensifiedColor;
				int32_t shadowMapIndex = shadow_map_disabled_index;
				DirectX::SimpleMath::Matrix shadowViewProjMatrix;
				float shadowMapDepth = 0.0f;
				float shadowMapBias = 0.0f;
				DirectX::SimpleMath::Vector2 notUsed;
			};

			Light_constants()
			{
				static_assert(sizeof(Light_entry) % 16 == 0);
				std::fill(_lights.begin(), _lights.end(), Light_entry());
			}

			bool addLight(Light_entry&& entry)
			{
				if (full())
					return false;

				_lights[_activeLights++] = entry;
				return true;
			}

			void clear()
			{
				_activeLights = 0;
			}

			bool empty() const
			{
				return _activeLights == 0;
			}

			bool full() const
			{
				return _activeLights == TMax_lights;
			}

		private:
			std::array<Light_entry, TMax_lights> _lights;
			int _activeLights = 0;
		};

		static DirectX::SimpleMath::Vector3 encodeColor(const DirectX::SimpleMath::Color& color, float intensity)
		{
			DirectX::SimpleMath::Vector3 dest;
			XMStoreFloat3(&dest, XMVectorScale(color, intensity));
			return dest;
		}

		Light_constants _lightConstants;
	};
}
