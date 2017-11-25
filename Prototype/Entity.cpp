#include "pch.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Constants.h"

using namespace OE;

void Entity::ApplyWorldTransform()
{
	const auto localTransform = DirectX::XMMatrixTransformation(
		Math::VEC_ZERO, Math::QUAT_IDENTITY, m_localScale,
		Math::VEC_ZERO, m_localRotation,
		m_localPosition);

	if (HasParent())
	{
		m_worldTransform = XMMatrixMultiply(m_parent->m_worldTransform, localTransform);
	}
	else
	{
		m_worldTransform = localTransform;
	}
}

void Entity::Update()
{
	ApplyWorldTransform();
	if (!m_children.empty())
	{
		for (auto const& child : m_children)
		{
			if (child->IsActive())
				child->Update();
		}
	}
}

Component& Entity::GetComponent(unsigned int index) const
{
	return *m_components[index].get();
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

	const std::shared_ptr<Entity> thisPtr = m_entityManager.RemoveFromRoot(*this);
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

	m_entityManager.AddToRoot(*this);
	m_parent = nullptr;
}
