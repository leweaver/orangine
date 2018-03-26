#include "pch.h"
#include "LogFileSink.h"

/** ==========================================================================
* 2013 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
*
* For more information see g3log/LICENSE or refer refer to http://unlicense.org
* 
* Modified to use different method for filename generation
* ============================================================================*/

#include "filesinkhelper.ipp"
#include <cassert>
#include <chrono>

namespace OE {
	using namespace g3;
	using namespace internal;

	LogFileSink::LogFileSink(const std::string &log_prefix, const std::string &log_directory, const std::string& logger_id, bool appendDate)
		: _log_file_with_path(log_directory)
		, _log_prefix_backup(log_prefix)
		, _logger_id(logger_id)
		, _appendDate(appendDate)
		, _outptr(new std::ofstream)
	{
		_log_prefix_backup = prefixSanityFix(log_prefix);
		if (!isValidFilename(_log_prefix_backup)) {
			std::cerr << "g3log: forced abort due to illegal log prefix [" << log_prefix << "]" << std::endl;
			abort();
		}
	}

	void LogFileSink::fileInit()
	{
		std::string file_name = createLogFileName(_log_prefix_backup);
		_log_file_with_path = pathSanityFix(_log_file_with_path, file_name);
		_outptr = createLogFile(_log_file_with_path);

		if (!_outptr) {
			std::cerr << "Cannot write log file to location, attempting current directory" << std::endl;
			_log_file_with_path = "./" + file_name;
			_outptr = createLogFile(_log_file_with_path);
		}
		assert(_outptr && "cannot open log file at startup");
		addLogFileHeader();
	}

	LogFileSink::~LogFileSink() {
		std::string exit_msg{ "g3log LogFileSink shutdown at: " };
		auto now = std::chrono::system_clock::now();
		exit_msg.append(localtime_formatted(now, internal::time_formatted)).append("\n");
		filestream() << exit_msg << std::flush;

		exit_msg.append("Log file at: [").append(_log_file_with_path).append("]\n");
		std::cerr << exit_msg << std::flush;
	}

	std::string LogFileSink::createLogFileName(const std::string &verified_prefix) {
		std::stringstream oss_name;
		oss_name << verified_prefix << ".";
		if (_logger_id != "") {
			oss_name << _logger_id << ".";
		}

		if (_appendDate) {
			auto now = std::chrono::system_clock::now();
			oss_name << g3::localtime_formatted(now, file_name_time_formatted);
		} 
		else {
			oss_name << "latest";
		}
		oss_name << ".log";
		return oss_name.str();
	}

	// The actual log receiving function
	void LogFileSink::fileWrite(LogMessageMover message) {
		std::ofstream &out(filestream());
		out << message.get().toString() << std::flush;
	}

	std::string LogFileSink::changeLogFile(const std::string &directory, const std::string &logger_id) {

		auto now = std::chrono::system_clock::now();
		auto now_formatted = g3::localtime_formatted(now, { internal::date_formatted + " " + internal::time_formatted });

		std::string file_name = createLogFileName(_log_prefix_backup);
		std::string prospect_log = directory + file_name;
		std::unique_ptr<std::ofstream> log_stream = createLogFile(prospect_log);
		if (nullptr == log_stream) {
			filestream() << "\n" << now_formatted << " Unable to change log file. Illegal filename or busy? Unsuccessful log name was: " << prospect_log;
			return {}; // no success
		}

		addLogFileHeader();
		std::ostringstream ss_change;
		ss_change << "\n\tChanging log file from : " << _log_file_with_path;
		ss_change << "\n\tto new location: " << prospect_log << "\n";
		filestream() << now_formatted << ss_change.str();
		ss_change.str("");

		std::string old_log = _log_file_with_path;
		_log_file_with_path = prospect_log;
		_outptr = std::move(log_stream);
		ss_change << "\n\tNew log file. The previous log file was at: ";
		ss_change << old_log << "\n";
		filestream() << now_formatted << ss_change.str();
		return _log_file_with_path;
	}
	std::string LogFileSink::fileName() {
		return _log_file_with_path;
	}
	void LogFileSink::addLogFileHeader() {
		filestream() << "Orangine - Copyright Lewis Weaver 2018" << std::endl << std::endl << header();
	}

} // OE
