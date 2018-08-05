#pragma once

#include "DeviceResources.h"

namespace oe {
	class Scene;

	class Manager_tickable {
	public:
		virtual void tick() = 0;
	};

	class Manager_windowDependent {
	public:
		virtual void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) = 0;
		virtual void destroyWindowSizeDependentResources() = 0;
	};

	class Manager_deviceDependent {
	public:
		virtual void createDeviceDependentResources(DX::DeviceResources& deviceResources) = 0;
		virtual void destroyDeviceDependentResources() = 0;
	};

	class Manager_windowsMessageProcessor {
	public:
		virtual void processMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0;
	};

	class Manager_base {
	public:
		explicit Manager_base(Scene& scene)
			: _scene(scene) {}
		virtual ~Manager_base() = default;
		
		virtual void initialize() = 0;
		virtual void shutdown() = 0;
	protected:
		Scene& _scene;
	};
}