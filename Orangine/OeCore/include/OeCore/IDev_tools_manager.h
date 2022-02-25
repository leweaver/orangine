#pragma once
#include "Collision.h"
#include "Color.h"
#include "Manager_base.h"
#include "Render_pass.h"
#include "Dispatcher.h"

namespace oe {
class VectorLog;
class Entity;
class Unlit_material;
class IPrimitive_mesh_data_factory;
struct Renderable;
struct BoundingFrustumRH;

class IDev_tools_manager {
 public:
  virtual void addDebugCone(const SSE::Matrix4& worldTransform, float diameter, float height, const Color& color) = 0;
  virtual void
  addDebugSphere(const SSE::Matrix4& worldTransform, float radius, const Color& color, size_t tessellation = 6) = 0;
  virtual void addDebugBoundingBox(const oe::BoundingOrientedBox& boundingOrientedBox, const Color& color) = 0;
  virtual void addDebugFrustum(const BoundingFrustumRH& boundingFrustum, const Color& color) = 0;
  virtual void addDebugAxisWidget(const SSE::Matrix4& worldTransform) = 0;
  virtual void setGuiDebugText(const std::string& text) = 0;
  virtual void clearDebugShapes() = 0;

  virtual void renderDebugShapes(const Camera_data& cameraData) = 0;
  virtual void renderImGui() = 0;

  virtual void setVectorLog(VectorLog* logSink) = 0;

  virtual void setCommandSuggestions(const std::vector<std::string>& suggestions) = 0;
  virtual const std::vector<std::string>& getCommandSuggestions() const = 0;

  /**
   * Event that's fired whenever a character is typed into the console. Can be used to update command suggestions.
   * The dispatcher argument is the current content of the console input box
   */
  virtual Dispatcher<std::string>& getCommandAutocompleteRequestedDispatcher() = 0;

  /**
   * Event that's fired whenever a console command is executed
   * The dispatcher argument is the current content of the console input box
   */
  virtual Dispatcher<std::string>& getCommandExecutedDispatcher() = 0;
};
}// namespace oe