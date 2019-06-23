#include "pch.h"
#include "OeCore/Skinned_mesh_component.h"

using namespace oe;

DEFINE_COMPONENT_TYPE(Skinned_mesh_component);

void Skinned_mesh_component::setJoints(std::vector<std::shared_ptr<Entity>>&& vector)
{
    _joints = move(vector);
    _jointsRaw.resize(_joints.size());
    std::transform(_joints.begin(), _joints.end(), _jointsRaw.begin(), [](const auto& ptr) { return ptr.get(); });
}

void Skinned_mesh_component::setInverseBindMatrices(std::vector<DirectX::SimpleMath::Matrix>&& matrices)
{
    _inverseBindMatrices = std::move(matrices);
}
