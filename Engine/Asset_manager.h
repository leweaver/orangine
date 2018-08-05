#pragma once

#include "Manager_base.h"
#include "Simple_types.h"
#include <map>
#include <string>

namespace oe {
	
	class IAsset_manager : public Manager_base {
	public:
		explicit IAsset_manager(Scene& scene) : Manager_base(scene) {}
		
		virtual bool getFilePath(FAsset_id assetId, std::wstring& path) const = 0;
	};

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
	};
}
