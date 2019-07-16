#pragma once

#include <string>
#include <tinygltf/json.hpp>
#include <OeCore/IConfigReader.h>

namespace oe {
    class JsonConfigReader : public IConfigReader {
    public:
        explicit JsonConfigReader(const std::wstring& configFileName);

		std::wstring readPath(const std::string& jsonPath) const override;
        std::map<std::wstring, std::wstring> readPathToPathMap(const std::string& jsonPath) const override;
    private:
		nlohmann::json::value_type JsonConfigReader::getJsonValue(const std::string& jsonPath) const;
		nlohmann::json::value_type _configJson;
    };
}