#pragma once

#include <OeCore/ITime_step_manager.h>
#include <OeCore/StepTimer.h>

#include <string>

namespace oe {
class Time_step_manager : public ITime_step_manager {
 public:
  Time_step_manager(Scene& scene, const StepTimer& stepTimer)
      : ITime_step_manager(scene), _timer(stepTimer) {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // Manager_tickable implementation
  void tick() override;

  // ITime_step_manager implementation
  double elapsedTime() const override { return _elapsedTime; }
  double deltaTime() const override { return _deltaTime; }

 private:
  // Values as of the last call to Tick.
  double _deltaTime = 0;
  double _elapsedTime = 0;

  const StepTimer& _timer;

  static std::string _name;
};
} // namespace oe