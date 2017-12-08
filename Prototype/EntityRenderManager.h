#pragma once

#include "DeviceResources.h"
#include "ManagerBase.h"

#include <map>

namespace OE {
	class Scene;
	class Material;
	class RendererData;
	class EntityFilter;

	class EntityRenderManager : public ManagerBase
	{
		std::map<std::string, std::unique_ptr<RendererData>> m_meshRendererData;
		std::map<std::string, std::unique_ptr<Material>> m_materials;
		std::shared_ptr<EntityFilter> entityFilter;

	public:
		explicit EntityRenderManager(Scene& scene);
		~EntityRenderManager();
		
		void Initialize() override;
		void Tick() override;

		void CreateDeviceDependentResources(const DX::DeviceResources& deviceResources);
		void CreateWindowSizeDependentResources(const DX::DeviceResources& deviceResources);
		void Render(const DX::DeviceResources& deviceResources);
		
	private:
		std::unique_ptr<RendererData> CreateRendererData(const DX::DeviceResources deviceResources);
		std::unique_ptr<Material> LoadMaterial(const std::string &materialName);
	};

}
