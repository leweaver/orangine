#pragma once

namespace oe {
	class Scene;

	class Manager_tickable {
		// Classes that implement this interface must have the following methods:
		//     void tick();
	};

	class Manager_base {
	public:
		explicit Manager_base(Scene& scene)
			: _scene(scene) {}
		virtual ~Manager_base() = default;
		
		// Classes that implement this interface must have the following methods:
		//     void initialize();
		//     void shutdown();
	protected:
		Scene& _scene;
	};
}