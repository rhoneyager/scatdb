#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include "../scatdb/debug.hpp"
#include "../private/info.hpp"
#include "../scatdb/scatdb.h"
#include "../scatdb/scatdb.hpp"

namespace {
	std::string lastErr;
	std::map<const scatdb::scatdb_base*, std::shared_ptr<const scatdb::scatdb_base> > ptrs;
	//std::vector<std::shared_ptr<const scatdb::scatdb_base> > ptrs;
	// TODO: add casting checks again
}

extern "C" {

	bool SDBR_free(SDBR_HANDLE h) {
		using namespace scatdb;
		scatdb_base* hp = (scatdb_base*) (h);
		if (ptrs.count(hp)) {
			ptrs[hp].reset();
			return true;
		}
		return false;
		return false;
	}

	int SDBR_err_len() {
		if (lastErr.size()) return (int) lastErr.size() + 1;
		return 0;
	}

	void prepString(uint64_t maxlen, char* buffer, const std::string &s) {
		size_t cplen = (size_t) maxlen;
		if (s.size() + 1 < maxlen) cplen = s.size();
		strncpy_s(buffer, cplen, s.c_str(), cplen);
		buffer[cplen] = '\0';
	}

	uint64_t SDBR_err_msg(uint64_t maxlen, char* buffer) {
		prepString(maxlen, buffer, lastErr);
		return 0;
	}

	uint64_t SDBR_findDB(uint64_t maxlen, char* buffer) {
		using namespace scatdb;
		try {
			if (!buffer) {
				lastErr = "SDBR_findDB buffer is null";
				return 0;
			}
			std::string dbloc(buffer, maxlen);
			bool res = db::findDB(dbloc);
			if (!res) return 0;
			prepString(maxlen, buffer, dbloc);
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return 0;
		}
		return 0;
	}

	SDBR_HANDLE SDBR_loadDB(const char* dbfile) {
		using namespace scatdb;
		try {
			auto d = db::loadDB(dbfile);
			ptrs[d.get()] = d;
			//ptrs.push_back(d);
			lastErr = "";
			return (SDBR_HANDLE)(d.get());
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return 0;
		}
	}

	bool SDBR_writeDBtext(SDBR_HANDLE handle, const char* outfile) {
		using namespace scatdb;
		try {
			const scatdb_base* hp = ( const scatdb_base* )(handle);
			const db* h = dynamic_cast<const db*>(hp);
			std::ofstream out(outfile);
			(h)->print(out);
			lastErr="";
		} catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool SDBR_writeDBHDF(SDBR_HANDLE handle, const char* outfile,
		SDBR_write_type wt, const char* hdfpath) {
		using namespace scatdb;
		try {
			const scatdb_base* hp = ( const scatdb_base* )(handle);
			const db* h = dynamic_cast<const db*>(hp);
			(h)->writeHDFfile(outfile, wt, hdfpath);
			lastErr="";
		} catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	uint64_t SDBR_getNumRows(SDBR_HANDLE handle) {
		using namespace scatdb;
		uint64_t res = 0;
		try {
			const scatdb_base* hp = ( const scatdb_base* )(handle);
			const db* h = dynamic_cast<const db*>(hp);
			res = (uint64_t) (h)->floatMat.rows();
			lastErr="";
		} catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool SDBR_start(int argc, char** argv) {
		try {
			namespace po = boost::program_options;
			po::options_description desc("Allowed options"), cmdline("Command-line options"),
				config("Config options"), hidden("Hidden options"), oall("all options");

			scatdb::debug::add_options(cmdline, config, hidden);

			desc.add(cmdline).add(config);
			oall.add(cmdline).add(config).add(hidden);

			po::variables_map vm;
			po::store(po::command_line_parser(argc, argv).allow_unregistered().
				options(oall).run(), vm);
			po::notify(vm);

			scatdb::debug::process_static_options(vm);
		} catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}
	bool SDBR_start_alt()
	{
		try {
			namespace po = boost::program_options;
			po::options_description desc("Allowed options"), cmdline("Command-line options"),
				config("Config options"), hidden("Hidden options"), oall("all options");

			scatdb::debug::add_options(cmdline, config, hidden);

			desc.add(cmdline).add(config);
			oall.add(cmdline).add(config).add(hidden);

			po::variables_map vm;
			auto pbase = scatdb::debug::getCurrentAppInfo();
			po::store(po::command_line_parser(pbase->pInfo->argv_v).allow_unregistered().
				options(oall).run(), vm);
			po::notify(vm);

			scatdb::debug::process_static_options(vm);
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool DLEXPORT_SDBR SDBR_getFloatTableSize(
		SDBR_HANDLE handle, uint64_t *numFloats, uint64_t *numBytes)
	{
		using namespace scatdb;
		uint64_t res = 0;
		try {
			const scatdb_base* hp = (const scatdb_base*)(handle);
			const db* h = dynamic_cast<const db*>(hp);
			uint64_t rows = (uint64_t)(h)->floatMat.rows();
			uint64_t cols = (uint64_t)(h)->floatMat.cols();
			*numFloats = rows * cols;
			*numBytes = *numFloats * sizeof(float);
			lastErr = "";
		}
		catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool DLEXPORT_SDBR SDBR_getIntTableSize(
		SDBR_HANDLE handle, uint64_t *numInts, uint64_t *numBytes)
	{
		using namespace scatdb;
		uint64_t res = 0;
		try {
			const scatdb_base* hp = (const scatdb_base*)(handle);
			const db* h = dynamic_cast<const db*>(hp);
			uint64_t rows = (uint64_t)(h)->intMat.rows();
			uint64_t cols = (uint64_t)(h)->intMat.cols();
			*numInts = rows * cols;
			*numBytes = *numInts * sizeof(uint64_t);
			lastErr = "";
		}
		catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool DLEXPORT_SDBR SDBR_getStatsTableSize(
		uint64_t *numFloats, uint64_t *numBytes)
	{
		using namespace scatdb;
		uint64_t res = 0;
		try {
			
			uint64_t rows = (uint64_t)scatdb::db::data_entries::SDBR_NUM_DATA_ENTRIES_STATS;
			uint64_t cols = (uint64_t)scatdb::db::data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS;
			*numFloats = rows * cols;
			*numBytes = *numFloats * sizeof(float);
			lastErr = "";
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return true;
	}

	bool DLEXPORT_SDBR SDBR_getFloatTable(SDBR_HANDLE handle, float* p, uint64_t maxsize)
	{
		using namespace scatdb;
		bool res = false;
		try {
			const scatdb_base* hp = (const scatdb_base*)(handle);
			const db* h = dynamic_cast<const db*>(hp);
			uint64_t rows = (uint64_t)(h)->floatMat.rows();
			uint64_t cols = (uint64_t)(h)->floatMat.cols();
			uint64_t numFloats = rows * cols;
			uint64_t numBytes = numFloats * sizeof(float);
			memcpy(p, h->floatMat.data(), (maxsize < numBytes) ? maxsize : numBytes);
			if (maxsize < numBytes) {
				lastErr = "Destination array is too small.";
			} else {
				lastErr = "";
				res = true;
			}
		}
		catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return res;
	}

	bool DLEXPORT_SDBR SDBR_getIntTable(SDBR_HANDLE handle, uint64_t* p, uint64_t maxsize)
	{
		using namespace scatdb;
		bool res = false;
		try {
			const scatdb_base* hp = (const scatdb_base*)(handle);
			const db* h = dynamic_cast<const db*>(hp);
			uint64_t rows = (uint64_t)(h)->intMat.rows();
			uint64_t cols = (uint64_t)(h)->intMat.cols();
			uint64_t numInts = rows * cols;
			uint64_t numBytes = numInts * sizeof(float);
			memcpy(p, h->intMat.data(), (maxsize < numBytes) ? maxsize : numBytes);
			if (maxsize < numBytes) {
				lastErr = "Destination array is too small.";
			}
			else {
				lastErr = "";
				res = true;
			}
		}
		catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return res;
	}

	bool DLEXPORT_SDBR SDBR_getStats(SDBR_HANDLE handle, float *p, uint64_t maxsize, uint64_t *count)
	{
		using namespace scatdb;
		bool res = false;
		try {
			const scatdb_base* hp = (const scatdb_base*)(handle);
			const db* h = dynamic_cast<const db*>(hp);
			auto stats = h->getStats();
			*count = stats->count;

			uint64_t rows = (uint64_t)(stats)->floatStats.rows();
			uint64_t cols = (uint64_t)(stats)->floatStats.cols();
			uint64_t numFloats = rows * cols;
			uint64_t numBytes = numFloats * sizeof(float);
			memcpy(p, (stats)->floatStats.data(), (maxsize < numBytes) ? maxsize : numBytes);
			if (maxsize < numBytes) {
				lastErr = "Destination array is too small.";
			}
			else {
				lastErr = "";
				res = true;
			}
		}
		catch (std::bad_cast &) {
			lastErr = "Passed handle in SDBR_writeDB is not a database handle.";
			return false;
		}
		catch (std::exception &e) {
			lastErr = std::string(e.what());
			return false;
		}
		return res;
	}

}


