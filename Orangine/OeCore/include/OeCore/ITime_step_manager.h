#pragma once

#include "Manager_base.h"

namespace oe {

/**
 * Provides a read-only view into the applications time-step
 */
class ITime_step_manager {
 public:
  ITime_step_manager() = default;
  virtual ~ITime_step_manager() = default;

  virtual void progressTime(double deltaTime) = 0;

  /** Total time since game start, in seconds. */
  virtual double getElapsedTime() const = 0;

  /** Time since last frame, in seconds. */
  virtual double getDeltaTime() const = 0;
};
} // namespace oe