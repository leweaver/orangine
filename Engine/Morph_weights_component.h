#pragma once

#include "Component.h"
#include <array>

namespace oe {
    class Morph_weights_component : public Component
    {
        DECLARE_COMPONENT_TYPE;
    public:

        Morph_weights_component(Entity& entity)
            : Component(entity)
        {}

        static uint8_t maxMorphTargetCount() { return 8; }
        uint8_t morphTargetCount() const { return _morphTargetCount; }
        void setMorphTargetCount(uint8_t morphTargetCount) {
            _morphTargetCount = morphTargetCount;
        }

        const std::array<double, 8>& morphWeights() const { return _morphWeights; }
        std::array<double, 8>& morphWeights() { return _morphWeights; }

    private:
        std::array<double, 8> _morphWeights;
        uint8_t _morphTargetCount = 0;
    };
}