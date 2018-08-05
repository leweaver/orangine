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
using namespace SimpleMath;
using namespace oe;

Entity_scripting_manager::Entity_scripting_manager(Scene &scene)
	: IEntity_scripting_manager(scene)
{
}

void Entity_scripting_manager::initialize()
{
	_scriptableEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Test_component::type() });
	_scriptData.yaw = XM_PI;
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
		entity.setRotation(Quaternion::CreateFromYawPitchRoll(animTimeYaw, animTimePitch, animTimeRoll));
	}
	
	const auto mouseSpeed = 1.0f / 300.0f;
	const auto mouseState = _scene.manager<IInput_manager>().mouseState().lock();
	if (mouseState) {
		if (mouseState->left == Input_manager::Mouse_state::Button_state::HELD) {
			_scriptData.yaw += -mouseState->deltaPosition.x * XM_2PI * mouseSpeed;
			_scriptData.pitch += mouseState->deltaPosition.y * XM_2PI * mouseSpeed;

			_scriptData.pitch = std::max(XM_PI * -0.45f, std::min(XM_PI * 0.45f, _scriptData.pitch));

		}
		//if (mouseState->middle == Input_manager::Mouse_state::Button_state::HELD) {
			_scriptData.distance = std::max(1.0f, std::min(40.0f, _scriptData.distance + static_cast<float>(mouseState->scrollWheelDelta) * -mouseSpeed));
		//}

		const auto deltaRot = Quaternion::CreateFromYawPitchRoll(_scriptData.yaw, _scriptData.pitch, 0.0f);			
		auto cameraPosition = Vector3(DirectX::XMVector3Rotate(XMLoadFloat3(&Vector3::Forward), XMLoadFloat4(&deltaRot)));
		cameraPosition *= _scriptData.distance;

		auto entity = _scene.mainCamera();
		if (entity != nullptr) {
			entity->setPosition(cameraPosition);
			entity->lookAt(Vector3::Zero, Vector3::Up);
		}
	}
}

void Entity_scripting_manager::shutdown()
{
}