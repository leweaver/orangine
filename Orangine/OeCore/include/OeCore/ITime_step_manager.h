#pragma once

#include "Manager_base.h"

namespace oe {

/**
 * Provides a read-only view into the applications time-step
 */
class ITime_step_manager
    : public Manager_base
    , public Manager_tickable {
 public:
  explicit ITime_step_manager(Scene& scene) : Manager_base(scene) {}
  virtual ~ITime_step_manager() = default;

  /** Total time since game start, in seconds. */
  virtual double elapsedTime() const = 0;

  /** Time since last frame, in seconds. */
  virtual double deltaTime() const = 0;
};
} // namespace oe