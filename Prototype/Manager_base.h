#pragma once

namespace oe {
	class Scene;

	class Manager_base {
	public:
		explicit Manager_base(Scene& scene)
			: _scene(scene) {}
		virtual ~Manager_base() = default;
		
		virtual void initialize() = 0;
		virtual void tick() = 0;
		virtual void shutdown() = 0;

	protected:
		Scene& _scene;
	};
}