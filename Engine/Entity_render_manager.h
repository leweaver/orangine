#pragma once

#include "DeviceResources.h"
#include "Manager_base.h"
#include "Renderer_data.h"
#include "Mesh_data_component.h"
#include "Material_repository.h"
#include "Primitive_mesh_data_factory.h"
#include "Deferred_light_material.h"
#include "Render_light_data.h"
#include "Render_pass_info.h"
#include "Collision.h"

#include <memory>
#include <array>

namespace DirectX {
	class CommonStates;
}

namespace oe {
	class Unlit_material;
	class Camera_component;
	class Scene;
	class Material;
	class Entity_filter;
	class Render_target_texture;
	class Entity_alpha_sorter;
	class Entity_cull_sorter;

	class IEntity_render_manager :
		public Manager_base,
		public Manager_tickable,
		public Manager_deviceDependent,
		public Manager_windowDependent
	{
	public:
		IEntity_render_manager(Scene& scene) : Manager_base(scene) {}

		virtual void render() = 0;
		virtual BoundingFrustumRH createFrustum(const Entity& entity, const Camera_component& cameraComponent) = 0;

		virtual void addDebugSphere(const DirectX::SimpleMath::Matrix& worldTransform, float radius, const DirectX::SimpleMath::Color& color) = 0;
		virtual void addDebugBoundingBox(const DirectX::BoundingOrientedBox& boundingOrientedBox, const DirectX::SimpleMath::Color& color) = 0;
		virtual void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const DirectX::SimpleMath::Color& color) = 0;
		virtual void clearDebugShapes() = 0;
	};

	class Entity_render_manager : public IEntity_render_manager
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
			std::unique_ptr<Renderer_data> rendererData{};
		};

		struct Camera_data
		{
			DirectX::SimpleMath::Matrix worldMatrix;
			DirectX::SimpleMath::Matrix viewMatrix;
			DirectX::SimpleMath::Matrix projectionMatrix;

			static const Camera_data identity;
		};

		template<class TData, class... TRender_passes>
		class Render_step {
		public:
			explicit Render_step(std::shared_ptr<TData> data) : data(data) {}

			bool enabled = true;
			std::shared_ptr<TData> data;
			std::tuple<TRender_passes...> renderPasses;
		};

	public:
		Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository, DX::DeviceResources& deviceResources);

		void render() override;
		BoundingFrustumRH createFrustum(const Entity& entity, const Camera_component& cameraComponent) override;
		
		// Manager_base implementations
		void initialize() override;
		void shutdown() override;

		// Manager_tickable implementation
		void tick() override;

		// Manager_windowDependent implementation
		void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
		void destroyWindowSizeDependentResources() override;
		
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		void addDebugSphere(const DirectX::SimpleMath::Matrix& worldTransform, float radius, const DirectX::SimpleMath::Color& color) override;
		void addDebugBoundingBox(const DirectX::BoundingOrientedBox& boundingOrientedBox, const DirectX::SimpleMath::Color& color) override;
		void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const DirectX::SimpleMath::Color& color) override;
		void clearDebugShapes() override;

	protected:

		using Light_data_provider = std::function<const Render_light_data*(const Entity&)>;
		template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
		void render(Entity* entity,
			Buffer_array_set& bufferArraySet,
			const Camera_data& cameraData,
			const Light_data_provider& lightDataProvider,
			const Render_pass_info<TBlend_mode, TDepth_mode>& renderPassInfo);
		
		void createRenderSteps();
		void renderLights();

	private:

		void clearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) const;

		template<int TIdx = 0, class TData, class... TRender_passes>
		void renderStep(Render_step<TData, TRender_passes...>& step);

		template<Render_pass_blend_mode TBlend_mode, Render_pass_depth_mode TDepth_mode>
		void renderPass(Render_pass_info<TBlend_mode, TDepth_mode>& renderPassInfo);
				
		std::unique_ptr<Material> loadMaterial(const std::string& materialName) const;

		std::shared_ptr<D3D_buffer> createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const;
		std::unique_ptr<Renderer_data> createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const;

		void loadRendererDataToDeviceContext(const Renderer_data& rendererData, Buffer_array_set& bufferArraySet) const;

		void drawRendererData(
			const Camera_data& cameraData,
			const DirectX::SimpleMath::Matrix& worldTransform,
			const Renderer_data& rendererData,
			const Render_pass_blend_mode blendMode,
			const Render_light_data& renderLightData,
			Material& material,
			bool wireframe,
			Buffer_array_set& bufferArraySet) const;

		// renders a full screen quad that sets our output buffers to decent default values.
		// Takes ownership of the passed in material
		Renderable initScreenSpaceQuad(std::shared_ptr<Material> material) const;		


		DX::DeviceResources& _deviceResources;
		std::unique_ptr<DirectX::CommonStates> _commonStates;

		// Entities
		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;
				
		std::shared_ptr<IMaterial_repository> _materialRepository;
		std::unique_ptr<Primitive_mesh_data_factory> _primitiveMeshDataFactory;

		bool _enableDeferredRendering;
		bool _fatalError;

		std::unique_ptr<Entity_alpha_sorter> _alphaSorter;
		std::unique_ptr<Entity_cull_sorter> _cullSorter;
			
		Camera_data _cameraData;
		
		// RenderStep definitions
		struct Render_step_shadowmap_data {
			
		};
		struct Render_step_deferred_data {
			std::shared_ptr<Deferred_light_material> _deferredLightMaterial;
			Renderable _pass0ScreenSpaceQuad;
			Renderable _pass2ScreenSpaceQuad;
		};
		struct Render_step_empty_data {};
		struct Render_step_debug_data {
			std::shared_ptr<Unlit_material> _unlitMaterial;
			std::vector<std::tuple<DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Color, std::shared_ptr<Renderer_data>>> _debugShapes;
		};

		Render_step<
			Render_step_shadowmap_data,
			Render_pass_info<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled>
		> _renderPass_shadowMap;

		Render_step<
			Render_step_deferred_data,
			Render_pass_info<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled>,
			Render_pass_info<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>,
			Render_pass_info<Render_pass_blend_mode::Additive, Render_pass_depth_mode::Disabled>
		> _renderPass_entityDeferred;

		Render_step<
			Render_step_empty_data,
			Render_pass_info<Render_pass_blend_mode::Blended_alpha, Render_pass_depth_mode::ReadWrite>
		> _renderPass_entityStandard;

		Render_step<
			Render_step_debug_data,
			Render_pass_info<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>
		> _renderPass_debugElements;

		std::shared_ptr<Texture> _depthTexture;

		struct Render_stats {
			int opaqueEntityCount = 0;
			int opaqueLightCount = 0;
			int alphaEntityCount = 0;
		};
		Render_stats _renderStats;

		// The template arguments here must match the size of the lights array in the shader constant buffer files.
		std::unique_ptr<Render_light_data_impl<0>> _clearGBufferMaterial_renderLightData;
		std::unique_ptr<Render_light_data_impl<8>> _pbrMaterial_forward_renderLightData;
		std::unique_ptr<Render_light_data_impl<0>> _pbrMaterial_deferred_renderLightData;
		std::unique_ptr<Render_light_data_impl<8>> _deferredLightMaterial_renderLightData;
		std::unique_ptr<Render_light_data_impl<0>> _unlitMaterial_renderLightData;
	};
}
