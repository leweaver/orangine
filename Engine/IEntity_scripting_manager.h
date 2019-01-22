#pragma once

#include "Manager_base.h"

namespace oe {
    class Entity_filter;

    class IEntity_scripting_manager : public Manager_base, public Manager_tickable {
    public:
        explicit IEntity_scripting_manager(Scene& scene) : Manager_base(scene) {}

        virtual void renderImGui() = 0;
        virtual void execute(const std::string& command) = 0;
        virtual bool commandSuggestions(const std::string& command, std::vector<std::string>& suggestions) = 0;
    };
}