#pragma once

#include "DeviceResources.h"
#include "ManagerBase.h"
#include "RendererData.h"
#include "MeshDataComponent.h"

#include <map>
#include "MaterialRepository.h"

namespace OE {
	class Scene;
	class Material;
	class EntityFilter;

	class EntityRenderManager : public ManagerBase
	{
		std::shared_ptr<EntityFilter> m_renderableEntities;
		std::shared_ptr<MaterialRepository> m_materialRepository;

	public:
		EntityRenderManager(Scene& scene, const std::shared_ptr<MaterialRepository> &materialRepository);
		~EntityRenderManager();
		
		void Initialize() override;
		void Tick() override;
		void Shutdown() override;

		void CreateDeviceDependentResources(const DX::DeviceResources &deviceResources);
		void CreateWindowSizeDependentResources(const DX::DeviceResources &deviceResources);
		void DestroyDeviceDependentResources();

		void Render(const DX::DeviceResources &deviceResources);

	private:

		ID3D11RasterizerState *m_rasterizerState;

		std::unique_ptr<Material> LoadMaterial(const std::string &materialName) const;

		std::shared_ptr<D3DBuffer> CreateBufferFromData(const MeshBuffer &buffer, UINT bindFlags, const DX::DeviceResources &deviceResources) const;
		std::unique_ptr<RendererData> CreateRendererData(const MeshDataComponent &meshData, const DX::DeviceResources &deviceResources) const;
	};

}
