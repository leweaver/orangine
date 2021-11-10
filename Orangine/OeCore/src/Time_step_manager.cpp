#include "pch.h"

#include <OeCore/Manager_base.h>

#include "Time_step_manager.h"

using namespace oe;

std::string Time_step_manager::_name = "Animation_manager";

template <> void oe::create_manager(Manager_instance<ITime_step_manager>& out, const StepTimer& stepTimer)
{
  out = Manager_instance<ITime_step_manager>(std::make_unique<Time_step_manager>(stepTimer));
}

void Time_step_manager::tick() {
  _deltaTime = _timer.GetElapsedSeconds();
  _elapsedTime += _deltaTime;
}