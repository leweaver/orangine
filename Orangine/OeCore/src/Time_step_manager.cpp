#include "pch.h"

#include <OeCore/Manager_base.h>

#include "Time_step_manager.h"

using namespace oe;

std::string Time_step_manager::_name = "Animation_manager";

template <> ITime_step_manager* oe::create_manager(
    Scene& scene, const StepTimer& stepTimer)
{
  return new Time_step_manager(scene, stepTimer);
}

void Time_step_manager::tick() {
  _deltaTime = _timer.GetElapsedSeconds();
  _elapsedTime += _deltaTime;
}