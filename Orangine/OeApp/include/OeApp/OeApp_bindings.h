#pragma once

#include <pybind11/pybind11.h>

#include <OeApp/SampleScene.h>
#include <OeScripting/OeScripting_bindings.h>

namespace oe {
class OeApp_bindings {
 public:
  explicit OeApp_bindings(pybind11::module module) : _module(module) {}

  static void create(pybind11::module& m);
  static const std::string& getModuleName();

  static void initStatics();
  static void destroyStatics();

 private:
  pybind11::module _module;
  static std::string _moduleName;
};

class PyClass_sampleScene {
 public:
  explicit PyClass_sampleScene(Sample_scene& sampleScene)
      : _sceneGraph(PyClass_sceneGraph(sampleScene.getSceneGraphManager()))
      , _input(PyClass_input(sampleScene.getInputManager()))
  {}

  PyClass_sceneGraph& getSceneGraph() { return _sceneGraph; }
  PyClass_input& getInput() { return _input; }

 private:
  PyClass_sceneGraph _sceneGraph;
  PyClass_input _input;
};
}