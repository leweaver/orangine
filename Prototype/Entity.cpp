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
	
	newParent.m_children.push_back(m_entityManager.TakeRoot(*this));
	m_parent = &newParent;
}

void Entity::RemoveParent()
{
	if (m_parent == nullptr)
		return;

	EntityVector& children = m_parent->m_children;
	for (EntityVector::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		auto &child = *iter;
		if (child->m_id == m_id)
		{
			std::unique_ptr<Entity> newRoot = move(child);
			children.erase(iter);
			m_entityManager.PutRoot(newRoot);
			m_parent = nullptr;
			break;
		}
	}
}
