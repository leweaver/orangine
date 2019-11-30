#include "pch.h"

#include "OeCoreConfig.h"

#include "Entity_scripting_manager.h"
#include "Script_runtime_data.h"
#include "OeScripting/Script_component.h"
#include "OeCore/IInput_manager.h"
#include "OeCore/IScene_graph_manager.h"
#include "OeCore/Test_component.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Light_component.h"
#include "OeCore/Shadow_map_texture.h"
#include "OeCore/Scene.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/Entity.h"
#include "OeCore/Math_constants.h"
#include <cmath>
#include <set>

#include <OeThirdParty/imgui.h>
#include <filesystem>

namespace py = pybind11;

using namespace DirectX;
using namespace oe;
using namespace internal;

std::string Entity_scripting_manager::_name = "Entity_scripting_manager";

Entity_scripting_manager::EngineInternalPythonModule::EngineInternalPythonModule(py::module engine_internal)
	: engine_internal(engine_internal)
	, reset_output_streams(engine_internal.attr("reset_output_streams"))
{
}

template<>
IEntity_scripting_manager* oe::create_manager(Scene & scene)
{
    return new Entity_scripting_manager(scene);
}

Entity_scripting_manager::Entity_scripting_manager(Scene &scene)
	: IEntity_scripting_manager(scene)
{
}

Scene* g_scene;
Scene* get_scene() {
    if (!g_scene) {
        LOG(FATAL) << "Python script attempting to access an uninitialized scene";
    }
    return g_scene;
}

void engine_log_func(const std::string& message, LEVELS level) {
	if (!g3::logLevel(level)) {
		return;
	}

	auto inspectModule = py::module::import("inspect");
	auto stackArray = inspectModule.attr("stack")();
	if (py::isinstance<py::list>(stackArray)) {
		auto stackList = py::list(stackArray);
		if (stackList.size() > 0) {
			auto frame = stackList[0];

			auto filename = std::string(py::str(frame.attr("filename")));
			auto lastSlash = std::max(filename.find_last_of('/'), filename.find_last_of('\\'));
			if (lastSlash != std::string::npos) {
				filename = filename.substr(lastSlash);
			}
			auto lineno = frame.attr("lineno").cast<int>();
			auto function = std::string(py::str(frame.attr("function")));

			LogCapture(filename.c_str(), lineno, function.c_str(), level).stream() << message;
		}
	}
	else {
		LOG(level) << "[python] " << message;
	}
}

bool engine_log_info_enabled_func() {
	return g3::logLevel(INFO);
}

void engine_log_info_func(const std::string& message) {
	engine_log_func(message, INFO);
}

bool engine_log_warning_enabled_func() {
	return g3::logLevel(WARNING);
}

void engine_log_warning_func(const std::string& message) {
	engine_log_func(message, WARNING);
}

PYBIND11_EMBEDDED_MODULE(engine, m) {
    m.doc() = "Orangine scripting API";

    m.def("scene", &get_scene, py::return_value_policy::reference, "A function which returns the Scene singleton");

	m.def("log_info_enabled", &engine_log_info_enabled_func);
	m.def("log_info", &engine_log_info_func);

	m.def("log_warning_enabled", &engine_log_warning_enabled_func);
	m.def("log_warning", &engine_log_warning_func);
}

