#pragma once

#include <g3log/g3log.hpp>
#include <memory>

#include <debugapi.h>
#include <OeCore/EngineUtils.h>

namespace oe {
class VisualStudioLogSink {
 public:
  VisualStudioLogSink() {}

  void append(g3::LogMessageMover message) const {
    auto str = utf8_decode(message.get().toString());
    OutputDebugString(str.c_str());
  }

 private:
  std::shared_ptr<VectorLog> _vectorLog;
};
} // namespace oe
