#pragma once

#include "Manager_base.h"
#include "Simple_types.h"
#include <string>

namespace oe {
    class IAsset_manager : public Manager_base {
    public:
        explicit IAsset_manager(Scene& scene) : Manager_base(scene) {}

		// Sets the default data path, that contains scripts/assets/shaders.
		// Must be called before initialize(), otherwise an exception will be thrown.
		virtual void setDataPath(const std::wstring&) = 0;
		virtual const std::wstring& getDataPath() const = 0;

		virtual void setDataPathOverrides(std::map<std::wstring, std::wstring>&& paths) = 0;
        virtual const std::map<std::wstring, std::wstring>& dataPathOverrides(std::map<std::wstring, std::wstring>&& paths) const = 0;

        virtual std::wstring makeAbsoluteAssetPath(const std::wstring& path) const = 0;
    };
}