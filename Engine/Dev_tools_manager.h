#pragma once
#include "Manager_base.h"
#include "Render_pass.h"

namespace oe {
    class VectorLog;
    class Entity;
	class Unlit_material;
	class Primitive_mesh_data_factory;
	struct Renderable;
	struct BoundingFrustumRH;

	class IDev_tools_manager : 
		public Manager_base,
		public Manager_deviceDependent {
	public:
		explicit IDev_tools_manager(Scene& scene) : Manager_base(scene) {}

		virtual void addDebugSphere(const DirectX::SimpleMath::Matrix& worldTransform, float radius, const DirectX::SimpleMath::Color& color) = 0;
		virtual void addDebugBoundingBox(const DirectX::BoundingOrientedBox& boundingOrientedBox, const DirectX::SimpleMath::Color& color) = 0;
		virtual void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const DirectX::SimpleMath::Color& color) = 0;
		virtual void clearDebugShapes() = 0;

		virtual void renderDebugShapes(const Render_pass::Camera_data& cameraData) = 0;
	    virtual void renderImGui() = 0;

        virtual void setVectorLog(VectorLog* logSink) = 0;
	};

	class Dev_tools_manager : public IDev_tools_manager {
	public:
		explicit Dev_tools_manager(Scene& scene)
			: IDev_tools_manager(scene)
			, _unlitMaterial(nullptr)
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
	};
}
