#pragma once

#include <gmock/gmock.h>

#include "..\Engine\Entity_render_manager.h"
#include "..\Engine\Camera_component.h"
#include "..\Engine\Renderable_component.h"

namespace oe_test {
	class Mock_entity_render_manager : public oe::IEntity_render_manager {
	public:
		Mock_entity_render_manager(oe::Scene& scene) : IEntity_render_manager(scene) {}
		
		MOCK_METHOD1(createFrustum, oe::BoundingFrustumRH(const oe::Camera_component&));
		
		MOCK_METHOD7(renderRenderable, void(oe::Renderable&,
			const DirectX::SimpleMath::Matrix&,
			float,
			const oe::Render_pass::Camera_data&,
			const oe::Light_provider::Callback_type&,
			oe::Render_pass_blend_mode,
			bool
			));
		
		MOCK_METHOD4(renderEntity, void(oe::Renderable_component&,
			const oe::Render_pass::Camera_data&,
			const oe::Light_provider::Callback_type&,
			oe::Render_pass_blend_mode
			));

		MOCK_CONST_METHOD1(createScreenSpaceQuad, oe::Renderable(std::shared_ptr<oe::Material>));
		MOCK_METHOD0(clearRenderStats, void());

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