﻿#include "pch.h"
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

		const auto localT = XMMatrixTranslationFromVector(worldPosition);
		const auto localR = XMMatrixRotationQuaternion(m_worldRotation);
		const auto localS = XMMatrixScalingFromVector(m_worldScale);

		// TODO: Something is WRONG HERE... Need to be different to work!?
		m_worldTransform = XMMatrixMultiply(XMMatrixMultiply(localS, localR), localT);		
	}
	else
	{
		m_worldRotation = m_localRotation;
		m_worldScale = m_localScale;

		const auto localT = XMMatrixTranslationFromVector(m_localPosition);
		const auto localR = XMMatrixRotationQuaternion(m_worldRotation);
		const auto localS = XMMatrixScalingFromVector(m_worldScale);
		m_worldTransform = XMMatrixMultiply(XMMatrixMultiply(localT, localR), localS);
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
	// Generate a world transform matrix
	if (false && HasParent())
	{
		// TODO: This is clearly broken, as it doesn't take into account the target position!
		const auto worldInv = XMMatrixInverse(nullptr, m_worldTransform);
		const auto worldRotInv = XMQuaternionRotationMatrix(worldInv);
		m_localRotation = XMQuaternionMultiply(worldRotInv, m_localRotation);
	}
	else
	{
		const auto laMat = XMMatrixLookAtRH(Position(), position, Vector3::Up);
		m_localRotation = XMQuaternionRotationMatrix(laMat);
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
	if (m_parent)
		return Vector3::Transform(m_localPosition, m_parent->WorldTransform());
	else
		return m_localPosition;
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
