#pragma once

#include "Component.h"

namespace oe {
	class Shadow_map_texture;

	class Light_component : public Component
	{
		DirectX::SimpleMath::Color _color{};
		float _intensity;

	public:
		
		Light_component(Entity& entity)
			: Component(entity)
			, _color(DirectX::Colors::White)
			, _intensity(1.0)
		{
		}

		~Light_component();

		const DirectX::SimpleMath::Color& color() const { return _color; }
		void setColor(const DirectX::SimpleMath::Color& color) { _color = color; }

		float intensity() const { return _intensity; }
		void setIntensity(float intensity) { _intensity = intensity; }

		// Runtime, non-serializable
		std::unique_ptr<Shadow_map_texture>& shadowData() { return _shadowData; }

	private:
		std::unique_ptr<Shadow_map_texture> _shadowData;

	};

	class Directional_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		Directional_light_component(Entity& entity)
			: Light_component(entity)
		{}

		bool shadowsEnabled() const { return _shadowsEnabled; }
		void setShadowsEnabled(bool shadowsEnabled) { _shadowsEnabled = shadowsEnabled; }

		float shadowMapBias() const { return _shadowMapBias; }
		void setShadowMapBias(float shadowMapBias) { _shadowMapBias = shadowMapBias; }
	private:

		bool _shadowsEnabled = false;
		float _shadowMapBias = 0.1f;
	};

	class Point_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		Point_light_component(Entity& entity)
			: Light_component(entity)
		{}
	};

	class Ambient_light_component : public Light_component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		Ambient_light_component(Entity& entity)
			: Light_component(entity)
		{}
	};
}
