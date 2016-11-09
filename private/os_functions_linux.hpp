#pragma once
#include <string>
#include "os_functions_common.hpp"
#include "info_fwd.hpp"
namespace scatdb {
	namespace debug {
		namespace linux {

#ifdef __unix__
			HIDDEN_SDBR bool pidExists(int pid);
			HIDDEN_SDBR bool waitOnExitForce();
			HIDDEN_SDBR int getPID();
			HIDDEN_SDBR int getPPID(int pid);
			HIDDEN_SDBR std::string GetModulePath(void *addr = NULL);
			HIDDEN_SDBR int moduleCallback(dl_phdr_info *info, size_t sz, void* data);
			HIDDEN_SDBR const std::string& getUsername();
			HIDDEN_SDBR const std::string& getHostname();
			HIDDEN_SDBR const std::string& getAppConfigDir();
			HIDDEN_SDBR const std::string& getHomeDir();
			HIDDEN_SDBR processInfo_p getInfo(int pid);
			HIDDEN_SDBR moduleInfo_p getModuleInfo(void* func);
			HIDDEN_SDBR void enumModules(int pid, std::ostream &out = std::cerr);
#endif
		}
	}
}
