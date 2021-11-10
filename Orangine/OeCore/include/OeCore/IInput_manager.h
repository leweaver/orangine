#pragma once

#include "Manager_base.h"

#include "Renderer_types.h"

namespace oe {

class IInput_manager {
 public:
  struct Mouse_state {
    enum class Button_state {
      Up = 0,       // Button is up
      Held = 1,     // Button is held down
      Released = 2, // Button was just released
      Pressed = 3,  // Button was just pressed
    };

    Vector2i absolutePosition = {0, 0};
    Vector2i deltaPosition = {0, 0};

    Button_state left = Button_state::Up;
    Button_state middle = Button_state::Up;
    Button_state right = Button_state::Up;
    int scrollWheelDelta = 0;
  };

  // Mouse functions
  virtual std::weak_ptr<Mouse_state> getMouseState() const = 0;
};
} // namespace oe