#include "../scatdb/defs.hpp"
#include "../scatdb/scatdb.hpp"
#include "../scatdb/scatdb_liu.h"
#include "../scatdb/debug.hpp"
#include "../private/info.hpp"
#include "../scatdb/error.hpp"
#include <boost/filesystem.hpp>
#include <string>

namespace scatdb {
	namespace scatdb_liu {
		std::string sdb_loc;
		void set_scatdb_location(const char* p) {
			if (!p) SDBR_throw(scatdb::error::error_types::xNullPointer);
			sdb_loc = std::string(p);
		}
		const char* get_scatdb_location() {
			if (sdb_loc.size()) return sdb_loc.c_str();

			const std::string envVar("SCATDB_DATA");
			
			auto search = [&](const std::string& base)->bool {
				using namespace boost::filesystem;
				path pp(base);
				if (!exists(pp)) return false;
				const path defaultFileName("scat_db2.dda");
				path ppf = pp / defaultFileName;
				if (exists(ppf)) {
					sdb_loc = ppf.string();
					return true;
				}
				return false;
			};

			bool res = false;
			auto m = scatdb::debug::getCurrentAppInfo();
			const auto &mEnv = m->pInfo->expandedEnviron;
			if (mEnv.count(envVar)) {
				if (search(mEnv.at(envVar))) return sdb_loc.c_str();
			}
			if (search(m->pInfo->cwd)) return sdb_loc.c_str();
			if (search(m->pInfo->path)) return sdb_loc.c_str();
			if (search(m->appConfigDir)) return sdb_loc.c_str();

			SDBR_throw(scatdb::error::error_types::xMissingFile)
				.add<std::string>("Reason", "Cannot find scat_db2.dda. "
					"Please set the SCATDB_DATA environment variable to point "
					"to its containing folder.");
			return sdb_loc.c_str(); // Vestigial, but avoids compiler warning
		}
	}

