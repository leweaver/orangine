#pragma once

#include "OeCore/VectorLog.h"

#include <g3log/g3log.hpp>
#include <memory>

namespace oe {
class VectorLogSink {
 public:
  VectorLogSink(std::shared_ptr<VectorLog> vectorLog) : _vectorLog(vectorLog) {}

  void append(g3::LogMessageMover message) const
  {
    _vectorLog->append(message.get()._timestamp, message.get().message(),
                       message.get()._level.value);
  }

 private:
  std::shared_ptr<VectorLog> _vectorLog;
};
} // namespace oe
