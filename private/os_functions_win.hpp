#pragma once
#ifdef _WIN32
#include <string>
#include <Windows.h>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "os_functions_common.hpp"
#include "info_fwd.hpp"
namespace boost {
	namespace filesystem {
		class path;
	}
}
namespace scatdb {
	namespace debug {
		namespace win {
			/// Convert from unicode string to multibyte
			HIDDEN_SDBR std::string convertStr(const LPTSTR instr);
			HIDDEN_SDBR std::string convertStr(const PWSTR instr);
			/// Windows function for getting process name and path
			HIDDEN_SDBR bool getPathWIN32(DWORD pid, ::boost::filesystem::path &modPath, ::boost::filesystem::path &filename);
			HIDDEN_SDBR bool IsAppRunningAsAdminMode();
			/// Windows handler for window close events.
			HIDDEN_SDBR BOOL WINAPI _CloseHandlerRoutine(DWORD dwCtrlType);
			HIDDEN_SDBR bool pidExists(int pid);
			HIDDEN_SDBR void appEntry();
			HIDDEN_SDBR bool waitOnExitForce();
			HIDDEN_SDBR int getPID();
			HIDDEN_SDBR int getPPID(int pid);
			HIDDEN_SDBR HMODULE GetCurrentModule();
			HIDDEN_SDBR std::string GetModulePath(HMODULE mod = NULL);
			HIDDEN_SDBR const std::string& getUsername();
			HIDDEN_SDBR const std::string& getHostname();
			HIDDEN_SDBR const std::string& getAppConfigDir();
			HIDDEN_SDBR const std::string& getHomeDir();
			HIDDEN_SDBR processInfo_p getInfo(int pid);
			HIDDEN_SDBR moduleInfo_p getModuleInfo(void* func);
			HIDDEN_SDBR void enumModules(int pid, std::ostream &out = std::cerr);

			HIDDEN_SDBR void writeDebugStr(const std::string & instr);
		}
	}
}
#endif
