#pragma once
#include "Manager_base.h"

namespace DX {
	class DeviceResources;
}

namespace DirectX {
	class Mouse;
}

namespace oe {

class Input_manager : public Manager_base, public Manager_windowDependent {
public:
	explicit Input_manager(Scene &scene, DX::DeviceResources& deviceResources);

	// Manager_base implementation
	void initialize() override;
	void shutdown() override;

	// Manager_windowDependent implementation
	void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
	void destroyWindowSizeDependentResources() override;

private:
	std::unique_ptr<DirectX::Mouse> _mouse;
};

}
