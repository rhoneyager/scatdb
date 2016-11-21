#include "../private/versioning.hpp"
#include "../scatdb/error.hpp"
#include "../scatdb/hash.hpp"
#include <cstring>
namespace scatdb {
	namespace versioning {
		namespace internal {
			versionInfo_p ver_int;
		}
		ver_match compareVersions(const versionInfo &a, const versionInfo &b)
		{
			ver_match res = INCOMPATIBLE;

#define tryNum(x) if (a.vn[versionInfo:: x] != b.vn[versionInfo:: x]) return res;
#define tryBool(x) if (a.vb[versionInfo:: x] != b.vb[versionInfo:: x]) return res;
#define tryStr(x) if (std::strncmp(a. x, b. x, versionInfo::charmax ) != 0) return res;
#define tryNumB(x) if ((a.vn[versionInfo:: x] != b.vn[versionInfo:: x]) && a.vn[versionInfo:: x]) return res;
			// First filter the incompatible stuff
			tryStr(vassembly);
			tryNum(V_MAJOR);
			tryNum(V_MINOR);
			tryBool(V_AMD64);
			tryBool(V_X64);
			tryBool(V_UNIX);
			tryBool(V_APPLE);
			tryBool(V_WIN32);
			tryStr(vboost);
			tryNum(V_MSCVER);

			res = COMPATIBLE_1;
			tryNumB(V_GNUC_MAJ);
			tryNumB(V_MINGW_MAJ);
			tryNumB(V_SUNPRO);
			tryNumB(V_PATHCC_MAJ);
			tryNumB(V_CLANG_MAJ);
			tryNumB(V_INTEL);

			res = COMPATIBLE_2;
			tryNumB(V_GNUC_MIN);
			tryNumB(V_MINGW_MIN);
			tryNumB(V_PATHCC_MIN);
			tryNumB(V_CLANG_MIN);
			tryNum(V_REVISION);

			res = COMPATIBLE_3;
			tryBool(V_OPENMP);
			tryNum(V_SVNREVISION);
			tryStr(vgithash);
			tryBool(V_HAS_ZLIB);
			tryBool(V_HAS_GZIP);
			tryBool(V_HAS_BZIP2);
			tryBool(V_HAS_SZIP);

			res = EXACT_MATCH;
			return res;
#undef tryNum
#undef tryBool
#undef tryStr
#undef tryNumB
		}

		void getLibVersionInfo(versionInfo &out)
		{
			genVersionInfo(out);
		}

		versionInfo_p getLibVersionInfo()
		{
			if (internal::ver_int) return internal::ver_int;
			std::shared_ptr<versionInfo> vi(new versionInfo);
			getLibVersionInfo(*(vi.get()));
			internal::ver_int = vi;
			return internal::ver_int;
		}

