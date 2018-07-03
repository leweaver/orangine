#pragma once

#include "Manager_base.h"

namespace oe {
	class Entity_filter;

	class Entity_scripting_manager : public Manager_base, public Manager_tickable {
	public:
		explicit Entity_scripting_manager(Scene &scene);


		// Manager_base implementation
		void initialize();
		void shutdown();

		// Manager_tickable implementation
		void tick();

	private:
		std::shared_ptr<Entity_filter> _scriptableEntityFilter;

		// TODO: This would be part of a script context
		struct ScriptData {
			float yaw = 0.0f;
			float pitch = 0.0f;
			float distance = 10.0f;
		};
		ScriptData _scriptData;
	};
}