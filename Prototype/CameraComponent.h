#pragma once
#include "Component.h"

namespace OE {
	class CameraComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		float m_nearPlane = 0.1f;
		float m_farPlane = 1000.f;
		float m_fov = 60.f;
	public:

		float getNearPlane() const { return m_nearPlane; }
		void setNearPlane(float nearPlane) { m_nearPlane = nearPlane; }


		float getFarPlane() const { return m_farPlane; }
		void setFarPlane(float farPlane) { m_farPlane = farPlane; }

		float getFov() const { return m_fov; }
		void setFov(float fov) { m_fov = fov; }
	};
}
