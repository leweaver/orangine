#pragma once

#include <SimpleMath.h>
#include <vectormath/vectormath.hpp>
#include "Color.h"

#include <tinygltf/json.hpp>

namespace Vectormath::SSE {

	inline void to_json(nlohmann::json& j, const Vector4& vec4) {
		j = nlohmann::json{
			static_cast<float>(vec4.getX()),
			static_cast<float>(vec4.getY()),
			static_cast<float>(vec4.getZ()),
			static_cast<float>(vec4.getW())
		};
	}

	inline void from_json(const nlohmann::json& j, Vector4& vec4) {
		vec4.setX(j.at(0).get<float>());
		vec4.setY(j.at(1).get<float>());
		vec4.setZ(j.at(2).get<float>());
		vec4.setW(j.at(3).get<float>());
	}
}

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