void Entity_scripting_manager::initialize()
{
	{
		_scriptableEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Script_component::type() });
		_testEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Test_component::type() });
		_scriptableEntityFilterListener = std::make_unique<Entity_filter::Entity_filter_listener>();
		_scriptableEntityFilterListener->onAdd = [this](Entity* entity) {
			_addedEntities.push_back(entity->getId());
		};
		_scriptableEntityFilterListener->onRemove = [this](Entity* entity) {
			_removedEntities.push_back(entity->getId());
		};
		_scriptableEntityFilter->add_listener(_scriptableEntityFilterListener);

		_renderableEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Renderable_component::type() });
		_lightEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({
			Directional_light_component::type(),
			Point_light_component::type(),
			Ambient_light_component::type()
			}, Entity_filter_mode::Any);
		_scriptData.yaw = XM_PI;

		try {
			initializePythonInterpreter();
			return;
		}
		catch (const py::error_already_set& err) {
			logPythonError(err, "service initialization");
		}
	}

	// This must happen outside of a catch, so that the error_already_set object is destructed prior to throwing.
	finalizePythonInterpreter();
	throw::std::runtime_error("Failed to initialize Entity Scripting Manager.");
}

void Entity_scripting_manager::logPythonError(const py::error_already_set& err, const std::string& whereStr) const {
	std::stringstream ss;
	ss << "Python error in " << whereStr << ": " << std::string(py::str(err.value()));

	auto traceback = err.trace();
	while (!traceback.is_none() && traceback.ptr() != nullptr) {
		std::string lineno = std::string(py::str(traceback.attr("tb_lineno")));
		std::string filename =
			std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_filename")));
		std::string funcname =
			std::string(py::str(traceback.attr("tb_frame").attr("f_code").attr("co_name")));

		ss << std::endl;
		ss << " at " << filename << ":" << lineno << " -> " << funcname;

		traceback = traceback.attr("tb_next");
	}

	LOG(WARNING) << ss.str();
}

