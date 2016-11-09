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
		int cplen = maxlen;
		if (s.size() + 1 < maxlen) cplen = (int) s.size();
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

	/*
	bool SDBR_writeDBHDF5(SDBR_HANDLE handle, const char* outfile) {
		using namespace scatdb;
		try {
			const scatdb_base* hp = ( const scatdb_base* )(handle);
			const db* h = dynamic_cast<const db*>(hp);
			(h)->writeHDFfile(outfile);
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
	*/

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
}