		hash::HASH_p getHashOfVersionInfo(const versionInfo &in) {
			std::shared_ptr< hash::HASH_t> res(new hash::HASH_t);
			hash::HASH_p hBools = hash::HASH(in.vb, sizeof(in.vb));
			hash::HASH_p hNums = hash::HASH(in.vn, sizeof(in.vn));

			hash::HASH_p hvdate = hash::HASH(in.vdate, (int) sizeof(char) * (int)strnlen_s(in.vdate, versionInfo::charmax));
			hash::HASH_p hvtime = hash::HASH(in.vtime, (int) sizeof(char) * (int)strnlen_s(in.vtime, versionInfo::charmax));
			hash::HASH_p hvsdate = hash::HASH(in.vsdate, (int) sizeof(char) * (int)strnlen_s(in.vsdate, versionInfo::charmax));
			hash::HASH_p hvssource = hash::HASH(in.vssource, (int) sizeof(char) * (int)strnlen_s(in.vssource, versionInfo::charmax));
			hash::HASH_p hvsuuid = hash::HASH(in.vsuuid, (int) sizeof(char) * (int)strnlen_s(in.vsuuid, versionInfo::charmax));
			hash::HASH_p hvboost = hash::HASH(in.vboost, (int) sizeof(char) * (int)strnlen_s(in.vboost, versionInfo::charmax));
			hash::HASH_p hvassembly = hash::HASH(in.vassembly, (int) sizeof(char) * (int)strnlen_s(in.vassembly, versionInfo::charmax));
			hash::HASH_p hvgithash = hash::HASH(in.vgithash, (int) sizeof(char) * (int)strnlen_s(in.vgithash, versionInfo::charmax));
			hash::HASH_p hvgitbranch = hash::HASH(in.vgitbranch, (int) sizeof(char) * (int)strnlen_s(in.vgitbranch, versionInfo::charmax));

			*res = *(hBools.get()) ^ *(hNums.get()) ^ *(hvdate.get()) ^ *(hvtime.get()) ^ *(hvsdate.get()) ^ *(hvssource.get())
				^ *(hvsuuid.get()) ^ *(hvboost.get()) ^ *(hvassembly.get()) ^ *(hvgithash.get()) ^ *(hvgitbranch.get());
			return res;
		}

		const std::string versionInfo::stringifyBool(int bools) {
			if (bools == V_DEBUG) return std::string("V_DEBUG");
			if (bools == V_OPENMP) return std::string("V_OPENMP");
			if (bools == V_AMD64) return std::string("V_AMD64");
			if (bools == V_X64) return std::string("V_X64");
			if (bools == V_UNIX) return std::string("V_UNIX");
			if (bools == V_APPLE) return std::string("V_APPLE");
			if (bools == V_WIN32) return std::string("V_WIN32");
			if (bools == V_LLVM) return std::string("V_LLVM");
			if (bools == V_HAS_BZIP2) return std::string("V_HAS_BZIP2");
			if (bools == V_HAS_GZIP) return std::string("V_HAS_GZIP");
			if (bools == V_HAS_ZLIB) return std::string("V_HAS_ZLIB");
			if (bools == V_HAS_SZIP) return std::string("V_HAS_SZIP");
			SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "Input out of range.");
			return std::string("");
		}

		const std::string versionInfo::stringifyInt(int bools) {
			if (bools == V_MAJOR) return std::string("V_MAJOR");
			if (bools == V_MINOR) return std::string("V_MINOR");
			if (bools == V_REVISION) return std::string("V_REVISION");
			if (bools == V_SVNREVISION) return std::string("V_SVNREVISION");
			if (bools == V_MSCVER) return std::string("V_MSCVER");
			if (bools == V_GNUC_MAJ) return std::string("V_GNUC_MAJ");
			if (bools == V_GNUC_MIN) return std::string("V_GNUC_MIN");
			if (bools == V_GNUC_PATCH) return std::string("V_GNUC_PATCH");
			if (bools == V_MINGW_MAJ) return std::string("V_MINGW_MAJ");
			if (bools == V_MINGW_MIN) return std::string("V_MINGW_MIN");
			if (bools == V_SUNPRO) return std::string("V_SUNPRO");
			if (bools == V_PATHCC_MAJ) return std::string("V_PATHCC_MAJ");
			if (bools == V_PATHCC_MIN) return std::string("V_PATHCC_MIN");
			if (bools == V_PATHCC_PATCH) return std::string("V_PATHCC_PATCH");
			if (bools == V_CLANG_MAJ) return std::string("V_CLANG_MAJ");
			if (bools == V_CLANG_MIN) return std::string("V_CLANG_MIN");
			if (bools == V_CLANG_PATCH) return std::string("V_CLANG_PATCH");
			if (bools == V_INTEL) return std::string("V_INTEL");
			if (bools == V_INTEL_DATE) return std::string("V_INTEL_DATE");
			SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "Input out of range.");
			return std::string("");
		}
	}
}
