#pragma once
#include "defs.hpp"
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifdef ERROR
#undef ERROR
#endif
#endif

namespace scatdb {
	namespace logging {
		enum PRIORITIES {
			DEBUG_2,
			DEBUG_1,
			INFO,
			NOTIFICATION,
			NORMAL,
			WARNING,
			ERROR,
			CRITICAL
		};
		struct log_properties {
			//log_properties();
			bool debugChannel;
			::std::string logFile;
			int consoleLogThreshold;
		};
		DLEXPORT_SDBR void emit_log(
			const std::string &channel,
			const std::string &message,
			PRIORITIES p = NORMAL);
		DLEXPORT_SDBR void setupLogging(
			int argc = 0,
			char** argv = nullptr,
			const log_properties* lps = nullptr);
	}
}

#define ryan_log(c,p,x) { ::std::ostringstream l; l << x; \
	::std::string s = l.str(); \
	::scatdb::logging::emit_log(c, s, p); }
