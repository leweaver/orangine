// Note - since python must be included before standard library, precompiled headers need to be disabled on this file.
#include <Python.h>
#include "pch.h"

#include "Scene_graph_manager.h"
#include "Entity_scripting_manager.h"
#include "Input_manager.h"
#include "OeCore/Test_component.h"
#include "OeCore/Renderable_component.h"
#include "OeCore/Camera_component.h"
#include "OeCore/Light_component.h"
#include "OeCore/Shadow_map_texture.h"
#include "OeCore/Scene.h"
#include "OeCore/Entity_filter.h"
#include "OeCore/Entity.h"

#include <cmath>
#include <set>

#include "imgui/imgui.h"

using namespace DirectX;
using namespace SimpleMath;
using namespace oe;
using namespace internal;

std::string Entity_scripting_manager::_name = "Entity_scripting_manager";

template<>
IEntity_scripting_manager* oe::create_manager(Scene & scene)
{
    return new Entity_scripting_manager(scene);
}

Entity_scripting_manager::Entity_scripting_manager(Scene &scene)
	: IEntity_scripting_manager(scene)
{
}

static PyObject *SpamError;

/* Return the number of arguments of the application command line */
static PyObject *
spam_system(PyObject *self, PyObject *args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    sts = system(command);
    if (sts < 0) {
        PyErr_SetString(SpamError, "System command failed");
        return NULL;
    }
    return PyLong_FromLong(sts);
}

static PyMethodDef SpamMethods[] = {
    {"system",  spam_system, METH_VARARGS,
     "Execute a shell command."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef spammodule = {
    PyModuleDef_HEAD_INIT,
    "spam",   /* name of module */
    NULL,     /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    SpamMethods
};

PyMODINIT_FUNC
PyInit_spam(void)
{
    return PyModule_Create(&spammodule);
}

void Entity_scripting_manager::initialize()
{
	_scriptableEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Test_component::type() });
	_renderableEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({ Renderable_component::type() });
	_lightEntityFilter = _scene.manager<IScene_graph_manager>().getEntityFilter({
		Directional_light_component::type(),
		Point_light_component::type(),
		Ambient_light_component::type()
		}, Entity_filter_mode::Any);
	_scriptData.yaw = XM_PI;

    TCHAR szFileName[MAX_PATH];
    GetModuleFileName(nullptr, szFileName, MAX_PATH);

    _pythonProgramName = szFileName;

    GetCurrentDirectoryW(MAX_PATH, szFileName);


    _pythonHome = 
        L"C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37;"
        L"C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37\\DLLs;"
        L"C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37\\Lib;"
        L"C:\\repos\\Orangine\\Engine\\scripts;"
        //L"C:\\Users\\hotma\\AppData\\Local\\Programs\\Python\\Python37\\Lib\\site-packages;" 
        
        //std::wstring(szFileName) + L"\\data\\scripts;" +
        //std::wstring(szFileName) + L"\\..\\Engine\\scripts"
        ;
    // TODO: Oh god the hacks. THE HACKS.

    Py_SetPythonHome(_pythonHome.c_str());
    Py_SetProgramName(_pythonProgramName.c_str());

    const auto res = PyImport_AppendInittab("spam", PyInit_spam);
    
    Py_Initialize();

//    PyImport_ImportModule("console");

    loadPythonModule("json");
    loadPythonModule("codecs");
    loadPythonModule("console");
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
	
	const auto mouseSpeed = 1.0f / 600.0f;
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

		if (mouseState->right == Input_manager::Mouse_state::Button_state::HELD) {
			renderDebugSpheres();
		}

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
    for (const auto& entry : _loadedModules) {
        Py_DECREF(entry.second);
    }
    _loadedModules.clear();
    if (Py_FinalizeEx() < 0) {
        LOG(WARNING) << "Failed to gracefully shut down python interpreter";
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

	for (const auto entity : *_renderableEntityFilter) {
		const auto& boundSphere = entity->boundSphere();
		const auto transform = Matrix::CreateTranslation(boundSphere.Center) * entity->worldTransform();
		devToolsManager.addDebugSphere(transform, boundSphere.Radius, Color(Colors::Gray));
	}

	const auto mainCameraEntity = _scene.mainCamera();
	if (mainCameraEntity != nullptr) {
		const auto cameraComponent = mainCameraEntity->getFirstComponentOfType<Camera_component>();
		if (cameraComponent) {
			auto frustum = renderManager.createFrustum(*cameraComponent);
			frustum.Far *= 0.5;
			devToolsManager.addDebugFrustum(frustum, Color(Colors::Red));
		}
	}

	for (const auto entity : *_lightEntityFilter) {
		const auto directionalLight = entity->getFirstComponentOfType<Directional_light_component>();
		if (directionalLight && directionalLight->shadowsEnabled()) {
			const auto& shadowData = directionalLight->shadowData();
			if (shadowData) {
				devToolsManager.addDebugBoundingBox(shadowData->casterVolume(), directionalLight->color());
			}
		}
	}
}


void Entity_scripting_manager::loadPythonModule(std::string moduleName)
{
    if (_loadedModules.find(moduleName) != _loadedModules.end()) {
        throw std::runtime_error("Module is already loaded: " + moduleName);
    }

    /*
    const auto res = PyRun_SimpleString(
        "import sys\n"
        "import spam\n"
        "import json\n"
        "\n"
        "spam.system(json.dumps(sys.path))\n"
    );
    return;
    */

    
    const auto moduleName_w = PyUnicode_DecodeFSDefault(moduleName.c_str());
    const auto loadedModule = PyImport_Import(moduleName_w);
    Py_DECREF(moduleName_w);

    if (loadedModule) {
        _loadedModules[moduleName] = loadedModule;
    }
}
