#pragma once

#include "Manager_base.h"

namespace oe {
class IUser_interface_manager : public Manager_base,
                                public Manager_windowDependent,
                                public Manager_windowsMessageProcessor {
 public:
  virtual void preInit_setUIScale(float scale) = 0;

  virtual void render() = 0;
  virtual bool keyboardCaptured() = 0;
  virtual bool mouseCaptured() = 0;
};
}// namespace oe