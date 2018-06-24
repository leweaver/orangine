#pragma once

namespace oe
{
	class Render_light_data {
	public:
		ID3D11Buffer* buffer() const { return _constantBuffer.Get(); }

	protected:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _constantBuffer;
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
			// DirectX constant buffers must be aligned to 16 byte boudaries (vector4)
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
			return _lightConstants.addLight({ Light_type::Point, lightPosition, encodeColor(color, intensity) });
		}
		bool addDirectionalLight(const DirectX::SimpleMath::Vector3& lightDirection, const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Directional, lightDirection, encodeColor(color, intensity) });
		}
		bool addAmbientLight(const DirectX::SimpleMath::Color& color, float intensity)
		{
			return _lightConstants.addLight({ Light_type::Ambient, DirectX::SimpleMath::Vector4::Zero, encodeColor(color, intensity) });
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

				
	private:

		class alignas(16) Light_constants {
		public:

			struct Light {
				Light_type type = Light_type::Directional;
				DirectX::SimpleMath::Vector3 lightPositionDirection;
				DirectX::SimpleMath::Vector3 intensifiedColor;
				uint32_t pad0 = 0;
			};

			Light_constants()
			{
				std::fill(_lights.begin(), _lights.end(), Light());
			}

			bool addLight(Light&& light)
			{
				if (full())
					return false;

				_lights[_activeLights++] = light;
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
			std::array<Light, TMax_lights> _lights;
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