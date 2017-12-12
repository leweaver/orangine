#pragma once

#include "DeviceResources.h"
#include "ManagerBase.h"

#include <map>
#include "RendererData.h"
#include "MeshLoader.h"

namespace OE {
	class Scene;
	class Material;
	class RendererData;
	class EntityFilter;

	class EntityRenderManager : public ManagerBase
	{
		std::map<std::string, std::unique_ptr<RendererData>> m_meshRendererData;
		std::map<std::string, std::unique_ptr<Material>> m_materials;
		std::shared_ptr<EntityFilter> m_entityFilter;
		std::map<std::string, std::shared_ptr<MeshLoader>> m_meshLoaders;


	public:
		explicit EntityRenderManager(Scene &scene);
		~EntityRenderManager();
		
		void Initialize() override;
		void Tick() override;

		void CreateDeviceDependentResources(const DX::DeviceResources &deviceResources);
		void CreateWindowSizeDependentResources(const DX::DeviceResources &deviceResources);
		void Render(const DX::DeviceResources &deviceResources);
		
		template <typename TLoader>
		std::shared_ptr<TLoader> AddMeshLoader()
		{
			const std::shared_ptr<TLoader> ml = std::make_shared<TLoader>();
			std::vector<std::string> extensions;
			ml->GetSupportedFileExtensions(extensions);
			for (const auto extension : extensions) {
				if (m_meshLoaders.find(extension) != m_meshLoaders.end())
					throw std::runtime_error("Failed to register mesh loader, extension is already registered: " + extension);
				m_meshLoaders[extension] = ml;
			}
			return ml;
		}
		
	private:
		std::unique_ptr<VertexBufferAccessor> CreateBufferFromData(const DX::DeviceResources &deviceResources, const void *data, UINT elementSize, UINT numVertices) const;
		std::unique_ptr<RendererData> LoadRendererDataFromFile(const std::string &meshName, const DX::DeviceResources &deviceResources) const;
		std::unique_ptr<Material> LoadMaterial(const std::string &materialName);


		std::unique_ptr<RendererData> LoadDummyData(const DX::DeviceResources &deviceResources) const;
	};

}
