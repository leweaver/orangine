#pragma once
#include "ID3D_resources_manager.h"

namespace oe::internal {
	class D3D_resources_manager : public ID3D_resources_manager {
	public:
		D3D_resources_manager(Scene& scene, DX::DeviceResources& deviceResources);

		// Manager_base implementation
		void initialize() override {};
		void shutdown() override {};

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		// ID3D_resources_manager implementation
		DirectX::CommonStates& commonStates() const override { return *_commonStates.get(); }
		DX::DeviceResources& deviceResources() const override { return _deviceResources; }

	private:

		DX::DeviceResources& _deviceResources;
		std::unique_ptr<DirectX::CommonStates> _commonStates;
        HWND _window;
	};
}
