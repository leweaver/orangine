#pragma once

#include "Manager_base.h"
#include "Simple_types.h"
#include <map>
#include <string>

namespace oe {
	
	class Asset_manager : public Manager_base, public Manager_tickable
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
		virtual ~Asset_manager() = default;

		bool getFilePath(FAsset_id assetId, std::wstring& path) const;
		
		// Manager_base implementation
		void initialize();
		void shutdown();

		// Manager_tickable implementation
		void tick();

	};
}
