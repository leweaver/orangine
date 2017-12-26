#include "pch.h"
#include "Entity.h"
#include "SceneGraphManager.h"
#include "Scene.h"
#include "Constants.h"

using namespace OE;
using namespace DirectX;

void Entity::ComputeWorldTransform()
{
	if (HasParent())
	{
		m_worldScale = XMVectorMultiply(m_parent->m_worldScale, m_localScale);
		m_worldRotation = XMQuaternionMultiply(m_localRotation, m_parent->Rotation());
		m_worldTransform = XMMatrixTransformation(
			Math::VEC_ZERO, Math::QUAT_IDENTITY, m_worldScale,
			Math::VEC_ZERO, m_worldRotation,
			XMVector3Transform(m_localPosition, m_parent->m_worldTransform));
	}
	else
	{
		m_worldTransform = XMMatrixTransformation(
			Math::VEC_ZERO, Math::QUAT_IDENTITY, m_localScale,
			Math::VEC_ZERO, m_localRotation,
			m_localPosition);
		m_worldRotation = m_localRotation;
		m_worldScale = m_localScale;
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
	// Generate a world transform matrix
	const auto laMat = XMMatrixLookAtRH(Position(), other.Position(), Math::VEC_UP);

	m_localRotation = XMQuaternionRotationMatrix(laMat);
	if (HasParent())
	{
		const auto worldInv = XMMatrixInverse(nullptr, m_worldTransform);
		const auto worldRotInv = XMQuaternionRotationMatrix(worldInv);
		m_localRotation = XMQuaternionMultiply(worldRotInv, m_localRotation);		
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

XMVECTOR Entity::Position() const
{
	return XMVector4Transform(m_localPosition, m_worldTransform);
}

const XMVECTOR& Entity::Scale() const
{
	return m_worldScale;
}

XMVECTOR Entity::Rotation() const
{
	return m_worldRotation;
	//return XMQuaternionRotationMatrix(m_worldTransform);
}

void Entity::OnComponentAdded(Component& component)
{
	m_scene.OnComponentAdded(*this, component);
}
