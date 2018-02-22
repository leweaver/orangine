#include "pch.h"
#include "Entity.h"
#include "SceneGraphManager.h"
#include "Scene.h"

using namespace OE;
using namespace DirectX;
using namespace SimpleMath;

Entity::Entity(Scene &scene, const std::string &name, ID_TYPE id)
	: m_worldTransform(Matrix::Identity)
	, m_worldRotation(Quaternion::Identity)
	, m_worldScale(Vector3::One)
	, m_localRotation(Quaternion::Identity)
	, m_localPosition(Vector3::Zero)
	, m_localScale(Vector3::One)
	, m_id(id)
	, m_name(name)
	, m_state(EntityState::UNINITIALIZED)
	, m_active(true)
	, m_parent(nullptr)
	, m_scene(scene)
{
}

void Entity::ComputeWorldTransform()
{
	if (HasParent())
	{
		m_worldScale = m_parent->m_worldScale * m_localScale;
		m_worldRotation = Quaternion::Concatenate(m_parent->WorldRotation(), m_localRotation);

		const auto worldPosition = Vector3::Transform(m_localPosition, m_parent->m_worldTransform);

		const auto worldT = XMMatrixTranslationFromVector(worldPosition);
		const auto worldR = XMMatrixRotationQuaternion(m_worldRotation);
		const auto worldS = XMMatrixScalingFromVector(m_worldScale);
		
		m_worldTransform = XMMatrixMultiply(XMMatrixMultiply(worldS, worldR), worldT);
	}
	else
	{
		m_worldRotation = m_localRotation;
		m_worldScale = m_localScale;

		const auto localT = XMMatrixTranslationFromVector(m_localPosition);
		const auto localR = XMMatrixRotationQuaternion(m_worldRotation);
		const auto localS = XMMatrixScalingFromVector(m_worldScale);
		m_worldTransform = XMMatrixMultiply(XMMatrixMultiply(localS, localR), localT);
	}
}

void Entity::Update()
{
	ComputeWorldTransform();
	if (!m_children.empty())
	{
		for (auto const& child : m_children)
		{
			if (child->IsActive())
				child->Update();
		}
	}
}

Component& Entity::GetComponent(size_t index) const
{
	return *m_components[index].get();
}

void Entity::LookAt(const Entity& other)
{
	LookAt(other.Position());
}

void Entity::LookAt(const Vector3 &position)
{
	const auto worldPosition = WorldPosition();
	if ((worldPosition - position).LengthSquared() == 0)
		return;

	// This seems to produce Left Handed results.??
	//const auto laMat = Matrix::CreateLookAt(worldPosition, position, Vector3::Up);

	auto from = worldPosition, to = position;
	Vector3 forward = (worldPosition - position);
	forward.Normalize();
	Vector3 right;
	Vector3::Up.Cross(forward, right);
	Vector3 up;
	forward.Cross(right, up);

	Matrix camToWorld;
	camToWorld._11 = right.x;
	camToWorld._12 = right.y;
	camToWorld._13 = right.z;
	camToWorld._21 = up.x;
	camToWorld._22 = up.y;
	camToWorld._23 = up.z;
	camToWorld._31 = forward.x;
	camToWorld._32 = forward.y;
	camToWorld._33 = forward.z;

	//camToWorld._41 = worldPosition.x;
	//camToWorld._42 = worldPosition.y;
	//camToWorld._43 = worldPosition.z;

	m_localRotation = Quaternion::CreateFromRotationMatrix(camToWorld);

	// Generate a world transform matrix
	if (HasParent())
	{
		Quaternion parentRotationInv;
		m_parent->WorldRotation().Inverse(parentRotationInv);
		m_localRotation = Quaternion::Concatenate(m_localRotation, parentRotationInv);
	}
}

void Entity::SetActive(bool bActive)
{
	m_active = bActive;
}

void Entity::SetParent(Entity& newParent)
{
	// TODO: Check that we don't create a cycle?
	if (HasParent())
	{
		RemoveParent();
	}

	auto &entityManager = m_scene.GetSceneGraphManager();
	const auto thisPtr = entityManager.GetEntityPtrById(GetId());
	entityManager.RemoveFromRoot(thisPtr);

	newParent.m_children.push_back(thisPtr);
	m_parent = &newParent;
}

void Entity::RemoveParent()
{
	if (m_parent == nullptr)
		return;

	EntityPtrVec& children = m_parent->m_children;
	for (auto it = children.begin(); it != children.end(); ++it) {
		const auto child = (*it).get();
		if (child->GetId() == GetId()) {
			children.erase(it);
			break;
		}
	}

	auto &entityManager = m_scene.GetSceneGraphManager();
	const auto thisPtr = entityManager.GetEntityPtrById(GetId());
	entityManager.AddToRoot(thisPtr);
	m_parent = nullptr;
}

Vector3 Entity::WorldPosition() const
{
	return m_worldTransform.Translation();
}

const Vector3 &Entity::WorldScale() const
{
	return m_worldScale;
}

const Quaternion &Entity::WorldRotation() const
{
	return m_worldRotation;
}

void Entity::OnComponentAdded(Component& component)
{
	m_scene.OnComponentAdded(*this, component);
}

Entity &EntityRef::Get() const
{
	const auto ptr = scene.GetSceneGraphManager().GetEntityPtrById(id);
	if (!ptr)
		throw std::runtime_error("Attempting to access deleted Entity (id=" + std::to_string(id) + ")");
	return *ptr.get();
}
