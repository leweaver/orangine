#pragma once

#include "OeCore/Component.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/Entity_filter_mode.h"

#include <set>
#include <memory>

namespace oe {
    class Entity;

    class Entity_filter_impl : public Entity_filter {
    public:
        using Component_type_set = std::set<Component::Component_type>;

        Component_type_set componentTypes;
        Entity_filter_mode mode;

        Entity_filter_impl(const Component_type_set::const_iterator &begin,
                           const Component_type_set::const_iterator &end,
                           Entity_filter_mode mode);

        void add_listener(std::weak_ptr<Entity_filter_listener> callback) override;

        void handleEntityAdd(std::shared_ptr <Entity> entity);
        void handleEntityRemove(std::shared_ptr <Entity> entity);
        void handleEntityComponentsUpdated(std::shared_ptr <Entity> entity);

    private:

        void publishEntityAdd(Entity* entity);
        void publishEntityRemove(Entity* entity);

        std::vector<std::weak_ptr<Entity_filter_listener>> _listeners;
    };
}