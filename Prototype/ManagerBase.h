#pragma once

namespace OE {
	class Scene;

	class ManagerBase {
	protected:
		Scene &m_scene;

	public:
		explicit ManagerBase(Scene &scene)
			: m_scene(scene) {}
		virtual ~ManagerBase() {};
		
		virtual void Initialize() = 0;
		virtual void Tick() = 0;
	};
}