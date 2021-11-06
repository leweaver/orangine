#pragma once
#include "Manager_base.h"

namespace oe {
    class IAnimation_manager :
        public Manager_base,
        public Manager_tickable
    {
    public:
        explicit IAnimation_manager(Scene& scene) : Manager_base(scene) {}
    };
}