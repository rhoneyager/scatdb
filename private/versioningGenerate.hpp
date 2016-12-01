#pragma once
#include "../scatdb/defs.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include "versioning.hpp"
#include "cmake-settings.h"

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

namespace scatdb {
	namespace versioning {
		/// Calculates version string based on compile-time version of external code.
		inline void genVersionInfo(versionInfo &out)
		{
			for (size_t i = 0; i < versionInfo::V_MAX_INTS; ++i) out.vn[i] = 0;
			for (size_t i = 0; i < versionInfo::V_MAX_BOOLS; ++i) out.vb[i] = false;
			out.vsdate[0] = '\0'; out.vssource[0] = '\0'; out.vsuuid[0] = '\0';
			out.vboost[0] = '\0'; out.vassembly[0] = '\0';
			strncpy_s(out.vdate, versionInfo::charmax, __DATE__, versionInfo::charmax);
			strncpy_s(out.vtime, versionInfo::charmax, __TIME__, versionInfo::charmax);
			strncpy_s(out.vgithash, versionInfo::charmax, scatdb_GitHash, versionInfo::charmax);
			strncpy_s(out.vgitbranch, versionInfo::charmax, scatdb_GitBranch, versionInfo::charmax);

			out.vn[versionInfo::V_MAJOR] = scatdb_MAJOR;
			out.vn[versionInfo::V_MINOR] = scatdb_MINOR;
			out.vn[versionInfo::V_REVISION] = scatdb_REVISION;

			out.vb[versionInfo::V_HAS_BZIP2] = COMPRESS_BZIP2;
			out.vb[versionInfo::V_HAS_GZIP] = COMPRESS_GZIP;
			out.vb[versionInfo::V_HAS_ZLIB] = COMPRESS_ZLIB;
			out.vb[versionInfo::V_HAS_SZIP] = COMPRESS_SZIP;

#ifdef __GNUC__
			out.vn[versionInfo::V_GNUC_MAJ] = __GNUC__;
			out.vn[versionInfo::V_GNUC_MIN] = __GNUC_MINOR__;
			out.vn[versionInfo::V_GNUC_PATCH] = __GNUC_PATCHLEVEL__;
#endif
#ifdef __MINGW32__
			out.vn[versionInfo::V_MINGW_MAJ] = __MINGW32_MAJOR_VERSION;
			out.vn[versionInfo::V_MINGW_MIN] = __MINGW32_MINOR_VERSION;
#endif
#ifdef __SUNPRO_CC
			out.vn[versionInfo::V_SUNPRO] = __SUNPRO_CC;
#endif
#ifdef __PATHCC__
			out.vn[versionInfo::V_PATHCC_MAJ] = __PATHCC__;
			out.vn[versionInfo::V_PATHCC_MIN] = __PATHCC_MINOR__;
			out.vn[versionInfo::V_PATHCC_PATCH] = __PATHCC_PATCHLEVEL__;
#endif
#ifdef __llvm__
			out.vb[versionInfo::V_LLVM] = true;
#endif
#ifdef __clang__
			out.vn[versionInfo::V_CLANG_MAJ] = __clang_major__;
			out.vn[versionInfo::V_CLANG_MIN] = __clang_minor__;
			out.vn[versionInfo::V_CLANG_PATCH] = __clang_patchlevel__;
#endif
#ifdef __INTEL_COMPILER
			out.vn[versionInfo::V_INTEL] = __INTEL_COMPILER;
			out.vn[versionInfo::V_INTEL_DATE] = __INTEL_COMPILER_BUILD_DATE;
#endif
#ifdef _MSC_FULL_VER
			out.vn[versionInfo::V_MSCVER] = _MSC_FULL_VER;
#endif
#ifdef BOOST_LIB_VERSION
			strncpy_s(out.vboost, versionInfo::charmax, BOOST_LIB_VERSION, versionInfo::charmax);
#endif
#ifdef _DEBUG
			out.vb[versionInfo::V_DEBUG] = true;
#endif
#ifdef _OPENMP
			out.vb[versionInfo::V_OPENMP] = true;
#endif
#ifdef __amd64
			out.vb[versionInfo::V_AMD64] = true;
#endif
#ifdef _M_X64
			out.vb[versionInfo::V_X64] = true;
#endif
#ifdef __unix__
			out.vb[versionInfo::V_UNIX] = true;
#endif
#ifdef __APPLE__
			out.vb[versionInfo::V_APPLE] = true;
#endif
#ifdef _WIN32
			out.vb[versionInfo::V_WIN32] = true;
#endif
		}

		inline void debug_preamble(std::ostream &out = std::cerr)
		{
			versionInfo v;
			genVersionInfo(v);
			debug_preamble(v, out);
		}
	}
}
#undef QUOTE
#undef EXPAND_AND_QUOTE
