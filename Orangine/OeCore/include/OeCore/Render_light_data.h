#pragma once

#include "Shadow_map_texture.h"
#include "Color.h"
#include "Simple_types.h"

namespace oe
{
	class Render_light_data {
	public:
		ID3D11Buffer* buffer() const { return _constantBuffer.Get(); }

		std::shared_ptr<Texture> environmentMapBrdf() const
		{
			return _environmentIblMapBrdf;
		}
		std::shared_ptr<Texture> environmentMapDiffuse() const
		{
			return _environmentIblMapDiffuse;
		}
		std::shared_ptr<Texture> environmentMapSpecular() const
		{
			return _environmentIblMapSpecular;
		}

	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
		std::shared_ptr<Texture> _environmentIblMapBrdf;
		std::shared_ptr<Texture> _environmentIblMapDiffuse;
		std::shared_ptr<Texture> _environmentIblMapSpecular;
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

		bool addPointLight(const SSE::Vector3& lightPosition, const Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Point, lightPosition, encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		bool addDirectionalLight(const SSE::Vector3& lightDirection, const Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Directional, lightDirection, encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		bool addDirectionalLight(const SSE::Vector3& lightDirection, 
			const Color& color, 
			float intensity, 
			const Shadow_map_texture_array_slice& shadowMapTexture,
			float shadowMapBias)
		{
			typename Light_constants::Light_entry lightEntry = { 
				Light_type::Directional, 
				lightDirection, 
				encodeColor(color, intensity),
				static_cast<int32_t>(shadowMapTexture.arraySlice()),
				shadowMapTexture.worldViewProjMatrix(),
				shadowMapBias,
				static_cast<int32_t>(shadowMapTexture.textureWidth())
			};

			if (_lightConstants.addLight(std::move(lightEntry))) {
				return true;
			}
			return false;
		}
		bool addAmbientLight(const Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Ambient, SSE::Vector3(0), encodeColor(color, intensity), Light_constants::shadow_map_disabled_index });
		}
		void setEnvironmentIblMap(
			std::shared_ptr<Texture> brdf,
			std::shared_ptr<Texture> diffuse, 
			std::shared_ptr<Texture> specular)
		{
			_environmentIblMapBrdf = brdf;
			_environmentIblMapDiffuse = diffuse;
			_environmentIblMapSpecular = specular;
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
				//  If you change this to a Vector3, note that  SSE::Vector3 is actually sizeof(Vector4), since it uses __m128!
				Float3 lightPositionDirection;
				Float3 intensifiedColor;
				int32_t shadowMapIndex = shadow_map_disabled_index;
				SSE::Matrix4 shadowViewProjMatrix;
				float shadowMapBias = 0.0f;
				int32_t shadowmapDimension = 0;
				float unused[2];
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

		static SSE::Vector3 encodeColor(const Color& color, float intensity)
		{
			return color.getXYZ() * intensity;
		}

		Light_constants _lightConstants;
	};
}
