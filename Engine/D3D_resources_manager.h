#pragma once
#include "Manager_base.h"

namespace DirectX {
	class CommonStates;
}

namespace oe {
	class ID3D_resources_manager :
		public Manager_base,
		public Manager_deviceDependent 
	{
	public:
		explicit ID3D_resources_manager(Scene& scene) : Manager_base(scene) {}

		virtual DirectX::CommonStates& commonStates() const = 0;
		virtual DX::DeviceResources& deviceResources() const = 0;
	};

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
