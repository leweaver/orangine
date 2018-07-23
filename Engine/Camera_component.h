#pragma once
#include "Component.h"

namespace oe {
	class Camera_component : public Component
	{
		DECLARE_COMPONENT_TYPE;
	public:

		// Field of view, in radians
		float fov() const { return _fov; }
		void setFov(float fov) { _fov = fov; }

		float nearPlane() const { return _nearPlane; }
		void setNearPlane(float nearPlane) { _nearPlane = nearPlane; }

		float farPlane() const { return _farPlane; }
		void setFarPlane(float farPlane) { _farPlane = farPlane; }

	private:
		float _fov = DirectX::XMConvertToRadians(60.f);
		float _nearPlane = 0.1f;
		float _farPlane = 1000.f;
	};
}
