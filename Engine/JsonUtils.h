#pragma once
#include <json.hpp>

#include "SimpleMath.h"

namespace DirectX::SimpleMath {

    inline void to_json(nlohmann::json& j, const Color& color) {
        j = nlohmann::json{ color.x, color.y, color.z, color.w };
    }

    inline void from_json(const nlohmann::json& j, Color& p) {
        j.at(0).get_to(p.x);
        j.at(1).get_to(p.y);
        j.at(2).get_to(p.z);
        j.at(3).get_to(p.w);
    }
}
