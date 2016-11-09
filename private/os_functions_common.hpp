#pragma once
#include "../scatdb/defs.hpp"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "../scatdb/debug.hpp"
#include "info_fwd.hpp"

// This header should not be publicly accessible.
namespace scatdb {
	namespace debug {
		namespace vars {
			extern size_t sys_num_threads;
			extern std::mutex m_sys_num_threads, m_sys_names;
			extern std::string hostname, username, homeDir, appConfigDir, moduleCallbackBuffer;
			extern bool _consoleTerminated;
			extern std::vector<std::pair<std::string, std::string> > loadedModulesList;
			extern bool doWaitOnExit;
			extern currentAppInfo_p _currentAppInfo;
		}
	}
}
