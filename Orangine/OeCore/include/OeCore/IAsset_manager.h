#pragma once

#include "Manager_base.h"
#include "Simple_types.h"
#include <string>

namespace oe {
    class IAsset_manager : public Manager_base {
    public:
        explicit IAsset_manager(Scene& scene) : Manager_base(scene) {}


		// Sets the root data path, that contains scripts/assets/shaders.
		// Must be called before initialize(), otherwise an exception will be thrown.
		virtual void setDataPath(const std::wstring&) = 0;
		virtual const std::wstring& getDataPath() const = 0;

        virtual bool getFilePath(FAsset_id assetId, std::wstring& path) const = 0;
    };
}