#pragma once
#include "Entity.h"
#include "Component.h"
#include "Entity_filter.h"
#include "Manager_base.h"
#include "Entity_filter_mode.h"

#include <vector>
#include <unordered_set>

namespace oe {

    class IScene_graph_manager :
        public Manager_base,
        public Manager_tickable {
    public:

        using Component_type_set = std::unordered_set<Component::Component_type>;

        IScene_graph_manager(Scene& scene)
            : Manager_base(scene)
        {}
        virtual ~IScene_graph_manager() = default;

        virtual std::shared_ptr<Entity> instantiate(const std::string& name) = 0;
        virtual std::shared_ptr<Entity> instantiate(const std::string& name, Entity& parentEntity) = 0;
        virtual void renderImGui() = 0;

        /**
        * Will do nothing if no entity exists with the given ID.
        */
        virtual void destroy(Entity::Id_type entityId) = 0;

        virtual std::shared_ptr<Entity> getEntityPtrById(Entity::Id_type id) const = 0;
        virtual std::shared_ptr<Entity_filter> getEntityFilter(const Component_type_set& componentTypes, Entity_filter_mode mode = Entity_filter_mode::All) = 0;

        virtual void handleEntityAdd(const Entity& entity) = 0;
        virtual void handleEntityRemove(const Entity& entity) = 0;
        virtual void handleEntityComponentAdd(const Entity& entity, const Component& componentType) = 0;
        virtual void handleEntityComponentRemove(const Entity& entity, const Component& componentType) = 0;
        virtual void handleEntitiesLoaded(const std::vector<std::shared_ptr<Entity>>& loadedEntities) = 0;


        // Internal use only.
        virtual std::shared_ptr<Entity> removeFromRoot(std::shared_ptr<Entity> entity) = 0;

        // Internal use only.
        virtual void addToRoot(std::shared_ptr<Entity> entity) = 0;
    };
}
