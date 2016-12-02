#include "../scatdb/logging.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <boost/version.hpp>
#include "../private/os_functions_common.hpp"
//#include "cmake-settings.h"
namespace {
	int logConsoleThreshold = 0;
	int logDebugThreshold = 0;
	std::string logFile;
	std::shared_ptr<std::ofstream> lOut;
}
namespace scatdb {
	namespace logging {
		void emit_log(
			const std::string &channel,
			const std::string &message,
			PRIORITIES p) {
			std::string m;
			std::ostringstream out;
			out << channel << " - " << message << std::endl;
			m = out.str();
			if (p >= logConsoleThreshold)
				std::cerr << m;
			if (p >= logDebugThreshold) {
				debug::writeDebugStr(m);
			}
			if (lOut) {
				*(lOut.get()) << m;
			}
		}
		void setupLogging(
			int argc,
			char** argv,
			const log_properties* lps) {
			if (lps) {
				logConsoleThreshold = lps->consoleLogThreshold;
				logDebugThreshold = lps->debuggerLogThreshold;
				logFile = lps->logFile;
				if (logFile.size()) {
					lOut = std::shared_ptr<std::ofstream>(new std::ofstream(logFile.c_str()));
				}
			}
		}

	}
}

