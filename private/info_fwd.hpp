#pragma once
#include "../scatdb/defs.hpp"
#include <memory>
#include <iostream>

namespace scatdb {
	namespace debug {

		/// Contains information about a process
		struct processInfo;
		typedef std::shared_ptr<const processInfo> processInfo_p;
		//typedef const processInfo* hProcessInfo;

		/// Contains information about a shared library
		struct moduleInfo;
		typedef std::shared_ptr<const moduleInfo> moduleInfo_p;
		//typedef const moduleInfo* hModuleInfo;

		/// Contains information about the running application
		struct currentAppInfo;
		typedef std::shared_ptr<const currentAppInfo> currentAppInfo_p;

		/// Return a structure containing information about the currently running application
		HIDDEN_SDBR currentAppInfo_p getCurrentAppInfo();

		/// Return a structure containing information about a process
		HIDDEN_SDBR processInfo_p getProcessInfo(int pid);

		/// Return a structure containing information about the module containing the given function
		HIDDEN_SDBR moduleInfo_p getModuleInfo(void*);

	}
	namespace versioning {
		/// Contains information about the version of a piece of code
		struct versionInfo;
		typedef std::shared_ptr<const versionInfo> versionInfo_p;
	}
}
/// Allows writing of a processInfo structure to a stream
HIDDEN_SDBR std::ostream &  operator<<(std::ostream&, const scatdb::debug::processInfo&);

