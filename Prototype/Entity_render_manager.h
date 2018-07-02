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
#include <memory>
#include <array>

namespace DirectX {
	class CommonStates;
	class GeometricPrimitive;
}

namespace oe {
	class Scene;
	class Material;
	class Entity_filter;
	class Render_target_texture;
	class Alpha_sorter;

	class Entity_render_manager : 
		public Manager_base, 
		public Manager_tickable,
		public Manager_deviceDependent,
		public Manager_windowDependent
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
		~Entity_render_manager() = default;
		
		// Manager_base implementation
		void initialize() override;
		void shutdown() override;

		// Manager_tickable implementation
		void tick();

		// Manager_windowDependent implementation
		void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
		void destroyWindowSizeDependentResources() override;
		
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;
		void render();

	protected:
		using Light_data_provider = std::function<const Render_light_data*(const Entity&)>;

		template<Render_pass_output_format TOutput_format>
		void render(Entity* entity, 
			Buffer_array_set& bufferArraySet,
			const Light_data_provider& lightDataProvider,
			const Render_pass_info<TOutput_format>& renderPassInfo);
		
		void renderSimplePrimitives();
		void renderLights();

	private:

		void setDepthEnabled(bool enabled);
		void setBlendEnabled(bool enabled);

		void setupRenderPass_entityDeferred_step1();
		void setupRenderPass_entityDeferred_step2();
		void setupRenderPass_entityStandard();

		std::unique_ptr<Material> loadMaterial(const std::string& materialName) const;

		std::shared_ptr<D3D_buffer> createBufferFromData(const Mesh_buffer& buffer, UINT bindFlags) const;
		std::unique_ptr<Renderer_data> createRendererData(const Mesh_data& meshData, const std::vector<Vertex_attribute>& vertexAttributes) const;

		void loadRendererDataToDeviceContext(const Renderer_data& rendererData, Buffer_array_set& bufferArraySet) const;

		template<Render_pass_output_format TOutput_format>
		void drawRendererData(
			const Camera_data& cameraData,
			const DirectX::SimpleMath::Matrix& worldTransform,
			const Renderer_data& rendererData,
			const Render_pass_info<TOutput_format>& renderPassInfo,
			const Render_light_data& renderLightData,
			Material& material,
			Buffer_array_set& bufferArraySet) const;

		// renders a full screen quad that sets our output buffers to decent default values.
		// Takes ownership of the passed in material
		Renderable initScreenSpaceQuad(std::shared_ptr<Material> material) const;		


		DX::DeviceResources& _deviceResources;
		std::unique_ptr<DirectX::CommonStates> _commonStates;

		// Entities
		std::shared_ptr<Entity_filter> _renderableEntities;
		std::shared_ptr<Entity_filter> _lightEntities;
				
		std::shared_ptr<Material_repository> _materialRepository;
		std::unique_ptr<Primitive_mesh_data_factory> _primitiveMeshDataFactory;
		std::vector<std::unique_ptr<DirectX::GeometricPrimitive>> _simplePrimitives;

		bool _enableDeferredRendering;
		bool _fatalError;
		bool _lastBlendEnabled;

		std::unique_ptr<Alpha_sorter> _alphaSorter;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerStateDepthEnabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilStateDepthDisabled;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _depthStencilStateDepthEnabled;

		// TODO: Use CommonStates::Additive?
		Microsoft::WRL::ComPtr<ID3D11BlendState> _deferredLightBlendState;

		std::vector<std::shared_ptr<Render_target_texture>> _pass1RenderTargets;

		Camera_data _cameraData;
		Renderable _pass1ScreenSpaceQuad;
		Renderable _pass2ScreenSpaceQuad;

		// Render pass definitions
		std::tuple<
			Render_pass_info<Render_pass_output_format::Shaded_Unlit>
		> _renderPass_clear;
		std::tuple<
			Render_pass_info<Render_pass_output_format::Shaded_Unlit>,
			Render_pass_info<Render_pass_output_format::Shaded_DeferredLight>,
			Render_pass_info<Render_pass_output_format::Shaded_Unlit>
		> _renderPass_entityDeferred;
		std::tuple<
			Render_pass_info<Render_pass_output_format::Shaded_StandardLight>
		> _renderPass_entityStandard;

		std::shared_ptr<Texture> _depthTexture;
		std::shared_ptr<Deferred_light_material> _deferredLightMaterial;

		// The template arguments here must match the size of the lights array in the shader constant buffer files.
		std::unique_ptr<Render_light_data_impl<8>> _renderPass_entityStandard_renderLightData;
		std::unique_ptr<Render_light_data_impl<0>> _renderPass_entityDeferred_renderLightData_blank;
		std::unique_ptr<Render_light_data_impl<8>> _renderPass_entityDeferred_renderLightData;
	};
}
