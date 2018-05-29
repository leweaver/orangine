#include "pch.h"
#include "Entity.h"
#include "Scene_graph_manager.h"
#include "Scene.h"

using namespace oe;
using namespace DirectX;
using namespace SimpleMath;

Entity::Entity(Scene& scene, std::string&& name, Id_type id)
	: _worldTransform(Matrix::Identity)
	, _worldRotation(Quaternion::Identity)
	, _worldScale(Vector3::One)
	, _localRotation(Quaternion::Identity)
	, _localPosition(Vector3::Zero)
	, _localScale(Vector3::One)
	, _id(id)
	, _name(std::move(name))
	, _state(Entity_state::Uninitialized)
	, _active(true)
	, _parent(nullptr)
	, _scene(scene)
{
}

void Entity::computeWorldTransform()
{
	if (hasParent())
	{
		_worldScale = _parent->_worldScale * _localScale;
		_worldRotation = Quaternion::Concatenate(_parent->worldRotation(), _localRotation);

		const auto worldPosition = Vector3::Transform(_localPosition, _parent->_worldTransform);

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

void Entity::update()
{
	computeWorldTransform();
	if (!_children.empty())
	{
		for (auto const& child : _children)
		{
			if (child->isActive())
				child->update();
		}
	}
}

Component& Entity::getComponent(size_t index) const
{
	return *_components[index];
}

void Entity::lookAt(const Entity& other)
{
	lookAt(other.position(), Vector3::Up);
}

void Entity::lookAt(const Vector3& position, const Vector3 &worldUp)
{
	Vector3 forward;
	if (hasParent()) {
		const auto parentWorldInv = _parent->_worldTransform.Invert();
		forward = Vector3::Transform(position, parentWorldInv) - _localPosition;
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
	
	Vector3 right;
	forward.Cross(worldUp, right);
	right.Normalize();

	Vector3 up;
	right.Cross(forward, up);

	Matrix camToWorld;
	camToWorld._11 = right.x;
	camToWorld._12 = right.y;
	camToWorld._13 = right.z;
	camToWorld._21 = up.x;
	camToWorld._22 = up.y;
	camToWorld._23 = up.z;
	camToWorld._31 = -forward.x;
	camToWorld._32 = -forward.y;
	camToWorld._33 = -forward.z;

	_localRotation = Quaternion::CreateFromRotationMatrix(camToWorld);

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

	auto &entityManager = _scene.sceneGraphManager();
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

	auto &entityManager = _scene.sceneGraphManager();
	const auto thisPtr = entityManager.getEntityPtrById(getId());
	entityManager.addToRoot(thisPtr);
	_parent = nullptr;
}

Vector3 Entity::worldPosition() const
{
	return _worldTransform.Translation();
}

const Vector3& Entity::worldScale() const
{
	return _worldScale;
}

const Quaternion& Entity::worldRotation() const
{
	return _worldRotation;
}

void Entity::onComponentAdded(Component& component)
{
	_scene.onComponentAdded(*this, component);
}

Entity& EntityRef::get() const
{
	const auto ptr = scene.sceneGraphManager().getEntityPtrById(id);
	if (!ptr)
		throw std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(id) + ")");
	return *ptr.get();
}
