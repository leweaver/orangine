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
