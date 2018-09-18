#pragma once

#include "Shadow_map_texture.h"

namespace oe
{
	class Render_light_data {
	public:
		ID3D11Buffer* buffer() const { return _constantBuffer.Get(); }		
		std::vector<ID3D11ShaderResourceView*> shadowMapShaderResourceViews() const { return _shadowMapSRVs; };

	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
		std::vector<ID3D11ShaderResourceView*> _shadowMapSRVs;
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

			_shadowMapSRVs.reserve(TMax_lights);
		}

		bool addPointLight(const DirectX::SimpleMath::Vector3& lightPosition, const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Point, lightPosition, encodeColor(color, intensity), Light_constants::SHADOW_MAP_DISABLED_INDEX });
		}
		bool addDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Directional, lightDirection, encodeColor(color, intensity), Light_constants::SHADOW_MAP_DISABLED_INDEX });
		}
		bool addDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, const DirectX::SimpleMath::Color& color, float intensity, const Shadow_map_texture& shadowMapTexture)
		{
			typename Light_constants::Light_entry lightEntry = { 
				Light_type::Directional, 
				lightDirection, 
				encodeColor(color, intensity), 
				static_cast<int32_t>(_shadowMapSRVs.size()),
				XMMatrixTranspose(shadowMapTexture.worldViewProjMatrix())
			};

			if (_lightConstants.addLight(std::move(lightEntry))) {
				auto shadowMap = shadowMapTexture.getShaderResourceView();
				_shadowMapSRVs.push_back(shadowMap);
				shadowMap->AddRef();
				return true;
			}
			return false;
		}
		bool addAmbientLight(const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Ambient, DirectX::SimpleMath::Vector3::Zero, encodeColor(color, intensity), Light_constants::SHADOW_MAP_DISABLED_INDEX });
		}
		void updateBuffer(ID3D11DeviceContext* context)
		{
			context->UpdateSubresource(_constantBuffer.Get(), 0, nullptr, &_lightConstants, 0, 0);
		}
		void clear()
		{
			_lightConstants.clear();
			for (const auto shadowMap : _shadowMapSRVs) {
				if (shadowMap)
					shadowMap->Release();
			}
			_shadowMapSRVs.clear();
		}
		bool empty() const
		{
			return _lightConstants.empty();
		}
		bool full() const
		{
			return _lightConstants.full();
		}

	private:

		class alignas(16) Light_constants {
		public:

			static constexpr int32_t SHADOW_MAP_DISABLED_INDEX = -1;

			// sizeof must be a multiple of 16 for the shader arrays to behave correctly
			struct Light_entry {
				Light_type type = Light_type::Directional;
				DirectX::SimpleMath::Vector3 lightPositionDirection;
				DirectX::SimpleMath::Vector3 intensifiedColor;
				int32_t shadowMapIndex = SHADOW_MAP_DISABLED_INDEX;
				DirectX::SimpleMath::Matrix shadowViewProjMatrix;
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
