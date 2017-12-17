#include "pch.h"
#include "SceneGraphManager.h"
#include "EntityScriptingManager.h"
#include "TestComponent.h"
#include "Scene.h"
#include "EntityFilter.h"
#include "Entity.h"

#include <cmath>
#include <set>

using namespace DirectX;
using namespace OE;

EntityScriptingManager::EntityScriptingManager(Scene &scene)
	: ManagerBase(scene)
{
}


EntityScriptingManager::~EntityScriptingManager()
{
}

void EntityScriptingManager::Initialize()
{
	std::set<Component::ComponentType> filter;
	filter.insert(TestComponent::Type());
	m_scriptableEntityFilter = m_scene.GetSceneGraphManager().GetEntityFilter(filter);
}

void EntityScriptingManager::Tick() {
	
	double elapsedTime = m_scene.GetElapsedTime();
	for (auto iter = m_scriptableEntityFilter->begin(); iter != m_scriptableEntityFilter->end(); ++iter) {
		Entity &entity = **iter;
		TestComponent *component = entity.GetFirstComponentOfType<TestComponent>();

		const XMVECTOR &speed = component->GetSpeed();

		float animTimeRoll = static_cast<float>(std::fmod(elapsedTime * XMVectorGetX(speed) * XM_2PI, XM_2PI));
		float animTimePitch = static_cast<float>(std::fmod(elapsedTime * XMVectorGetY(speed) * XM_2PI, XM_2PI));
		float animTimeYaw = static_cast<float>(std::fmod(elapsedTime * XMVectorGetZ(speed) * XM_2PI, XM_2PI));

		entity.SetLocalRotation(DirectX::XMQuaternionRotationRollPitchYaw(
			animTimeRoll,
			animTimePitch,
			animTimeYaw));
	}
}