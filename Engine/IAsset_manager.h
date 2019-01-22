#pragma once

#include "Manager_base.h"
#include "Simple_types.h"
#include <string>

namespace oe {
    class IAsset_manager : public Manager_base {
    public:
        explicit IAsset_manager(Scene& scene) : Manager_base(scene) {}

        virtual bool getFilePath(FAsset_id assetId, std::wstring& path) const = 0;
    };
}