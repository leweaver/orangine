#pragma once

#include <gmock/gmock.h>

#include "..\Engine\Entity_render_manager.h"
#include "..\Engine\Camera_component.h"

namespace oe_test {
	class Mock_entity_render_manager : public oe::IEntity_render_manager {
	public:
		Mock_entity_render_manager(oe::Scene& scene) : IEntity_render_manager(scene) {}
		
		MOCK_METHOD0(render, void());
		MOCK_METHOD2(createFrustum, oe::BoundingFrustumRH(const oe::Entity&, const oe::Camera_component&));
		
		MOCK_METHOD3(addDebugSphere, void(const DirectX::SimpleMath::Matrix&, float, const DirectX::SimpleMath::Color&));
		MOCK_METHOD2(addDebugBoundingBox, void(const DirectX::BoundingOrientedBox& boundingOrientedBox, const DirectX::SimpleMath::Color& color));
		MOCK_METHOD2(addDebugFrustum, void(const oe::BoundingFrustumRH&, const DirectX::SimpleMath::Color&));
		MOCK_METHOD0(clearDebugShapes, void());

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

		// Manager_tickable implementation
		MOCK_METHOD0(tick, void());

		// Manager_windowDependent implementation
		MOCK_METHOD4(createWindowSizeDependentResources, void(DX::DeviceResources&, HWND, int, int));
		MOCK_METHOD0(destroyWindowSizeDependentResources, void());

		// Manager_deviceDependent implementation
		MOCK_METHOD1(createDeviceDependentResources, void(DX::DeviceResources&));
		MOCK_METHOD0(destroyDeviceDependentResources, void());
	};
}