void Entity_scripting_manager::tick() 
{
    const auto& sceneGraphManager = _scene.manager<IScene_graph_manager>();
    for (const auto entityId : _addedEntities) {
        auto entity = sceneGraphManager.getEntityPtrById(entityId);
        if (entity == nullptr)
            continue;

        const auto& component = entity->getFirstComponentOfType<Script_component>();
        const auto& componentScriptPath = component->scriptName();

		if (componentScriptPath.empty()) {
			continue;
		}
		if (componentScriptPath.at(componentScriptPath.size() - 1) == '.') {
			LOG(WARNING) << "Invalid script name, cannot end with .";
			continue;
		}

		try {
			const auto lastDot = componentScriptPath.find_last_of('.');
			std::unique_ptr<Script_runtime_data, Script_component::Script_runtime_data_deleter> runtimeData;
			runtimeData.reset(new Script_runtime_data());

			if (lastDot != std::string::npos) {
				const auto scriptModuleName = componentScriptPath.substr(0, lastDot);
				const auto scriptClassName = componentScriptPath.substr(lastDot + 1);

				const auto scriptModule = py::module::import(scriptModuleName.c_str());
				const auto scriptClassDefn = scriptModule.attr(scriptClassName.c_str());

				runtimeData->instance = scriptClassDefn();
			}
			else {
				const auto scriptClassDefn = py::globals().attr(componentScriptPath.c_str());

				runtimeData->instance = scriptClassDefn();
			}

			// If required, initialize the instance
			if (py::hasattr(runtimeData->instance, "init")) {
				auto initFn = runtimeData->instance.attr("init");
				auto result = initFn();
			}

			runtimeData->hasTick = py::hasattr(runtimeData->instance, "tick");
			component->_scriptRuntimeData = move(runtimeData);

		} catch (const py::error_already_set& err) {
			std::stringstream whereStr; 
			whereStr << "Entity ID " << entityId;
			if (!entity->getName().empty()) {
				whereStr << " (" << entity->getName() << ")";
			}
			whereStr << " init()" << componentScriptPath;
			logPythonError(err, whereStr.str());
		}
        
        // TODO: Bind to this entity somehow?
        // TODO: Start script init lifecycle.
    }
    _addedEntities.clear();

	for (const auto entityId : _removedEntities) {
		auto entity = sceneGraphManager.getEntityPtrById(entityId);
		if (entity == nullptr)
			continue;

		const auto& component = entity->getFirstComponentOfType<Script_component>();
		if (component->_scriptRuntimeData != nullptr) {
			if (py::hasattr(component->_scriptRuntimeData->instance, "shutdown")) {
				try {
					auto _ = component->_scriptRuntimeData->instance.attr("shutdown")();
				}
				catch (const py::error_already_set& err) {
					std::stringstream whereStr;
					whereStr << "Entity ID " << entityId;
					if (!entity->getName().empty()) {
						whereStr << " (" << entity->getName() << ")";
					}
					whereStr << " shutdown()";
					logPythonError(err, whereStr.str());
				}
			}
			component->_scriptRuntimeData = nullptr;
		}
	}
	_removedEntities.clear();

	for (auto iter = _scriptableEntityFilter->begin(); iter != _scriptableEntityFilter->end(); ++iter) {
		auto& entity = **iter;
		const auto component = entity.getFirstComponentOfType<Script_component>();
		if (component->_scriptRuntimeData == nullptr || !component->_scriptRuntimeData->hasTick)
			continue;

		try {
			auto _ = component->_scriptRuntimeData->instance.attr("tick")();
		}
		catch (const py::error_already_set& err) {
			std::stringstream whereStr;
			whereStr << "Entity ID " << entity.getId();
			if (!entity.getName().empty()) {
				whereStr << " (" << entity.getName() << ")";
			}
			whereStr << " tick()";
			logPythonError(err, whereStr.str());
		}
	}

	try {
		flushStdIo();
	}
	catch (const py::error_already_set& err) {
		logPythonError(err, "Flushing stdout and stderr");
	}

	const auto elapsedTime = _scene.elapsedTime();
	for (auto iter = _testEntityFilter->begin(); iter != _testEntityFilter->end(); ++iter) {
		auto& entity = **iter;
		auto* component = entity.getFirstComponentOfType<Test_component>();
		if (component == nullptr)
			continue;
		
		const auto &speed = component->getSpeed();

		const auto animTimePitch = static_cast<float>(fmod(elapsedTime * speed.getX() * XM_2PI, XM_2PI));
		const auto animTimeYaw = static_cast<float>(fmod(elapsedTime * speed.getY()* XM_2PI, XM_2PI));
		const auto animTimeRoll = static_cast<float>(fmod(elapsedTime * speed.getZ()* XM_2PI, XM_2PI));
		entity.setRotation(SimpleMath::Quaternion::CreateFromYawPitchRoll(animTimeYaw, animTimePitch, animTimeRoll));
	}
	
	const auto mouseSpeed = 1.0f / 600.0f;
	const auto mouseState = _scene.manager<IInput_manager>().mouseState().lock();
	if (mouseState) {
		if (mouseState->left == IInput_manager::Mouse_state::Button_state::HELD) {
			_scriptData.yaw += -mouseState->deltaPosition.x * XM_2PI * mouseSpeed;
			_scriptData.pitch += mouseState->deltaPosition.y * XM_2PI * mouseSpeed;

			_scriptData.pitch = std::max(XM_PI * -0.45f, std::min(XM_PI * 0.45f, _scriptData.pitch));

		}
		//if (mouseState->middle == Input_manager::Mouse_state::Button_state::HELD) {
			_scriptData.distance = std::max(1.0f, std::min(40.0f, _scriptData.distance + static_cast<float>(mouseState->scrollWheelDelta) * -mouseSpeed));
		//}

		if (mouseState->right == IInput_manager::Mouse_state::Button_state::HELD) {
			renderDebugSpheres();
		}

		const auto cameraRot = SSE::Matrix4::rotationY(_scriptData.yaw + Math::PI) * SSE::Matrix4::rotationX(-_scriptData.pitch);
		auto cameraPosition = cameraRot * Math::Direction::Backward;
		cameraPosition *= _scriptData.distance;

		auto entity = _scene.mainCamera();
		if (entity != nullptr) {
			entity->setPosition({ cameraPosition.getX(), cameraPosition.getY(), cameraPosition.getZ() });
			auto cameraRotQuat = SSE::Quat(cameraRot.getUpper3x3());
			entity->setRotation({ cameraRotQuat.getX(), cameraRotQuat.getY(), cameraRotQuat.getZ(), cameraRotQuat.getW() });
		}
	}
}

