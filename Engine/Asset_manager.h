#pragma once

#include "Simple_types.h"
#include "IAsset_manager.h"
#include <map>
#include <string>

namespace oe::internal {
	class Asset_manager : public IAsset_manager
	{
		struct Asset_info
		{
			Asset_id id;
			std::wstring filename;

			// TODO: More things here as required
		};

		std::map<Asset_id, Asset_info> _assets{};

	public:
		explicit Asset_manager(Scene& scene);

		bool getFilePath(FAsset_id assetId, std::wstring& path) const override;
		
		// Manager_base implementation
		void initialize() override;
		void shutdown() override;
        const std::string& name() const override;

	private:

        static std::string _name;
	};
}
