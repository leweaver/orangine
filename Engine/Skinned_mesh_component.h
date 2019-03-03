#pragma once
#include "Component.h"

namespace oe {
    class Entity;

    class Skinned_mesh_component : public Component {
        DECLARE_COMPONENT_TYPE;

    public:
        explicit Skinned_mesh_component(Entity& entity) : Component(entity) {}

        const std::shared_ptr<Entity>& skeletonTransformRoot() const { return _skeletonTransformRoot; }
        void setSkeletonTransformRoot(std::shared_ptr<Entity> root) { _skeletonTransformRoot = root; }

        const std::vector<Entity*>& joints() const { return _jointsRaw; }
        void setJoints(std::vector<std::shared_ptr<Entity>>&& vector);

        const std::vector<DirectX::SimpleMath::Matrix>& inverseBindMatrices() const { return _inverseBindMatrices; }
        void setInverseBindMatrices(std::vector<DirectX::SimpleMath::Matrix>&& matrices);

    private:
        // If null, this entity is used as the transform root.
        std::shared_ptr<Entity> _skeletonTransformRoot;

        // Pointers to the joints that make up the skeleton. The order and size of this array
        // should match the inverse bind matrices array.
        std::vector<std::shared_ptr<Entity>> _joints;
        // A fast lookup
        std::vector<Entity*> _jointsRaw;

        std::vector<DirectX::SimpleMath::Matrix> _inverseBindMatrices;
    };
}
