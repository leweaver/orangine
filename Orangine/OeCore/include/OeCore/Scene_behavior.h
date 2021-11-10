#pragma once

namespace oe {
class Scene;

/**
 * Ticked once per frame. Useful for things that aren't tied to a specific entity,
 * such as scorekeeping and rules engines. This is how you add custom manager classes to your app.
 */
class Scene_behavior {
 public:
  explicit Scene_behavior(std::string&& name) : _name(std::move(name)) {}
  virtual ~Scene_behavior() = default;
  Scene_behavior(const Scene_behavior&) = default;
  Scene_behavior& operator=(const Scene_behavior&) = default;
  Scene_behavior(Scene_behavior&&) = default;
  Scene_behavior& operator=(Scene_behavior&&) = default;

  const std::string& name() const { return _name; }

  // Called once before the first time this behavior is ticked
  virtual void initialize() {}

  // Called once as the scene is un-initializing
  virtual void shutdown() {}

  virtual void handleScene() = 0;

  // Called after handleScene
  virtual void renderImGui() {}

 private:
  std::string _name;
};
} // namespace oe