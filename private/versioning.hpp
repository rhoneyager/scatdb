#pragma once
#include "../scatdb/defs.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include "versioningForwards.hpp"

namespace scatdb {
	namespace versioning {
		struct versionInfo {
			enum nums {
				V_MAJOR, V_MINOR, V_REVISION, V_SVNREVISION,
				V_MSCVER,
				V_GNUC_MAJ, V_GNUC_MIN, V_GNUC_PATCH,
				V_MINGW_MAJ, V_MINGW_MIN,
				V_SUNPRO,
				V_PATHCC_MAJ, V_PATHCC_MIN, V_PATHCC_PATCH,
				V_CLANG_MAJ, V_CLANG_MIN, V_CLANG_PATCH,
				V_INTEL, V_INTEL_DATE,
				V_MAX_INTS
			};
			enum bools {
				V_DEBUG, V_OPENMP, V_AMD64, V_X64, V_UNIX, V_LINUX, V_BSD, V_APPLE, V_WIN32,
				V_LLVM, V_HAS_BZIP2, V_HAS_GZIP, V_HAS_ZLIB, V_HAS_SZIP,
				V_MAX_BOOLS
			};

			uint64_t vn[V_MAX_INTS];
			bool vb[V_MAX_BOOLS];

			static const uint64_t charmax = 256;
			char vdate[charmax];
			char vtime[charmax];
			char vsdate[charmax];
			char vssource[charmax];
			char vsuuid[charmax];
			char vboost[charmax];
			char vassembly[charmax];
			char vgithash[charmax];
			char vgitbranch[charmax];

			static const std::string stringifyBool(int);
			static const std::string stringifyInt(int);
		};
	}
}
