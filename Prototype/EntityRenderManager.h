#pragma once

#include "DeviceResources.h"
#include "ManagerBase.h"
#include "RendererData.h"
#include "MeshDataComponent.h"
#include "MaterialRepository.h"
#include "Game.h"
#include "PrimitiveMeshDataFactory.h"

namespace OE {
	class Scene;
	class Material;
	class EntityFilter;

	class EntityRenderManager : public ManagerBase
	{
		struct Renderable
		{
			Renderable();
			std::shared_ptr<MeshData> meshData;
			std::shared_ptr<Material> material;
			std::unique_ptr<RendererData> rendererData;
		};

		DX::DeviceResources m_deviceResources;

		std::shared_ptr<EntityFilter> m_renderableEntities;
		std::shared_ptr<EntityFilter> m_lightEntities;
		std::shared_ptr<MaterialRepository> m_materialRepository;

		std::unique_ptr<PrimitiveMeshDataFactory> m_primitiveMeshDataFactory;
		
		Renderable m_screenSpaceQuad;

	public:
		EntityRenderManager(Scene& scene, const std::shared_ptr<MaterialRepository> &materialRepository, DX::DeviceResources &deviceResources);
		~EntityRenderManager();
		
		void Initialize() override;
		void Tick() override;
		void Shutdown() override;

		void createDeviceDependentResources();
		void createWindowSizeDependentResources();
		void destroyDeviceDependentResources();

		void clearGBuffer();
		void render();

	private:

		ID3D11RasterizerState *m_rasterizerState;

		std::unique_ptr<Material> LoadMaterial(const std::string &materialName) const;

		std::shared_ptr<D3DBuffer> CreateBufferFromData(const MeshBuffer &buffer, UINT bindFlags) const;
		std::unique_ptr<RendererData> CreateRendererData(const MeshData &meshData, const std::vector<VertexAttribute> &vertexAttributes) const;

		// renders a full screen quad that sets our output buffers to decent default values.

		void initScreenSpaceQuad();
	};
}
