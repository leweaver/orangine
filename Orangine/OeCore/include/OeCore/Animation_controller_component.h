#pragma once
#include "OeCore/Component.h"
#include "OeCore/Mesh_data.h"
#include "OeCore/Renderer_enum.h"

namespace oe {
struct Mesh_buffer_accessor;
class Animation_controller_component : public Component {
  DECLARE_COMPONENT_TYPE;

 public:
  struct Animation_channel {
    std::shared_ptr<Entity> targetNode;
    Animation_type animationType;
    Animation_interpolation interpolationType;
    uint8_t valuesPerKeyFrame;

    std::shared_ptr<std::vector<float>> keyframeTimes;
    std::unique_ptr<Mesh_buffer_accessor> keyframeValues;
  };

  struct Animation_state {
    bool playing = true;
    double currentTime = 0.0f;
    double speed = 1.0f;
  };

  struct Animation {
    std::vector<std::unique_ptr<Animation_channel>> channels;
  };

  explicit Animation_controller_component(Entity& entity) : Component(entity) {}

  void addAnimation(std::string animationName, std::unique_ptr<Animation> animation);

  // A map of animation name -> Animation
  const std::map<std::string, std::unique_ptr<Animation>>& animations() const {
    return _nameToAnimation;
  }
  bool containsAnimationByName(const std::string& animationName) const {
    return _nameToAnimation.find(animationName) != _nameToAnimation.end();
  }

  const std::unique_ptr<Animation>& animationByName(const std::string& animationName);
  
  // Serializable Properties.
  BEGIN_COMPONENT_PROPERTIES();
  END_COMPONENT_PROPERTIES();

  // A map of animation names to vector of channel states.
  std::map<std::string, std::vector<Animation_state>> activeAnimations;

 private:
  std::map<std::string, std::unique_ptr<Animation>> _nameToAnimation;
};
} // namespace oe
