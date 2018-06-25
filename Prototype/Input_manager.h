#pragma once
#include "Manager_base.h"

namespace DX {
	class DeviceResources;
}

namespace oe {

class Input_manager : public Manager_base {
public:
	explicit Input_manager(Scene &scene, DX::DeviceResources& deviceResources);

	// Manager_base implementation
	void initialize();
	void shutdown();
};

}
