#include "tests_main.h"

#include <gtest/gtest.h>
#include <g3log/logworker.hpp>
#include <g3log/logmessage.hpp>

using oe::testing_helpers::CaptureLogger;
using oe::testing_helpers::CapturedLogs;

namespace oe::testing_helpers {

std::string g_mockFatal_message = {};
g3::SignalType g_mockFatal_signal = -1;
bool g_mockFatalWasCalled = false;

std::string mockFatalMessage() {
  return g_mockFatal_message;
}

g3::SignalType mockFatalSignal() {
  return g_mockFatal_signal;
}

bool mockFatalWasCalled() {
  return g_mockFatalWasCalled;
}

void mockFatalCall(g3::FatalMessagePtr fatal_message) {
  g_mockFatal_message = fatal_message.get()->toString();
  g_mockFatal_signal = fatal_message.get()->_signal_id;
  g_mockFatalWasCalled = true;
  g3::LogMessagePtr message{fatal_message.release()};
  g3::internal::pushMessageToLogger(message); //fatal_message.copyToLogMessage());
}

void clearMockFatal() {
  g_mockFatal_message.clear();
  g_mockFatal_signal = -1;
  g_mockFatalWasCalled = false;
}

struct ScopedLogger {
  explicit ScopedLogger(uint32_t maxSize)
      : _currentWorker(g3::LogWorker::createLogWorker())
      , _vectorLog(maxSize)
  {}
  ~ScopedLogger() = default;

  g3::LogWorker* get()
  {
    return _currentWorker.get();
  }

  void append(g3::LogMessageMover message)
  {
    _vectorLog.append(message.get()._timestamp, message.get().message(), message.get()._level.value);

    std::cout << message.get().toString() << std::flush;
  }

  CapturedLogs flushAndShutdown()
  {
    if (_currentWorker == nullptr) {
      return CapturedLogs();
    }

    _currentWorker.reset();

    uint32_t logCount = _vectorLog.messageCount();
    auto capturedLogs = std::vector<oe::VectorLog::Log_message>(logCount);
    for (uint32_t i = 0 ; i < logCount; ++i) {
      capturedLogs.push_back(_vectorLog.getMessageAt(i));
    }

    return CapturedLogs(std::move(capturedLogs), mockFatalWasCalled());
  }

 private:
  oe::VectorLog _vectorLog;
  std::unique_ptr<g3::LogWorker> _currentWorker;
};

class ScopedLoggerSink {
 public:
  explicit ScopedLoggerSink(ScopedLogger* scopedLogger)
      : _scopedLogger(scopedLogger)
  {}

  void append(g3::LogMessageMover message)
  {
    _scopedLogger->append(std::move(message));
  }

  ~ScopedLoggerSink() = default;

 private:
  ScopedLogger* _scopedLogger;
};
}

CaptureLogger::CaptureLogger(uint32_t maxLogs)
    : _maxLogs(maxLogs)
    , _scope(new ScopedLogger(maxLogs))
    , _handle(_scope->get()->addSink(std::make_unique<ScopedLoggerSink>(_scope.get()), &ScopedLoggerSink::append)) {
  g3::initializeLogging(_scope->get());
  clearMockFatal();
  setFatalExitHandler(&mockFatalCall);

#ifdef G3_DYNAMIC_LOGGING
  g3::only_change_at_initialization::addLogLevel(INFO, true);
  g3::only_change_at_initialization::addLogLevel(G3LOG_DEBUG, true);
  g3::only_change_at_initialization::addLogLevel(WARNING, true);
  g3::only_change_at_initialization::addLogLevel(FATAL, true);
#endif
}

CaptureLogger::~CaptureLogger()
{}

CapturedLogs CaptureLogger::flushAndReset()
{
  g3::internal::shutDownLogging(); // is done at reset. Added for test clarity
  auto logs = _scope->flushAndShutdown();

  _scope.reset(new ScopedLogger(_maxLogs));
  _handle = _scope->get()->addSink(std::make_unique<ScopedLoggerSink>(_scope.get()), &ScopedLoggerSink::append);
  g3::initializeLogging(_scope->get());
  clearMockFatal();
  setFatalExitHandler(&mockFatalCall);

  return logs;
}

CapturedLogs::CapturedLogs(std::vector<oe::VectorLog::Log_message>&& logs, bool wasFatalCalled) :
    _capturedLogs(logs),
    _wasFatalCalled(wasFatalCalled)
{}

bool CapturedLogs::containsLog(const std::string& msg, const LEVELS& level)
{
  for (const auto& log : _capturedLogs) {
    if (log.level != level.value) {
      continue;
    }

    if (log.message == msg) {
      return true;
    }
  }
  return false;
}

bool CapturedLogs::containsLog(const LEVELS& level)
{
  for (const auto& log : _capturedLogs) {
    if (log.level == level.value) {
      return true;
    }
  }
  return false;
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();

  return 0;
}