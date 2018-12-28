#pragma once

#include "DeviceResources.h"
#include "Manager_base.h"
#include "Renderer_data.h"
#include "Mesh_data_component.h"
#include "Material_repository.h"
#include "Primitive_mesh_data_factory.h"
#include "Deferred_light_material.h"
#include "Render_pass.h"
#include "Render_light_data.h"
#include "Render_pass_config.h"
#include "Collision.h"
#include "Renderable.h"

#include <memory>
#include "Light_provider.h"

namespace oe {
	class Unlit_material;
	class Camera_component;
	class Renderable_component;
	class Scene;
	class Material;
	class Entity_filter;
	class Render_target_texture;
	class Entity_alpha_sorter;
	class Entity_cull_sorter;
	class Shadow_map_texture_pool;

	class IEntity_render_manager :
		public Manager_base,
		public Manager_deviceDependent,
		public Manager_windowDependent
	{
	public:
		
		IEntity_render_manager(Scene& scene) : Manager_base(scene) {}

		virtual BoundingFrustumRH createFrustum(const Camera_component& cameraComponent) = 0;

		virtual void renderRenderable(Renderable& renderable,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			float radius,
			const Render_pass::Camera_data& cameraData,
			const Light_provider::Callback_type& lightDataProvider,
			Render_pass_blend_mode blendMode,
			bool wireFrame
		) = 0;

		virtual void renderEntity(Renderable_component& renderable,
			const Render_pass::Camera_data& cameraData,
			const Light_provider::Callback_type& lightDataProvider,
			Render_pass_blend_mode blendMode
		) = 0;

		virtual Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) const = 0;
		virtual void clearRenderStats() = 0;
	};

	class Entity_render_manager : public IEntity_render_manager
	{
		struct Buffer_array_set {
			std::vector<ID3D11Buffer*> bufferArray;
			std::vector<uint32_t> strideArray;
			std::vector<uint32_t> offsetArray;
		};

	public:

		Entity_render_manager(Scene& scene, std::shared_ptr<IMaterial_repository> materialRepository);

		BoundingFrustumRH createFrustum(const Camera_component& cameraComponent) override;
		
		// Manager_base implementation
		void initialize() override;
		void shutdown() override;
		
		// Manager_windowDependent implementation
		void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
		void destroyWindowSizeDependentResources() override;

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;

		void renderRenderable(Renderable& renderable,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			float radius,
			const Render_pass::Camera_data& cameraData,
			const Light_provider::Callback_type& lightDataProvider,
			Render_pass_blend_mode blendMode,
			bool wireFrame
		) override;

		void renderEntity(Renderable_component& renderable,
			const Render_pass::Camera_data& cameraData,
			const Light_provider::Callback_type& lightDataProvider,
			Render_pass_blend_mode blendMode
			) override;
		Renderable createScreenSpaceQuad(std::shared_ptr<Material> material) const override;
		void clearRenderStats() override;

	protected:

		void drawRendererData(
			const Render_pass::Camera_data& cameraData,
			const DirectX::SimpleMath::Matrix& worldTransform,
			const Renderer_data& rendererData,
			Render_pass_blend_mode blendMode,
			const Render_light_data& renderLightData,
			Material& material,
			bool wireframe);

	private:

		std::unique_ptr<Material> loadMaterial(const std::string& materialName) const;

		std::shared_ptr<D3D_buffer> createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const;
		std::unique_ptr<Renderer_data> createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const;

		void loadRendererDataToDeviceContext(const Renderer_data& rendererData);

		DX::DeviceResources& deviceResources() const;

		// Entities
		std::shared_ptr<IMaterial_repository> _materialRepository;

		struct Render_stats {
			int opaqueEntityCount = 0;
			int opaqueLightCount = 0;
			int alphaEntityCount = 0;
		};

		// Rendering
		Render_stats _renderStats;
		Buffer_array_set _bufferArraySet;
		std::vector<Entity*> _renderLights;

		// The template arguments here must match the size of the lights array in the shader constant buffer files.
		std::unique_ptr<Render_light_data_impl<0>> _renderLightData_unlit;
		std::unique_ptr<Render_light_data_impl<8>> _renderLightData_lit;
	};
}
