#pragma once

namespace oe {
    class IConfigReader {
    public:
		virtual std::wstring readPath(const std::string& jsonPath) const = 0;
        virtual std::map<std::wstring, std::wstring> readPathToPathMap(const std::string& jsonPath) const = 0;
    };

}