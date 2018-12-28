#pragma once
#include "Component.h"

namespace oe {
	class Camera_component : public Component
	{
		DECLARE_COMPONENT_TYPE;
	public:

		Camera_component(std::shared_ptr<Entity> entity)
			: Component(entity)
			, _fov(DirectX::XMConvertToRadians(60.f))
		{}

		// Field of view, in radians
		float fov() const { return _fov; }
		void setFov(float fov) { _fov = fov; }

		float nearPlane() const { return _nearPlane; }
		void setNearPlane(float nearPlane) { _nearPlane = nearPlane; }

		float farPlane() const { return _farPlane; }
		void setFarPlane(float farPlane) { _farPlane = farPlane; }

	private:
		float _fov;
		float _nearPlane = 0.1f;
		float _farPlane = 1000.f;
	};
}
