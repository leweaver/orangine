#pragma once

#include "IEntity_scripting_manager.h"

namespace oe::internal {
	class Entity_scripting_manager : public IEntity_scripting_manager {
	public:
		explicit Entity_scripting_manager(Scene &scene);

		// Manager_base implementation
		void initialize() override;
		void shutdown() override;
        
		// Manager_tickable implementation
		void tick() override;

        // IEntity_scripting_manager implementation
        void renderImGui() override;
        void execute(const std::string& command) override;
        bool commandSuggestions(const std::string& command, std::vector<std::string>& suggestions) override;

	private:
		std::shared_ptr<Entity_filter> _scriptableEntityFilter;
		std::shared_ptr<Entity_filter> _renderableEntityFilter;
		std::shared_ptr<Entity_filter> _lightEntityFilter;

		// TODO: This would be part of a script context
		struct ScriptData {
			float yaw = 0.0f;
			float pitch = 0.0f;
			float distance = 10.0f;
		};
		ScriptData _scriptData;

		void renderDebugSpheres() const;
	};
}
