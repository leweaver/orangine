#pragma once
#include "OeCore/Collision.h"
#include "OeCore/Fps_counter.h"
#include "OeCore/IDev_tools_manager.h"
#include "OeCore/IScene_graph_manager.h"
#include <OeCore/IEntity_render_manager.h>
#include <OeCore/IMaterial_manager.h>
#include "OeCore/Mesh_data.h"
#include <OeCore/Dispatcher.h>

namespace oe {
class Perf_timer;
}

namespace oe::internal {
class Dev_tools_manager : public IDev_tools_manager, public Manager_base, public Manager_tickable, public Manager_deviceDependent {
 public:
  Dev_tools_manager(
          IScene_graph_manager& sceneGraphManager, IEntity_render_manager& entityRenderManager,
          IMaterial_manager& materialManager)
      : IDev_tools_manager(), Manager_base(), Manager_tickable(), Manager_deviceDependent(), _unlitMaterial(nullptr)
      , _sceneGraphManager(sceneGraphManager)
      , _entityRenderManager(entityRenderManager)
      , _materialManager(materialManager)
  {}

  // Manager_base implementation
  void initialize() override;
  void shutdown() override;
  const std::string& name() const override;

  // Manager_tickable implementation
  void tick() override;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // IEntity_render_manager implementation

  // Creates a cone, whose base sits on the XZ plane, and height goes up the Y plane.
  // The origin of the object is the midpoint, not the base.
  void addDebugCone(const SSE::Matrix4& worldTransform, float diameter, float height,
                    const Color& color) override;
  void addDebugSphere(const SSE::Matrix4& worldTransform, float radius, const Color& color,
                      size_t tessellation = 6) override;
  void addDebugBoundingBox(const oe::BoundingOrientedBox& boundingOrientedBox,
                           const Color& color) override;
  void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const Color& color) override;
  void addDebugAxisWidget(const SSE::Matrix4& worldTransform) override;
  void setGuiDebugText(const std::string& text) override;
  void clearDebugShapes() override;
  void renderDebugShapes(const Camera_data& cameraData) override;
  void renderImGui() override;
  void setVectorLog(VectorLog* vectorLog) override { _vectorLog = vectorLog; }

  const std::vector<std::string>& getCommandSuggestions() const override
  {
    return _commandSuggestions;
  }
  void setCommandSuggestions(const std::vector<std::string>& commandSuggestions) override
  {
    _commandSuggestions = commandSuggestions;
  }
  Dispatcher<std::string>& getCommandAutocompleteRequestedDispatcher() override
  {
    return _commandAutocompleteRequestedDispatcher;
  }
  Dispatcher<std::string>& getCommandExecutedDispatcher()
  {
    return _commandExecutedDispatcher;
  }

 private:
  using LightProvider = std::function<void(const oe::BoundingSphere& target,
                                           std::vector<Entity*>& lights, uint8_t maxLights)>;

  std::shared_ptr<Renderable>
  getOrCreateRenderable(size_t hash, std::function<std::shared_ptr<Mesh_data>()> factory);
  void renderAxisWidgets();
  void renderSkeletons();

  static std::string _name;

  std::shared_ptr<Unlit_material> _unlitMaterial;
  std::vector<std::tuple<SSE::Matrix4, Color, std::shared_ptr<Renderable>>> _debugShapes;
  LightProvider _noLightProvider;

  std::unique_ptr<Fps_counter> _fpsCounter;

  VectorLog* _vectorLog = nullptr;
  std::string _consoleInput;
  std::string _guiDebugText = "";
  bool _scrollLogToBottom = false;
  bool _renderSkeletons = false;
  std::vector<std::string> _commandSuggestions;

 private:
  std::shared_ptr<Entity_filter> _animationControllers;
  std::shared_ptr<Entity_filter> _skinnedMeshEntities;

  // Map of std::hash output to renderable.
  std::map<std::size_t, std::shared_ptr<Renderable>> _shapeMeshCache;

  IScene_graph_manager& _sceneGraphManager;
  IEntity_render_manager& _entityRenderManager;
  IMaterial_manager& _materialManager;

  Invokable_dispatcher<std::string> _commandAutocompleteRequestedDispatcher;

 private:
  Invokable_dispatcher<std::string> _commandExecutedDispatcher;
};
} // namespace oe::internal
