#pragma once
#include "OeCore/IAnimation_manager.h"
#include "OeCore/IScene_graph_manager.h"
#include "OeCore/ITime_step_manager.h"
#include "OeCore/Animation_controller_component.h"

namespace oe {
    class Animation_manager : public IAnimation_manager
    {
    public:
        Animation_manager(Scene& scene, IScene_graph_manager& sceneGraphManager, ITime_step_manager& timeStepManager) : IAnimation_manager(scene), _sceneGraphManager(sceneGraphManager), _timeStepManager(timeStepManager) {}

        Animation_manager(const Animation_manager& other) = delete;
        Animation_manager(Animation_manager&& other) = delete;
        void operator=(const Animation_manager& other) = delete;
        void operator=(Animation_manager&& other) = delete;

        // Manager_base implementation
        void initialize() override;
        void shutdown() override;
        const std::string& name() const override;

        // Manager_tickable implementation
        void tick() override;

        static void handleTranslationAnimationStep(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex);
        static void handleRotationAnimationStep(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex);
        static void handleScaleAnimationStep(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex);
        static void handleMorphAnimationStep(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex);

        static void handleTranslationAnimationLerp(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleRotationAnimationLerp(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleScaleAnimationLerp(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleMorphAnimationLerp(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);

        template<class TTypeIn, class TTypeOut>
        static TTypeOut calculateCubicSpline(const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);

        static void handleTranslationAnimationCubicSpline(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleRotationAnimationCubicSpline(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleScaleAnimationCubicSpline(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);
        static void handleMorphAnimationCubicSpline(
            Entity& entity,
            const Animation_controller_component::Animation_channel& channel,
            uint32_t lowerValueIndex, uint32_t upperValueIndex, double factor);

    private:

        static std::string _name;

        IScene_graph_manager& _sceneGraphManager;
        ITime_step_manager& _timeStepManager;
        std::shared_ptr<Entity_filter> _animationControllers;
    };
}
