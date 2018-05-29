#pragma once

#include "Manager_base.h"

namespace oe {
	class Entity_filter;

	class Entity_scripting_manager : public Manager_base {
	public:
		explicit Entity_scripting_manager(Scene &scene);
		
		void initialize() override;
		void tick() override;
		void shutdown() override {}

	private:
		std::shared_ptr<Entity_filter> _scriptableEntityFilter;
	};
}
