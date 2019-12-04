#pragma once
#include "Manager_base.h"
#include "Render_pass.h"
#include "Color.h"
#include "Collision.h"

namespace oe {
    class VectorLog;
    class Entity;
    class Unlit_material;
    class Primitive_mesh_data_factory;
    struct Renderable;
    struct BoundingFrustumRH;

    class IDev_tools_manager :
        public Manager_base,
        public Manager_tickable,
        public Manager_deviceDependent {
    public:
        explicit IDev_tools_manager(Scene& scene) : Manager_base(scene) {}

        virtual void addDebugCone(const SSE::Matrix4& worldTransform, float diameter, float height, const Color& color) = 0;
		virtual void addDebugSphere(const SSE::Matrix4& worldTransform, float radius, const Color& color, size_t tessellation = 6) = 0;
        virtual void addDebugBoundingBox(const oe::BoundingOrientedBox& boundingOrientedBox, const Color& color) = 0;
        virtual void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const Color& color) = 0;
        virtual void clearDebugShapes() = 0;

        virtual void renderDebugShapes(const Render_pass::Camera_data& cameraData) = 0;
        virtual void renderImGui() = 0;

        virtual void setVectorLog(VectorLog* logSink) = 0;
    };
}