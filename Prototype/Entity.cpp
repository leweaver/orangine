#include "pch.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Scene.h"
#include "Constants.h"

using namespace OE;

void Entity::ComputeWorldTransform()
{
	const auto localTransform = DirectX::XMMatrixTransformation(
		Math::VEC_ZERO, Math::QUAT_IDENTITY, m_localScale,
		Math::VEC_ZERO, m_localRotation,
		m_localPosition);

	if (HasParent())
	{
		m_worldTransform = XMMatrixMultiply(m_parent->m_worldTransform, localTransform);		
		m_worldScale = DirectX::XMVectorMultiply(m_parent->m_worldScale, m_localScale);
	}
	else
	{
		m_worldTransform = localTransform;
		m_worldScale = m_localScale;
	}
}


void Entity::Initialize()
{
	ComputeWorldTransform();
	if (!m_children.empty())
	{
		for (auto const& child : m_children)
		{
			child->Initialize();
		}
	}

	if (!m_components.empty())
	{
		for (auto const& comp : m_components)
		{
			comp.get()->Initialize();
		}
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

	if (!m_components.empty())
	{
		for (auto const& comp : m_components)
		{
			comp.get()->Update();
		}
	}
}

Component& Entity::GetComponent(unsigned int index) const
{
	return *m_components[index].get();
}

void Entity::LookAt(const Entity& other)
{
	// Generate a world transform matrix
	const auto laMat = DirectX::XMMatrixLookAtRH(Position(), other.Position(), Math::VEC_UP);

	m_localRotation = DirectX::XMQuaternionRotationMatrix(laMat);
	if (HasParent())
	{
		const auto worldInv = DirectX::XMMatrixInverse(nullptr, m_worldTransform);
		const auto worldRotInv = XMQuaternionRotationMatrix(worldInv);
		m_localRotation = DirectX::XMQuaternionMultiply(worldRotInv, m_localRotation);		
	}
}

void Entity::SetActive(bool bActive)
{
	m_active = bActive;
}

void Entity::SetParent(Entity& newParent)
{
	if (HasParent())
	{
		RemoveParent();
	}

	const std::shared_ptr<Entity> thisPtr = m_scene.EntityManager().RemoveFromRoot(*this);
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

	m_scene.EntityManager().AddToRoot(*this);
	m_parent = nullptr;
}

DirectX::XMVECTOR Entity::Position() const
{
	return XMVector4Transform(m_localPosition, m_worldTransform);
}

const DirectX::XMVECTOR& Entity::Scale() const
{
	return m_worldScale;
}

DirectX::XMVECTOR Entity::Rotation() const
{
	return XMQuaternionRotationMatrix(m_worldTransform);
}

void Entity::OnComponentAdded(Component& component)
{
	m_scene.OnComponentAdded(*this, component);
}
