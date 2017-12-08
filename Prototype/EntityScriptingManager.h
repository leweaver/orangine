#pragma once

#include "ManagerBase.h"

namespace OE {
	class EntityFilter;

	class EntityScriptingManager : public ManagerBase
	{
		std::shared_ptr<EntityFilter> m_scriptableEntityFilter;

	public:
		explicit EntityScriptingManager(Scene &scene);
		~EntityScriptingManager();
		
		void Initialize() override;
		void Tick() override;
	};
}
