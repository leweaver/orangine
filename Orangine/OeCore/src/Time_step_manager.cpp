#include <OeCore/Manager_base.h>

#include "Time_step_manager.h"

using namespace oe;

std::string Time_step_manager::_name = "Time_step_manager";

template <> void oe::create_manager(Manager_instance<ITime_step_manager>& out)
{
  out = Manager_instance<ITime_step_manager>(std::make_unique<Time_step_manager>());
}

void Time_step_manager::progressTime(double deltaTime)
{
  _deltaTime = deltaTime;
  _elapsedTime += _deltaTime;
}