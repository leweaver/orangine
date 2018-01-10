#pragma once

#include <string>
#include "SimpleTypes.h"
#include "ManagerBase.h"
#include <map>

namespace OE {
	
	class AssetManager : public ManagerBase
	{
		struct AssetInfo
		{
			AssetId m_id;
			std::string m_filename;

			// TODO: More things here as required
		};

		std::map<AssetId, AssetInfo> m_assets;

	public:
		AssetManager(Scene &scene);
		virtual ~AssetManager() {}

		bool AssetManager::GetFilePath(FAssetId assetId, std::string &path) const;

		void Initialize() override;
		void Tick() override;
		void Shutdown() override {}

	};
}
