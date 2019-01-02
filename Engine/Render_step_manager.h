#pragma once

#include "Manager_base.h"
#include "Render_pass_config.h"
#include "Render_pass.h"
#include "Renderable.h"

#include <memory>
#include <array>
#include "Light_provider.h"

namespace oe {
	class Camera_component;
	class Deferred_light_material;
	class Render_light_data;
	class Unlit_material;
	struct Renderer_data;
	class Entity_alpha_sorter;
	class Entity_cull_sorter;
	class Entity_filter;
	class Entity;

	class IRender_step_manager:
		public Manager_base,
		public Manager_deviceDependent,
		public Manager_windowDependent
	{
	public:
		IRender_step_manager(Scene& scene)
			: Manager_base(scene)
		{}

		template<class TData, class... TRender_passes>
		class Render_step {
		public:
			explicit Render_step(std::shared_ptr<TData> data) : data(data) {}

			bool enabled = true;
			std::shared_ptr<TData> data;
			std::tuple<TRender_passes...> renderPassConfigs;
			std::array<std::unique_ptr<Render_pass>, sizeof...(TRender_passes)> renderPasses;
		};
		
		// IRender_step_manager
		virtual void createRenderSteps() = 0;
		virtual Render_pass::Camera_data createCameraData(Camera_component& component) = 0;
		virtual void render(std::shared_ptr<Entity> cameraEntity) = 0;

	};

	class Render_step_manager : public IRender_step_manager {
	public:
		explicit Render_step_manager(Scene& scene);

		// Manager_base implementations
		void initialize() override;
		void shutdown() override;

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		// Manager_windowDependent implementation
		void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
		void destroyWindowSizeDependentResources() override;

		// IRender_step_manager implementation
		void createRenderSteps() override;
		Render_pass::Camera_data createCameraData(Camera_component& component) override;
		void render(std::shared_ptr<Entity> cameraEntity) override;

	private:

		DX::DeviceResources& deviceResources() const;
		void clearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) const;
		
		template<int TRender_pass_idx = 0, class TData, class... TRender_passes>
		void createRenderStepResources(Render_step<TData, TRender_passes...>& step);

		template<int TRender_pass_idx = 0, class TData, class... TRender_passes>
		void destroyRenderStepResources(Render_step<TData, TRender_passes...>& step);

		template<int TRender_pass_idx = 0, class TData, class... TRender_passes>
		void renderStep(Render_step<TData, TRender_passes...>& step,
			const Render_pass::Camera_data& cameraData);

		template<
			Render_pass_blend_mode TBlend_mode,
			Render_pass_depth_mode TDepth_mode,
			Render_pass_stencil_mode TStencil_mode,
			uint32_t TStencil_read_mask,
			uint32_t TStencil_write_mask
		>
		void renderPass(Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask, TStencil_write_mask>& renderPassInfo,
			Render_pass& renderPass,
			const Render_pass::Camera_data& cameraData);

		template<
			Render_pass_blend_mode TBlend_mode,
			Render_pass_depth_mode TDepth_mode,
			Render_pass_stencil_mode TStencil_mode,
			uint32_t TStencil_read_mask,
			uint32_t TStencil_write_mask
		>
		void renderEntity(
			Entity* entity,
			const Render_pass::Camera_data& cameraData,
			Light_provider::Callback_type& lightProvider,
			const Render_pass_config<TBlend_mode, TDepth_mode, TStencil_mode, TStencil_read_mask, TStencil_write_mask>& renderPassInfo);

		void renderLights(const Render_pass::Camera_data& cameraData, Render_pass_blend_mode blendMode);

		// RenderStep definitions
		struct Render_step_deferred_data {
			std::shared_ptr<Deferred_light_material> deferredLightMaterial;
			Renderable pass0ScreenSpaceQuad;
			Renderable pass2ScreenSpaceQuad;
		};
		struct Render_step_empty_data {};

		Render_step<
			Render_step_empty_data,
			Render_pass_config<
				Render_pass_blend_mode::Opaque,
				Render_pass_depth_mode::ReadWrite,
				Render_pass_stencil_mode::Enabled,
				0xFF,
				0xFF
			>
		> _renderStep_shadowMap;

		Render_step<
			Render_step_deferred_data,
			Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::Disabled>,
			Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>,
			Render_pass_config<Render_pass_blend_mode::Additive, Render_pass_depth_mode::Disabled>
		> _renderStep_entityDeferred;

		Render_step<
			Render_step_empty_data,
			Render_pass_config<Render_pass_blend_mode::Blended_alpha, Render_pass_depth_mode::ReadWrite>
		> _renderStep_entityStandard;

		Render_step<
			Render_step_empty_data,
			Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>
		> _renderStep_debugElements;

		Render_step<
			Render_step_empty_data,
			Render_pass_config<Render_pass_blend_mode::Opaque, Render_pass_depth_mode::ReadWrite>
		> _renderStep_skybox;
		
		// Broad rendering
		std::unique_ptr<Entity_alpha_sorter> _alphaSorter;
		std::unique_ptr<Entity_cull_sorter> _cullSorter;

		// Entities
		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;

		Light_provider::Callback_type _simpleLightProvider;

		bool _fatalError;
		bool _enableDeferredRendering;

	};
}
