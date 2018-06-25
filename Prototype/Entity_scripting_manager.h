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
	};
}
