﻿#pragma once

#include "OeCore/Simple_types.h"
#include "OeCore/IAsset_manager.h"
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

		void setDataPath(const std::wstring& dataPath) override;
		const std::wstring& getDataPath() const override { return _dataPath; }

        void setDataPathOverrides(std::map<std::wstring, std::wstring>&& paths) override;
        const std::map<std::wstring, std::wstring>& dataPathOverrides(std::map<std::wstring, std::wstring>&& paths) const override;

        std::wstring makeAbsoluteAssetPath(const std::wstring& path) const override;
		
		// Manager_base implementation
        void loadConfig(const IConfigReader&) override;
		void initialize() override;
		void shutdown() override;
        const std::string& name() const override;

	private:

        static std::string _name;
		std::wstring _dataPath = L"./data";
        std::map<std::wstring, std::wstring> _dataPathOverrides;

		bool _initialized = false;
	};
}
