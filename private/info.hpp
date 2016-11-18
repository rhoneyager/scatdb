#pragma once
#include "../scatdb/defs.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "info_fwd.hpp"

namespace scatdb {
	namespace debug {

		/// Contains information about a process
		struct processInfo
		{
			/// Executable name
			std::string name;
			/// Executable path
			std::string path;
			/// Current working directory
			std::string cwd;
			/// Environment variables
			std::string environment;
			/// Command-line
			std::string cmdline;
			/// Command line number of arguments
			//int argc;
			/// Command line argument buffer
			//const char **argv;
			/// Command line argument buffer backend
			std::vector<std::string> argv_v;
			/// Process start time
			std::string startTime;
			/// Process ID
			int pid;
			/// Process ID of parent
			int ppid;

			std::map<std::string, std::string> expandedEnviron;
			std::vector<std::string> expandedCmd;
		};

		/// Contains information about a shared library
		struct moduleInfo
		{
			std::string path;
		};

		/// Contains information about the running application
		struct currentAppInfo {
			/// Username of the executing user
			std::string username;
			/// Hostname of the current machine
			std::string hostname;
			/// Stable per-user configuration directory for the application
			std::string appConfigDir;
			/// User home directory
			std::string homeDir;
			/// Other information about the current process.
			processInfo_p pInfo;
			/// Information about the scatdb library's build environment
			versioning::versionInfo_p vInfoLib;
		};

	}
}