void Entity_scripting_manager::flushStdIo() {
	const auto _ = _pythonContext.engine_internal->reset_output_streams();
}

void Entity_scripting_manager::shutdown()
{
    for (const auto& entry : _loadedModules) {
        Py_DECREF(entry.second);
    }
    _loadedModules.clear();

	finalizePythonInterpreter();
}

void Entity_scripting_manager::initializePythonInterpreter() {

	assert(g_scene == nullptr);
	g_scene = &_scene;

	py::initialize_interpreter();
	_pythonInitialized = true;
	_pythonContext.sys = py::module::import("sys");
	auto sysPathList = _pythonContext.sys.attr("path").cast<py::list>();

	const auto& dataLibPath = _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(L"OeScripting/lib");
	sysPathList.append(utf8_encode(dataLibPath));

	const auto& dataScriptsPath = _scene.manager<IAsset_manager>().makeAbsoluteAssetPath(L"OeScripting/scripts");
	sysPathList.append(utf8_encode(dataScriptsPath));

	_pythonContext.engine_internal = std::make_unique<EngineInternalPythonModule>(py::module::import("engine_internal"));

	// This must be called at least once before python attempts to write to stdout or stderr
	flushStdIo();
}

void Entity_scripting_manager::finalizePythonInterpreter() {

	g_scene = nullptr;

	if (_pythonInitialized) {
		_pythonContext = {};
		py::finalize_interpreter();
		_pythonInitialized = false;
	}
}

void Entity_scripting_manager::renderImGui()
{
    if (_showImGui) {
        ImGui::ShowDemoWindow();        
    }
}

void Entity_scripting_manager::execute(const std::string& command)
{
    // TODO: Execute some python, or something :)
    LOG(INFO) << "exec: " << command;

    if (command == "imgui") {
        _showImGui = !_showImGui;
    }
}

bool Entity_scripting_manager::commandSuggestions(const std::string& command, std::vector<std::string>& suggestions)
{
    // TODO: provide some suggestions!
    return false;
}

const std::string& Entity_scripting_manager::name() const
{
    return _name;
}

void Entity_scripting_manager::renderDebugSpheres() const
{
	auto& renderManager = _scene.manager<IEntity_render_manager>();
	auto& devToolsManager = _scene.manager<IDev_tools_manager>();
	devToolsManager.clearDebugShapes();

	for (const auto& entity : *_renderableEntityFilter) {
		const auto& boundSphere = entity->boundSphere();
		const auto transform = SimpleMath::Matrix::CreateTranslation(boundSphere.Center) * entity->worldTransform();
		devToolsManager.addDebugSphere(toVectorMathMat4(transform), boundSphere.Radius, Colors::Gray);
	}

	const auto mainCameraEntity = _scene.mainCamera();
	if (mainCameraEntity != nullptr) {
		const auto cameraComponent = mainCameraEntity->getFirstComponentOfType<Camera_component>();
		if (cameraComponent) {
			auto frustum = renderManager.createFrustum(*cameraComponent);
			frustum.Far *= 0.5;
			devToolsManager.addDebugFrustum(frustum, Colors::Red);
		}
	}

	for (const auto& entity : *_lightEntityFilter) {
		const auto directionalLight = entity->getFirstComponentOfType<Directional_light_component>();
		if (directionalLight && directionalLight->shadowsEnabled()) {
			const auto& shadowData = directionalLight->shadowData();
			if (shadowData) {
				devToolsManager.addDebugBoundingBox(shadowData->casterVolume(), directionalLight->color());
			}
		}
	}
}
