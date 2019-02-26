#pragma once
#include "IDev_tools_manager.h"
#include "IScene_graph_manager.h"

namespace oe::internal {
	class Dev_tools_manager : public IDev_tools_manager {
	public:
		explicit Dev_tools_manager(Scene& scene)
			: IDev_tools_manager(scene)
			, _unlitMaterial(nullptr)
            , _vectorLog(nullptr)
            , _scrollLogToBottom(true)
		{}

		// Manager_base implementation
		void initialize() override;
		void shutdown() override;

		// Manager_deviceDependent implementation
		void createDeviceDependentResources(DX::DeviceResources& deviceResources) override;
		void destroyDeviceDependentResources() override;
		
		// IEntity_render_manager implementation
		void addDebugSphere(const DirectX::SimpleMath::Matrix& worldTransform, float radius, const DirectX::SimpleMath::Color& color) override;
		void addDebugBoundingBox(const DirectX::BoundingOrientedBox& boundingOrientedBox, const DirectX::SimpleMath::Color& color) override;
		void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const DirectX::SimpleMath::Color& color) override;
		void clearDebugShapes() override;
		void renderDebugShapes(const Render_pass::Camera_data& cameraData) override;
        void renderImGui() override;
        void setVectorLog(VectorLog* vectorLog) override { _vectorLog = vectorLog; }

	private:

		using LightProvider = std::function<void(const DirectX::BoundingSphere& target, std::vector<Entity*>& lights, uint8_t maxLights)>;

		std::shared_ptr<Unlit_material> _unlitMaterial;
		std::vector<std::tuple<DirectX::SimpleMath::Matrix, DirectX::SimpleMath::Color, Renderable>> _debugShapes;
		LightProvider _noLightProvider;

        VectorLog* _vectorLog;
        std::string _consoleInput;
        bool _scrollLogToBottom;
        std::vector<std::string> _commandSuggestions;
	    std::shared_ptr<Entity_filter> _animationControllers;
	};
}
