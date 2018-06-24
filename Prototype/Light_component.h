#pragma once

#include "Component.h"

namespace oe {
	class Light_component : public Component
	{
		DirectX::SimpleMath::Color _color{};
		float _intensity;

	public:
		
		Light_component()
			: _color(DirectX::Colors::White)
			, _intensity(1.0)
		{
		}

		const DirectX::SimpleMath::Color& color() const
		{
			return _color;
		}

		void setColor(const DirectX::SimpleMath::Color& color)
		{
			_color = color;
		}

		float intensity() const
		{
			return _intensity;
		}

		void setIntensity(float intensity)
		{
			_intensity = intensity;
		}
	};

	class Directional_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		bool shadowsEnabled() const { return _shadowsEnabled; }
		void setShadowsEnabled(bool shadowsEnabled) { _shadowsEnabled = shadowsEnabled; }
	private:

		bool _shadowsEnabled = false;
	};

	class Point_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;
	};

	class Ambient_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;
	};
}
