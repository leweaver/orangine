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
	m_scriptableEntityFilter = m_scene.GetSceneGraphManager().GetEntityFilter({ TestComponent::Type() });
}

void EntityScriptingManager::Tick() {
	
	const double elapsedTime = m_scene.GetElapsedTime();
	for (auto iter = m_scriptableEntityFilter->begin(); iter != m_scriptableEntityFilter->end(); ++iter) {
		Entity &entity = **iter;
		TestComponent *component = entity.GetFirstComponentOfType<TestComponent>();

		const auto &speed = component->GetSpeed();

		const float animTimePitch = static_cast<float>(fmod(elapsedTime * speed.x * XM_2PI, XM_2PI));
		const float animTimeYaw = static_cast<float>(fmod(elapsedTime * speed.y * XM_2PI, XM_2PI));
		const float animTimeRoll = static_cast<float>(fmod(elapsedTime * speed.z * XM_2PI, XM_2PI));

		entity.SetRotation(SimpleMath::Quaternion::CreateFromYawPitchRoll(animTimeYaw, animTimePitch, animTimeRoll));
	}
}