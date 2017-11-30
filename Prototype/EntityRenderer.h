#pragma once
#include "DeviceResources.h"
#include "RenderableComponent.h"
#include <set>

namespace OE {
	class Scene;

	class EntityRenderer
	{
		Scene& m_scene;
		std::set<RenderableComponent*> m_renderables;

	public:
		explicit EntityRenderer(Scene& scene);
		~EntityRenderer();

		void CreateDeviceDependentResources(const DX::DeviceResources& deviceResources);
		void CreateWindowSizeDependentResources(const DX::DeviceResources& deviceResources);
		void Render(const DX::DeviceResources& deviceResources);
		
		void AddRenderable(RenderableComponent& renderableComponent);
		void RemoveRenderable(RenderableComponent& renderableComponent);
	};

}
