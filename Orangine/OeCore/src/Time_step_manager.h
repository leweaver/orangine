#pragma once

#include <OeCore/ITime_step_manager.h>
#include <OeCore/StepTimer.h>

#include <string>

namespace oe {
class Time_step_manager : public ITime_step_manager
    , public Manager_base {
 public:
  Time_step_manager()
      : ITime_step_manager()
      , Manager_base()
  {}

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override { return _name; }

  // ITime_step_manager implementation
  void progressTime(double deltaTime) override;
  double getElapsedTime() const override { return _elapsedTime; }
  double getDeltaTime() const override { return _deltaTime; }

 private:
  // Values as of the last call to Tick.
  double _deltaTime = 0;
  double _elapsedTime = 0;

  StepTimer _timer;

  static std::string _name;
};
} // namespace oe