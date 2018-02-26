#pragma once

#include "Component.h"

namespace OE {
	class LightComponent : public Component
	{
		DirectX::SimpleMath::Color m_color{};
		float m_intensity;

	public:
		
		LightComponent()
			: m_color(DirectX::Colors::White)
			, m_intensity(1.0)
		{
		}

		const DirectX::SimpleMath::Color &getColor() const
		{
			return m_color;
		}

		void setColor(const DirectX::SimpleMath::Color &color)
		{
			m_color = color;
		}

		float getIntensity() const
		{
			return m_intensity;
		}

		void setIntensity(float intensity)
		{
			m_intensity = intensity;
		}
	};

	class DirectionalLightComponent : public LightComponent
	{
		DECLARE_COMPONENT_TYPE;
	};

	class PointLightComponent : public LightComponent
	{
		DECLARE_COMPONENT_TYPE;
	};
}
