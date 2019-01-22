#pragma once

#include <gmock/gmock.h>

#include "..\Engine\IScene_graph_manager.h"

namespace oe_test {
	class Mock_scene_graph_manager : public oe::IScene_graph_manager {
	public:
		Mock_scene_graph_manager(oe::Scene& scene) : IScene_graph_manager(scene) {}

		// Manager_base implementation
		MOCK_METHOD0(initialize, void());
		MOCK_METHOD0(shutdown, void());

		// Manager_tickable implementation
		MOCK_METHOD0(tick, void());

        // IScene_graph_manager implementation
        MOCK_METHOD1(instantiate, std::shared_ptr<oe::Entity>(const std::string&));
        MOCK_METHOD2(instantiate, std::shared_ptr<oe::Entity>(const std::string&, oe::Entity&));

        MOCK_METHOD1(destroy, void(oe::Entity::Id_type));

        MOCK_CONST_METHOD1(getEntityPtrById, std::shared_ptr<oe::Entity>(oe::Entity::Id_type));
        MOCK_METHOD2(getEntityFilter, std::shared_ptr<oe::Entity_filter>(const Component_type_set&, oe::Entity_filter_mode));

        MOCK_METHOD1(handleEntityAdd, void(const oe::Entity&));
        MOCK_METHOD1(handleEntityRemove, void(const oe::Entity&));

        MOCK_METHOD2(handleEntityComponentAdd, void(const oe::Entity&, const oe::Component&));
        MOCK_METHOD2(handleEntityComponentRemove, void(const oe::Entity&, const oe::Component&));
        MOCK_METHOD1(handleEntitiesLoaded, void(const std::vector<std::shared_ptr<oe::Entity>>&));

        MOCK_METHOD1(removeFromRoot, std::shared_ptr<oe::Entity>(std::shared_ptr<oe::Entity>));
        MOCK_METHOD1(addToRoot, void(std::shared_ptr<oe::Entity>));
	};
}