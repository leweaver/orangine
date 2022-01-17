#include <OeApp/OeApp_bindings.h>
#include <OeApp/SampleScene.h>

#include <g3log/g3log.hpp>

namespace py = pybind11;

using oe::OeApp_bindings;
using oe::Sample_scene;

std::string OeApp_bindings::_moduleName;

void OeApp_bindings::create(pybind11::module& m)
{
  _moduleName = m.attr("__name__").cast<std::string>();
  m.doc() = "Orangine App Bindings";

  py::class_<PyClass_sampleScene>(m, "SampleScene")
          .def_property_readonly("scene_graph", &PyClass_sampleScene::getSceneGraph)
          .def_property_readonly("input", &PyClass_sampleScene::getInput);
}

const std::string& OeApp_bindings::getModuleName() {
  return _moduleName;
}

void OeApp_bindings::initStatics(){
  _moduleName = "oe_app";
}
void OeApp_bindings::destroyStatics(){

}