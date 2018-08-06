#pragma once

#include <gmock/gmock.h>

#include "..\Engine\Entity_repository.h"

namespace oe_test {
	class Mock_entity_repository : public oe::IEntity_repository {
	public:
		Mock_entity_repository() : IEntity_repository() {}

		MOCK_METHOD1(instantiate, std::shared_ptr<oe::Entity>(std::string_view name));
		MOCK_METHOD1(remove, void(oe::Entity::Id_type id));
		MOCK_METHOD1(getEntityPtrById, std::shared_ptr<oe::Entity>(oe::Entity::Id_type id));
		MOCK_METHOD1(getEntityById, oe::Entity& (oe::Entity::Id_type id));
	};
}