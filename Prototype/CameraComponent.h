#pragma once
#include "Component.h"

namespace OE {
	class CameraComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		float m_fov = DirectX::XMConvertToRadians(60.f);
		//float m_aspectRatio = 60.f;
		float m_nearPlane = 0.1f;
		float m_farPlane = 1000.f;
	public:

		// Field of view, in radians
		float Fov() const { return m_fov; }
		void SetFov(float fov) { m_fov = fov; }
		/*
		float AspectRatio() const { return m_aspectRatio; }
		void SetAspectRatio(float aspectRatio) { m_aspectRatio = m_aspectRatio; }
		*/
		float NearPlane() const { return m_nearPlane; }
		void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }

		float FarPlane() const { return m_farPlane; }
		void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
	};
}
