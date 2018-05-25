#pragma once

#include "DeviceResources.h"
#include "Manager_base.h"
#include "Renderer_data.h"
#include "Mesh_data_component.h"
#include "Material_repository.h"
#include "Primitive_mesh_data_factory.h"
#include "Deferred_light_material.h"
#include <memory>

namespace oe {
	class Scene;
	class Material;
	class Entity_filter;
	class Render_target_texture;

	class Entity_render_manager : public Manager_base
	{
		struct Buffer_array_set {
			std::vector<ID3D11Buffer*> bufferArray;
			std::vector<uint32_t> strideArray;
			std::vector<uint32_t> offsetArray;
		};

		struct Renderable
		{
			Renderable();
			std::shared_ptr<Mesh_data> meshData;
			std::shared_ptr<Material> material;
			std::unique_ptr<Renderer_data> rendererData;
		};

		struct Camera_data
		{
			DirectX::SimpleMath::Matrix worldMatrix;
			DirectX::SimpleMath::Matrix viewMatrix;
			DirectX::SimpleMath::Matrix projectionMatrix;

			static const Camera_data identity;
		};

	public:
		Entity_render_manager(Scene& scene, std::shared_ptr<Material_repository> materialRepository, DX::DeviceResources& deviceResources);
		~Entity_render_manager();
		
		void initialize() override;
		void tick() override;
		void shutdown() override;

		void createDeviceDependentResources();
		void createWindowSizeDependentResources();
		void destroyDeviceDependentResources();
		void destroyWindowSizeDependentResources();

		void render();

	protected:
		void renderEntities();
		void renderLights();

	private:

		void setDepthEnabled(bool enabled);
		void setupRenderEntities();
		void setupRenderLights();

		std::unique_ptr<Material> loadMaterial(const std::string& materialName) const;

		std::shared_ptr<D3D_buffer> createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const;
		std::unique_ptr<Renderer_data> createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const;

		void loadRendererDataToDeviceContext(const Renderer_data* rendererData, Buffer_array_set& bufferArraySet) const;
		void drawRendererData(
			const Camera_data& cameraData,
			const DirectX::SimpleMath::Matrix& worldTransform,
			const Renderer_data* rendererData,
			Material* material,
			Buffer_array_set& bufferArraySet) const;

		// renders a full screen quad that sets our output buffers to decent default values.
		// Takes ownership of the passed in material
		Renderable initScreenSpaceQuad(std::shared_ptr<Material> material) const;
		
		DX::DeviceResources& _deviceResources;

		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;
		std::shared_ptr<Material_repository> _materialRepository;

		std::unique_ptr<Primitive_mesh_data_factory> _primitiveMeshDataFactory;

		bool _fatalError;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerStateDepthEnabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilStateDepthEnabled;

		std::vector<std::shared_ptr<Render_target_texture>> _pass1RenderTargets;

		Camera_data _cameraData;
		Renderable _pass1ScreenSpaceQuad;
		Renderable _pass2ScreenSpaceQuad;

		std::shared_ptr<Texture> _depthTexture;
		std::shared_ptr<Deferred_light_material> _deferredLightMaterial;
	};
}
