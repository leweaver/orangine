#pragma once

#include "DeviceResources.h"
#include "ManagerBase.h"
#include "RendererData.h"
#include "MeshDataComponent.h"
#include "MaterialRepository.h"
#include "Game.h"
#include "PrimitiveMeshDataFactory.h"
#include "DeferredLightMaterial.h"
#include <memory>

namespace OE {
	class Scene;
	class Material;
	class EntityFilter;
	class TextureRenderTarget;

	class EntityRenderManager : public ManagerBase
	{
		struct BufferArraySet {
			std::vector<ID3D11Buffer*> bufferArray;
			std::vector<UINT> strideArray;
			std::vector<UINT> offsetArray;
		};

		struct Renderable
		{
			Renderable();
			std::shared_ptr<MeshData> meshData;
			std::shared_ptr<Material> material;
			std::unique_ptr<RendererData> rendererData;
		};

		struct CameraData
		{
			DirectX::SimpleMath::Matrix worldMatrix;
			DirectX::SimpleMath::Matrix viewMatrix;
			DirectX::SimpleMath::Matrix projectionMatrix;

			static const CameraData Identity;
		};

		DX::DeviceResources &m_deviceResources;

		std::shared_ptr<EntityFilter> m_renderableEntities;
		std::shared_ptr<EntityFilter> m_lightEntities;
		std::shared_ptr<MaterialRepository> m_materialRepository;

		std::unique_ptr<PrimitiveMeshDataFactory> m_primitiveMeshDataFactory;

		bool m_fatalError;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerStateDepthEnabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilStateDepthEnabled;

		std::vector<std::shared_ptr<TextureRenderTarget>> m_pass1RenderTargets;
		
		CameraData m_cameraData;
		Renderable m_pass1ScreenSpaceQuad;
		Renderable m_pass2ScreenSpaceQuad;

		std::shared_ptr<Texture> m_depthTexture;
		std::shared_ptr<DeferredLightMaterial> m_deferredLightMaterial;

	public:
		EntityRenderManager(Scene& scene, const std::shared_ptr<MaterialRepository> &materialRepository, DX::DeviceResources &deviceResources);
		~EntityRenderManager();
		
		void Initialize() override;
		void Tick() override;
		void Shutdown() override;

		void createDeviceDependentResources();
		void createWindowSizeDependentResources();
		void destroyDeviceDependentResources();

		void render();

	protected:
		void renderEntities();
		void renderLights();

	private:

		void setDepthEnabled(bool enabled);
		void setupRenderEntities();
		void setupRenderLights();

		std::unique_ptr<Material> loadMaterial(const std::string &materialName) const;

		std::shared_ptr<D3DBuffer> createBufferFromData(const MeshBuffer &buffer, UINT bindFlags) const;
		std::unique_ptr<RendererData> createRendererData(const MeshData &meshData, const std::vector<VertexAttribute> &vertexAttributes) const;

		void loadRendererDataToDeviceContext(const RendererData *rendererData, BufferArraySet &bufferArraySet) const;
		void drawRendererData(
			const CameraData &cameraData,
			const DirectX::SimpleMath::Matrix &worldTransform,
			const RendererData *rendererData, 
			Material* material,
			BufferArraySet &bufferArraySet) const;

		// renders a full screen quad that sets our output buffers to decent default values.
		// Takes ownership of the passed in material
		Renderable initScreenSpaceQuad(std::shared_ptr<Material> material) const;
	};
}
