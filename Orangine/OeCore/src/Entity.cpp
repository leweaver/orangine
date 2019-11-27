#include "pch.h"
#include "OeCore/Entity.h"
#include "Scene_graph_manager.h"
#include "OeCore/Scene.h"

using namespace oe;
using namespace DirectX;

Entity::Entity(Scene& scene, std::string&& name, Id_type id)
	: _localRotation(SimpleMath::Quaternion::Identity)
	, _localPosition(SimpleMath::Vector3::Zero)
	, _localScale(SimpleMath::Vector3::One)
    , _calculateWorldTransform(true)
	, _calculateBoundSphereFromChildren(true)
	, _id(id)
	, _name(std::move(name))
	, _state(Entity_state::Uninitialized)
	, _active(true)
	, _parent(nullptr)
	, _scene(scene)
	, _worldTransform(SimpleMath::Matrix::Identity)
	, _worldRotation(SimpleMath::Quaternion::Identity)
	, _worldScale(SimpleMath::Vector3::One)
{
}

void Entity::computeWorldTransform()
{
	if (hasParent())
	{
		_worldScale = _parent->_worldScale * _localScale;
		_worldRotation = SimpleMath::Quaternion::Concatenate(_parent->worldRotation(), _localRotation);

		const auto worldPosition = SimpleMath::Vector3::Transform(_localPosition, _parent->_worldTransform);

		const auto worldT = XMMatrixTranslationFromVector(worldPosition);
		const auto worldR = XMMatrixRotationQuaternion(_worldRotation);
		const auto worldS = XMMatrixScalingFromVector(_worldScale);
		
		_worldTransform = XMMatrixMultiply(XMMatrixMultiply(worldS, worldR), worldT);
	}
	else
	{
		_worldRotation = _localRotation;
		_worldScale = _localScale;

		const auto localT = XMMatrixTranslationFromVector(_localPosition);
		const auto localR = XMMatrixRotationQuaternion(_worldRotation);
		const auto localS = XMMatrixScalingFromVector(_worldScale);

		_worldTransform = XMMatrixMultiply(XMMatrixMultiply(localS, localR), localT);
	}
}

Component& Entity::getComponent(size_t index) const
{
	return *_components[index];
}

void Entity::lookAt(const Entity& other)
{
	lookAt(other.position(), SimpleMath::Vector3::Up);
}

void Entity::lookAt(const SimpleMath::Vector3& position, const SimpleMath::Vector3 &worldUp)
{
	SimpleMath::Vector3 forward;
	if (hasParent()) {
		const auto parentWorldInv = _parent->_worldTransform.Invert();
		forward = SimpleMath::Vector3::Transform(position, parentWorldInv) - _localPosition;
	}
	else {
		forward = position - _localPosition;
	}

	if (forward.LengthSquared() == 0)
		return;

	// The DirectX LookAt function creates a view matrix; we want to create a transform
	// to rotate this object towards a target. So, we need to create the matrix manually.
	// (This also gives us the benefit of skipping the translation calculations; since they
	// are not used)

	forward.Normalize();
	
	SimpleMath::Vector3 right;
	forward.Cross(worldUp, right);
	right.Normalize();

	SimpleMath::Vector3 up;
	right.Cross(forward, up);

	SimpleMath::Matrix camToWorld;
	camToWorld._11 = right.x;
	camToWorld._12 = right.y;
	camToWorld._13 = right.z;
	camToWorld._21 = up.x;
	camToWorld._22 = up.y;
	camToWorld._23 = up.z;
	camToWorld._31 = -forward.x;
	camToWorld._32 = -forward.y;
	camToWorld._33 = -forward.z;

	_localRotation = SimpleMath::Quaternion::CreateFromRotationMatrix(camToWorld);

	computeWorldTransform();
}

void Entity::setActive(bool bActive)
{
	_active = bActive;
}

void Entity::setParent(Entity& newParent)
{
	// TODO: Check that we don't create a cycle?
	if (hasParent())
	{
		removeParent();
	}

	auto &entityManager = _scene.manager<IScene_graph_manager>();
	const auto thisPtr = entityManager.getEntityPtrById(getId());
	entityManager.removeFromRoot(thisPtr);

	auto newParentPtr = entityManager.getEntityPtrById(newParent.getId());
	newParentPtr->_children.push_back(thisPtr);

	_parent = newParentPtr.get();
}

void Entity::removeParent()
{
	if (_parent == nullptr)
		return;

	Entity_ptr_vec& children = _parent->_children;
	for (auto it = children.begin(); it != children.end(); ++it) {
		const auto child = (*it).get();
		if (child->getId() == getId()) {
			children.erase(it);
			break;
		}
	}

	auto &entityManager = _scene.manager<IScene_graph_manager>();
	const auto thisPtr = entityManager.getEntityPtrById(getId());
	entityManager.addToRoot(thisPtr);
	_parent = nullptr;
}

SimpleMath::Vector3 Entity::worldPosition() const
{
	return _worldTransform.Translation();
}

const SimpleMath::Vector3& Entity::worldScale() const
{
	return _worldScale;
}

const SimpleMath::Quaternion& Entity::worldRotation() const
{
	return _worldRotation;
}

std::shared_ptr<Entity> Entity::verifyEntityPtr() const
{
	const auto thisPtr = _scene.manager<IScene_graph_manager>().getEntityPtrById(getId());
	if (!thisPtr)
		throw std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(getId()) + ")");

	return thisPtr;
}

void Entity::onComponentAdded(Component& component)
{
	_scene.onComponentAdded(*this, component);
}

Entity& EntityRef::get() const
{
	const auto ptr = scene.manager<IScene_graph_manager>().getEntityPtrById(id);
	if (!ptr)
		throw std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(id) + ")");
	return *ptr.get();
}
