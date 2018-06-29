#include "pch.h"
#include "Scene_graph_manager.h"
#include "Entity_scripting_manager.h"
#include "Input_manager.h"
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
	_scriptableEntityFilter = _scene.manager<Scene_graph_manager>().getEntityFilter({ Test_component::type() });
}

void Entity_scripting_manager::tick() {
	
	const auto elapsedTime = _scene.elapsedTime();
	for (auto iter = _scriptableEntityFilter->begin(); iter != _scriptableEntityFilter->end(); ++iter) {
		auto& entity = **iter;
		auto* component = entity.getFirstComponentOfType<Test_component>();

		const auto &speed = component->getSpeed();

		const auto animTimePitch = static_cast<float>(fmod(elapsedTime * speed.x * XM_2PI, XM_2PI));
		const auto animTimeYaw = static_cast<float>(fmod(elapsedTime * speed.y * XM_2PI, XM_2PI));
		const auto animTimeRoll = static_cast<float>(fmod(elapsedTime * speed.z * XM_2PI, XM_2PI));

		const auto mouseState = _scene.manager<Input_manager>().mouseState().lock();
		if (mouseState) {
			if (mouseState->left == Input_manager::Mouse_state::Button_state::HELD)
				entity.setRotation(SimpleMath::Quaternion::CreateFromYawPitchRoll(animTimeYaw, animTimePitch, animTimeRoll));
		}
	}
}

void Entity_scripting_manager::shutdown()
{
}