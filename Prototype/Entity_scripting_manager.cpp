#include "pch.h"
#include "Scene_graph_manager.h"
#include "Entity_scripting_manager.h"
#include "Test_component.h"
#include "Scene.h"
#include "Entity_filter.h"
#include "Entity.h"

#include <cmath>
#include <set>

using namespace DirectX;
using namespace oe;

Entity_scripting_manager::Entity_scripting_manager(Scene &scene)
	: Manager_base(scene)
{
}

void Entity_scripting_manager::initialize()
{
	_scriptableEntityFilter = _scene.sceneGraphManager().getEntityFilter({ Test_component::type() });
}

void Entity_scripting_manager::tick() {
	
	const double elapsedTime = _scene.elapsedTime();
	for (auto iter = _scriptableEntityFilter->begin(); iter != _scriptableEntityFilter->end(); ++iter) {
		Entity &entity = **iter;
		Test_component *component = entity.getFirstComponentOfType<Test_component>();

		const auto &speed = component->getSpeed();

		const float animTimePitch = static_cast<float>(fmod(elapsedTime * speed.x * XM_2PI, XM_2PI));
		const float animTimeYaw = static_cast<float>(fmod(elapsedTime * speed.y * XM_2PI, XM_2PI));
		const float animTimeRoll = static_cast<float>(fmod(elapsedTime * speed.z * XM_2PI, XM_2PI));

		entity.setRotation(SimpleMath::Quaternion::CreateFromYawPitchRoll(animTimeYaw, animTimePitch, animTimeRoll));
	}
}