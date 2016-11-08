#include "../scatdb/logging.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <boost/version.hpp>
//#include "cmake-settings.h"
namespace {
	int logConsoleThreshold = 0;
	std::string logFile;
	std::shared_ptr<std::ofstream> lOut;
}
namespace scatdb {
	namespace logging {
		void emit_log(
			const std::string &channel,
			const std::string &message,
			PRIORITIES p) {
			if (p >= logConsoleThreshold)
				std::cerr << channel << " - " << message << std::endl;
			if (lOut) {
				*(lOut.get()) << channel << " - " << message << std::endl;
			}
		}
		void setupLogging(
			int argc,
			char** argv,
			const log_properties* lps) {
			if (lps) {
				logConsoleThreshold = lps->consoleLogThreshold;
				logFile = lps->logFile;
				if (logFile.size()) {
					lOut = std::shared_ptr<std::ofstream>(new std::ofstream(logFile.c_str()));
				}
			}
		}

	}
}

