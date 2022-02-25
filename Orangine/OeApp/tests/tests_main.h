#pragma once

#include <OeCore/VectorLog.h>

#include <vector>

#include <g3log/g3log.hpp>
#include <g3log/sinkhandle.hpp>
#include <g3log/filesink.hpp>

namespace oe::testing_helpers {
class ScopedLoggerSink;
class ScopedLogger;

std::string mockFatalMessage();
g3::SignalType mockFatalSignal();
bool mockFatalWasCalled();
void mockFatalCall(g3::FatalMessagePtr fatal_message);
void clearMockFatal();

struct CapturedLogs {
  CapturedLogs() : _capturedLogs(), _wasFatalCalled(false) {};
  CapturedLogs(std::vector<oe::VectorLog::Log_message>&& logs, bool wasFatalCalled);

  // Calling either of these methods will invalidate the internal logger by calling flushAndShutdown()
  // no more LOG calls will be captured.
  bool containsLog(const std::string& msg, const LEVELS& level);
  bool containsLog(const LEVELS& level);

  bool wasFatalCalled() const { return _wasFatalCalled; }

 private:
  std::vector<oe::VectorLog::Log_message> _capturedLogs;
  bool _wasFatalCalled;
};

/** RAII temporarily replace of logger
 *  and restoration of original logger at scope end*/
struct CaptureLogger {
  explicit CaptureLogger(uint32_t maxLogs = 10U);
  ~CaptureLogger();

  CapturedLogs flushAndReset();
 private:
  uint32_t _maxLogs;
  std::unique_ptr<ScopedLogger> _scope;
  std::unique_ptr<g3::SinkHandle<ScopedLoggerSink>> _handle;
};
}