	void db::readDBscatdb(std::shared_ptr<db> res, const char* dbfile) {
		if (dbfile) _set_scatdb_location(dbfile);
		/*
		static float fs[NFREQ], ts[NTEMP], szs[NSIZE][NSHAP], abss[NFREQ][NTEMP][NSHAP][NSIZE],
		scas[NFREQ][NTEMP][NSHAP][NSIZE],
		bscs[NFREQ][NTEMP][NSHAP][NSIZE], gs[NFREQ][NTEMP][NSHAP][NSIZE],
		reff[NSIZE][NSHAP], pqs[NFREQ][NTEMP][NSHAP][NSIZE][NQ];
		static int shs[NSHAP], mf, mt, msh, msz[NSHAP];
		*/
		std::unique_ptr<float[]>
			o_fs(new float[NFREQ]),
			o_ts(new float[NTEMP]),
			o_szs(new float[NSIZE*NSHAP]),
			o_abss(new float[NFREQ*NTEMP*NSHAP*NSIZE]),
			o_scas(new float[NFREQ*NTEMP*NSHAP*NSIZE]),
			o_bscs(new float[NFREQ*NTEMP*NSHAP*NSIZE]),
			o_gs(new float[NFREQ*NTEMP*NSHAP*NSIZE]),
			o_reff(new float[NSIZE*NSHAP]),
			o_pqs(new float[NFREQ*NTEMP*NSHAP*NSIZE*NQ]);
		std::unique_ptr<int[]>
			o_shs(new int[NSHAP]),
			o_msz(new int[NSHAP]);
		for (int i = 0; i < NFREQ; ++i) o_fs.get()[i] = -1;
		int o_mf = 0, o_mt = 0, o_msh = 0;
		int retval = liu_scatdb_onlyread(
			o_fs.get(), o_ts.get(), o_szs.get(), o_abss.get(), o_scas.get(),
			o_bscs.get(), o_gs.get(), o_reff.get(), o_pqs.get(),
			o_shs.get(), o_msz.get(), &o_mf, &o_mt, &o_msh);

		// Now, decompose into database entries
		// There are NFREQ*NTEMP*NSHAP*NSIZE entries
		auto getIndex4 = [](int f, int t, int s, int z) -> int {
			int res = f; res *= NTEMP;
			res += t; res *= NSHAP;
			res += s; res *= NSIZE;
			res += z; return res;
		};
		auto getIndexSizeShape = [](int size, int shape) -> int {
			int res = size; res *= NSHAP; res += shape; return res;
		};
		int numFreqs = NFREQ, numTemps = NTEMP, numShapes = NSHAP, numSizes = NSIZE;
		int n4 = NFREQ*NTEMP*NSHAP*NSIZE;
		/*
		for (int i = 0; i < numShapes; ++i) std::cout << " " << *(o_shs.get() + i);
		std::cout << std::endl;
		for (int i = 0; i < numShapes; ++i) std::cout << " " << *(o_msz.get() + i);
		std::cout << std::endl;
		for (int i = 0; i < numFreqs; ++i) std::cout << " " << *(o_fs.get() + i);
		std::cout << std::endl;
		for (int i = 0; i < numTemps; ++i) std::cout << " " << *(o_ts.get() + i);
		std::cout << std::endl;
		std::cout << "numFreqs = " << numFreqs << " numTemps = " << numTemps
			<< " numShapes " << numShapes << " numSizes " << numSizes << std::endl;
		*/
		res->floatMat.resize(NFREQ*NTEMP*NSHAP*NSIZE,data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		res->intMat.resize(NFREQ*NTEMP*NSHAP*NSIZE, data_entries::SDBR_NUM_DATA_ENTRIES_INTS);
		res->floatMat.setConstant(-1);
		res->intMat.setConstant(-1);
		int i = 0, line = -1;
		for (int ifreq = 0; ifreq < numFreqs; ++ifreq) {
			for (int itemp = 0; itemp < numTemps; ++itemp) {
				for (int ishape = 0; ishape < numShapes; ++ishape) {
					for (int isize = 0; isize < numSizes; ++isize) {
						line++;
						int idx4 = getIndex4(ifreq, itemp, ishape, isize);
						int idxss = getIndexSizeShape(isize, ishape);
						// &fts, &its
						auto fts = res->floatMat.block<1, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS>(i, 0);
						auto its = res->intMat.block<1, data_entries::SDBR_NUM_DATA_ENTRIES_INTS>(i, 0);
						bool invalid = false;
						its(data_entries::SDBR_FLAKETYPE) = o_shs.get()[ishape];
						if (its(data_entries::SDBR_FLAKETYPE) < 0) invalid = true;
						fts(data_entries::SDBR_FREQUENCY_GHZ) = o_fs.get()[ifreq];
						if (fts(data_entries::SDBR_FREQUENCY_GHZ) <= 0) invalid = true;
						fts(data_entries::SDBR_TEMPERATURE_K) = o_ts.get()[itemp];
						if (fts(data_entries::SDBR_TEMPERATURE_K) <= 0) invalid = true;
						fts(data_entries::SDBR_AEFF_UM) = o_reff.get()[idxss];
						if (fts(data_entries::SDBR_AEFF_UM) <= 0) invalid = true;
						fts(data_entries::SDBR_MAX_DIMENSION_MM) = o_szs.get()[idxss] / 1000;
						if (fts(data_entries::SDBR_MAX_DIMENSION_MM) <= 0) invalid = true;
						fts(data_entries::SDBR_CABS_M) = o_abss.get()[idx4];
						if (fts(data_entries::SDBR_CABS_M) <= 0) invalid = true;
						fts(data_entries::SDBR_CBK_M) = o_bscs.get()[idx4];
						fts(data_entries::SDBR_CSCA_M) = o_scas.get()[idx4];
						fts(data_entries::SDBR_CEXT_M) = fts(data_entries::SDBR_CSCA_M) + fts(data_entries::SDBR_CABS_M);
						fts(data_entries::SDBR_G) = o_gs.get()[idx4];
						fts(data_entries::SDBR_AS_XY) = -1;
						/*
						std::cout << "i = " << i << " line = " << line
							<< " idx4 " << idx4 << " idxss " << idxss
							<< "   ifreq " << ifreq << " itemp " << itemp << " ishape " << ishape
							<< " isize " << isize << std::endl
							<< its(data_entries::SDBR_FLAKETYPE) << "  "
							<< fts << std::endl;
						*/
						if (invalid) {
							continue;
						}
						++i;
					}
				}
			}
		}
		res->floatMat.conservativeResize(i, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		res->intMat.conservativeResize(i, data_entries::SDBR_NUM_DATA_ENTRIES_INTS);
	}
}
#ifdef __cplusplus
extern "C" {
#endif
	const char* _get_scatdb_location() {
		return scatdb::scatdb_liu::get_scatdb_location();
	}
	void _set_scatdb_location(const char* loc) {
		return scatdb::scatdb_liu::set_scatdb_location(loc);
	}
#ifdef __cplusplus
};
#endif
