#pragma once

#include <gmock/gmock.h>

#include "..\Engine\IAsset_manager.h"

namespace oe_test {
	class Mock_asset_manager : public oe::IAsset_manager {
	public:
		Mock_asset_manager(oe::Scene& scene) : IAsset_manager(scene) {}

		MOCK_CONST_METHOD2(getFilePath, bool(oe::FAsset_id, std::wstring&));

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

	};
}