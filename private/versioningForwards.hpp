#pragma once
#include "../scatdb/defs.hpp"
#include <memory>
#include <cstdint>
#include <string>
#include "../scatdb/hashForwards.hpp"

namespace scatdb {
	namespace versioning {
		struct versionInfo;
		typedef std::shared_ptr<const versionInfo> versionInfo_p;

		enum ver_match {
			INCOMPATIBLE, COMPATIBLE_1, COMPATIBLE_2, COMPATIBLE_3, EXACT_MATCH
		};

		DLEXPORT_SDBR ver_match compareVersions(const versionInfo &a, const versionInfo &b);

		/// Internal scatdb version
		DLEXPORT_SDBR void getLibVersionInfo(versionInfo &out);
		DLEXPORT_SDBR hash::HASH_p getHashOfVersionInfo(const versionInfo&);
		DLEXPORT_SDBR versionInfo_p getLibVersionInfo();

	}
}
