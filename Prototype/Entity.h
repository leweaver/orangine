﻿#pragma once

#include <DirectXMath.h>
#include <vector>
#include <map>

#include "Component.h"

namespace OE {

	class EntityRepository;
	class Scene;
	class SceneGraphManager;

	enum class EntityState : uint8_t
	{
		UNINITIALIZED,
		INITIALIZED,
		READY,
		DESTROYED
	};

	/**
	 * A node in the scenegraph. This class is not designed to be extended via polymorphism; 
	 * functionality and behaviors should be added in the form of Components (for storing state)
	 * and GameServices (for executing behavior).
	 */
	class Entity
	{
	public:
		typedef unsigned int ID_TYPE;

	private:
		// TODO: Refactor into a public & private interface, so that friend isn't required.
		friend class EntityRepository;
		friend class SceneGraphManager;

		typedef std::vector<std::shared_ptr<Entity>> EntityPtrVec;
		typedef std::map<ID_TYPE, std::shared_ptr<Entity>> EntityPtrMap;

		// Generated by a call to ComputeWorldTransform
		DirectX::SimpleMath::Matrix m_worldTransform;
		DirectX::SimpleMath::Quaternion m_worldRotation;
		DirectX::SimpleMath::Vector3 m_worldScale;		// w is ignored

		DirectX::SimpleMath::Quaternion m_localRotation;
		DirectX::SimpleMath::Vector3 m_localPosition;   // w is ignored
		DirectX::SimpleMath::Vector3 m_localScale;		// w is ignored

		ID_TYPE m_id;
		const std::string m_name;
		EntityState m_state;
		bool m_active;

		EntityPtrVec m_children;
		Entity *m_parent;
		Scene &m_scene;

		std::vector<std::unique_ptr<Component>> m_components;

		Entity(Scene &scene, const std::string &name, ID_TYPE id);
		
	public:

		// Don't allow direct copy of Entity objects (we have a unique_ptr list of components).
		Entity(const Entity& that) = delete;

		/* If true, Update method will be called by parent during recursion. */
		bool IsActive() const { return m_active; }
		void SetActive(bool bActive);
		
		EntityState GetState() const { return m_state; }
	private: 
		void SetState(EntityState state);

	public:

		void RemoveParent();
		bool HasParent() const { return m_parent != nullptr; }
		void SetParent(Entity& newParent);

		/**
		* Applies transforms recursively down (from root -> leaves),
		* then updates components from bottom up (from leaves -> root)
		*/
		void Update();

		/**
		 * Computes the world transformation matrix for just this entity. (Non-recursive)
		 */
		void ComputeWorldTransform();

		const ID_TYPE& GetId() const { return m_id; }
		size_t GetComponentCount() const { return m_components.size(); }

		Component& GetComponent(size_t index) const;

		template<typename TComponent>
		std::vector<std::reference_wrapper<TComponent>> GetComponentsOfType() const;

		/* Math Functions */
		void LookAt(const Entity& other);

		/**
		 * returns a nullptr if no component of given type was found.
		 */
		template<typename TComponent>
		TComponent *GetFirstComponentOfType() const;

		template<typename TComponent>
		TComponent &AddComponent();
		
		const DirectX::SimpleMath::Matrix &GetWorldTransform() const
		{
			return m_worldTransform;
		}

		// Rotation from the forward vector, in local space
		const DirectX::SimpleMath::Quaternion &Rotation() const
		{
			return m_localRotation;
		}

		// Rotation from the forward vector, in local space
		void SetRotation(const DirectX::SimpleMath::Quaternion &xmvector)
		{
			m_localRotation = xmvector;
		}

		// Translation from the origin, in local space
		const DirectX::SimpleMath::Vector3 &Position() const
		{
			return m_localPosition;
		}

		// Translation from the origin, in local space
		void SetPosition(const DirectX::SimpleMath::Vector3 &xmvector)
		{
			m_localPosition = xmvector;
		}

		// Scale, in local space
		const DirectX::SimpleMath::Vector3 &Scale() const
		{
			return m_localScale;
		}

		// Scale, in local space
		void SetScale(const DirectX::SimpleMath::Vector3 &xmvector)
		{
			m_localScale = xmvector;
		}

		Scene& GetScene() const
		{
			return m_scene;
		}

		const DirectX::SimpleMath::Vector3 &WorldScale() const;
		DirectX::SimpleMath::Vector3 WorldPosition() const;
		const DirectX::SimpleMath::Quaternion &WorldRotation() const;

	private:
		/*
		* Redirect events to the Scene.
		*/
		void OnComponentAdded(Component& component);
	};

	template <typename TComponent>
	std::vector<std::reference_wrapper<TComponent>> Entity::GetComponentsOfType() const
	{
		std::vector<std::reference_wrapper<TComponent>> comps;
		for (auto iter = m_components.begin(); iter != m_components.end(); ++iter)
		{
			TComponent* comp = dynamic_cast<TComponent*>((*iter).get());
			if (comp != nullptr)
				comps.push_back(std::reference_wrapper<TComponent>(*comp));
		}
		return comps;
	}

	template <typename TComponent>
	TComponent* Entity::GetFirstComponentOfType() const
	{
		for (auto iter = m_components.begin(); iter != m_components.end(); ++iter)
		{
			const auto comp = dynamic_cast<TComponent*>((*iter).get());
			if (comp != nullptr)
				return comp;
		}
		return nullptr;
	}

	template <typename TComponent>
	TComponent& Entity::AddComponent()
	{
		TComponent* component = new TComponent();
		m_components.push_back(std::unique_ptr<Component>(component));

		this->OnComponentAdded(*component);

		return *component;
	}

	struct EntityRef
	{
		Scene &scene;
		Entity::ID_TYPE id;

		EntityRef(Entity &entity)
			: scene(entity.GetScene())
			, id(entity.GetId())
		{}

		EntityRef(Scene &scene, Entity::ID_TYPE id)
			: scene(scene),
			  id(id)
		{}

		Entity &Get() const;

		Entity &operator *() const
		{
			return Get();
		}
	};
}

