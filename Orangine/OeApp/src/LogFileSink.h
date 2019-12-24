/** ==========================================================================
 * 2013 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 *
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================*/
#pragma once

#include <memory>
#include <string>

#include "g3log/logmessage.hpp"
namespace oe {

// Mostly copied from g3::FileSink - modified to allow overwrite of same logfile each run.
class LogFileSink {
 public:
  LogFileSink(const std::string& log_prefix, const std::string& log_directory,
              const std::string& logger_id = "g3log", bool appendDate = false);
  virtual ~LogFileSink();

  void fileInit();
  void fileWrite(g3::LogMessageMover message);
  std::string changeLogFile(const std::string& directory, const std::string& logger_id);
  std::string fileName();

 protected:
  virtual std::string createLogFileName(const std::string& verified_prefix);

 private:
  std::string _log_file_with_path;
  std::string _log_prefix_backup; // needed in case of future log file changes of directory
  std::string _logger_id;
  bool _appendDate;

  std::unique_ptr<std::ofstream> _outptr;

  void addLogFileHeader();
  std::ofstream& filestream() { return *(_outptr.get()); }

  LogFileSink& operator=(const LogFileSink&) = delete;
  LogFileSink(const LogFileSink& other) = delete;
};
} // namespace oe
