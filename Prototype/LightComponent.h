#pragma once

#include "Component.h"
#include "SimpleTypes.h"

namespace OE {
	class LightComponent
	{
		Color m_color{};
		float m_intensity;

	public:
		
		LightComponent()
			: m_color(Color(255, 255, 255))
			, m_intensity(1.0)
		{
		}

		const Color &getColor() const
		{
			return m_color;
		}

		void setColor(const Color &color)
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

	class DirectionalLightComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;
	};
}
