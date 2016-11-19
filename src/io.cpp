#include "../scatdb/defs.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "../scatdb/debug.hpp"
#include "../private/info.hpp"
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"

#include "../scatdb/scatdb.hpp"

namespace {
	const char scatdb_name[] = "scatdb.hdf5";
	const char scatdb_db_env[] = "scatdb_db";
	const char scatdb_dir_env[] = "scatdb_DIR";
	const char scatdb_config_dir[] = "scatdb";

	/// Always points to the first loaded database file.
	std::shared_ptr<const scatdb::db> loadedDB;
	std::mutex m_db;
	bool finddberrgiven = false;
}

namespace scatdb {
	/**
	* \brief Function that returns the location of the scattering database file.
	*
	* Finding the default config file has become a rather involved process.
	* First, check the application execution arguments (if available).
	* Second, check the environment variables. Uses the key "scatdb_db", and accepts
	* multiple files, separated by semicolons. Searches for file existence in
	* left-to-right order.
	*
	* Finally, check using the precompiled paths.
	**/
	bool db::findDB(std::string &filename) {
		if (filename.size()) {
			SDBR_log("scatdb", scatdb::logging::DEBUG_2,
				"Using database file: " << filename);
			return true;
		}

		filename = "";
		using namespace boost::filesystem;

		SDBR_log("scatdb", scatdb::logging::NOTIFICATION, 
			"Finding '" << scatdb_name << "' file");
		// Check application execution arguments
		//BOOST_LOG_SEV(lg, Ryan_Debug::log::debug_2) << "Checking app command line";
		//path testCMD(rtmath::debug::sConfigDefaultFile);
		//if (exists(testCMD))
		//{
		//	filename = rtmath::debug::sConfigDefaultFile;
		//BOOST_LOG_SEV(lg, Ryan_Debug::log::debug_2) << filename;
		//	return true;
		//}

		// Checking environment variables
		using namespace scatdb;
		auto mInfo = debug::getCurrentAppInfo();
		SDBR_log("scatdb", scatdb::logging::DEBUG_2,
			"Checking environment variable '"
			<< std::string(scatdb_db_env) << "'.");
		
		const std::map<std::string, std::string> &mEnv = mInfo->pInfo->expandedEnviron;
		auto findEnv = [&](const std::string &fkey, std::string &outname) -> bool {
			SDBR_log("scatdb", scatdb::logging::DEBUG_2,
				"Parsing list of environment variables to find '" << fkey << "'.");
			std::string flkey = fkey;
			std::transform(flkey.begin(), flkey.end(), flkey.begin(), ::tolower);
			auto it = std::find_if(mEnv.cbegin(), mEnv.cend(),
				[&flkey](const std::pair<std::string, std::string> &pred)
			{
				std::string key = pred.first;
				std::transform(key.begin(), key.end(), key.begin(), ::tolower);
				if (key == flkey) return true;
				return false;
			});
			if (it != mEnv.cend())
			{
				typedef boost::tokenizer<boost::char_separator<char> >
					tokenizer;
				boost::char_separator<char> sep(";");
				SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
					"Candidates are: " << it->second);
				std::string ssubst;
				tokenizer tcom(it->second, sep);
				for (auto ot = tcom.begin(); ot != tcom.end(); ot++)
				{
					path testEnv(it->second);
					if (exists(testEnv))
					{
						outname = it->second;
						SDBR_log("scatdb", scatdb::logging::DEBUG_2,
							"Using " << outname);
						return true;
					}
				}
			} else SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"Environment variable '" << fkey << "' was not found.");
			return false;
		};
		if (findEnv(std::string(scatdb_db_env), filename)) return true;

		// Check a few other places
		SDBR_log("scatdb", scatdb::logging::DEBUG_2,
			"Checking app data directories for '" << scatdb_name << "'.");
		std::string sAppConfigDir = mInfo->appConfigDir;
		std::string sHomeDir = mInfo->homeDir;
		std::string dllPath = debug::getModuleInfo((void*)&(db::findDB))->path;
		
		std::string appPath = mInfo->pInfo->path;

		std::string sCWD = mInfo->pInfo->cwd;
		auto searchPath = [&](const std::string &base, const std::string &suffix, bool searchParent) -> bool
		{
			using namespace boost::filesystem;
			path pBase(base);
			if (base.size() == 0) return false;
			if (!is_directory(pBase))
				pBase.remove_filename();
			if (searchParent) pBase.remove_leaf();
			if (suffix.size()) pBase = pBase / path(suffix);

			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"Searching in: " << pBase.string());
			path p1 = pBase / std::string(scatdb_name);

			bool res = false;
			path pRes;
			res = boost::filesystem::exists(p1);
			if (!res) return false;
			filename = p1.string();
			return true;
		};
		bool found = false; // junk variable

		if (searchPath(sCWD, "", false)) found = true;
		if (searchPath(sCWD, "", true)) found = true;
		else if (searchPath(sAppConfigDir, std::string(scatdb_config_dir), false)) found = true;
		else if (searchPath(sHomeDir, "", false)) found = true;
		else if (searchPath(dllPath, "", true)) found = true;
		else if (searchPath(appPath, "", true)) found = true;

		if (!filename.size()) {
			std::string RDCpath;
			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"Searching based on the '" << scatdb_dir_env << "' environment variable.");

			findEnv(std::string(scatdb_dir_env), RDCpath);
			if (searchPath(RDCpath, "../../../../share", false)) found = true;
		}
		SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
			"Searching based on the path to the dll.");


		if (searchPath(dllPath, "../../share/scatdb", false)) found = true;
		if (searchPath(dllPath, "../../share", false)) found = true;

		if (filename.size()) {
			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"Using database file: " << filename);
			return true;
		}

		if (!finddberrgiven) {
			finddberrgiven = true;
			SDBR_log("scatdb", scatdb::logging::ERROR,
				"Unable to find the '" << scatdb_name << "' database file. Please consult the log to "
				"learn which paths were examined. To view the log on the console, specify "
				"option '--log-level-console-threshold 0' when running this program. Alternatively, to write the "
				"log to a file, use the option '--log-file filename'. "
				"To manually specify the path to '" << scatdb_name << "', set the '"
				<< scatdb_db_env << "' environment variable or specify the file on the "
				"command line (with option '--dbfile filename').");
		}
		
		return false;
	}

	void db::readDBtext(std::shared_ptr<db> res, const char* dbf) {

		using namespace boost::interprocess;
		//const std::size_t FileSize = 10000;
		file_mapping m_file(dbf, read_only);
		mapped_region region(m_file, read_only);
		void * addr = region.get_address();
		char* caddr = (char*)addr;
		std::size_t size = region.get_size();
		const char *left = caddr, *right = caddr + size;
		// Skip header line
		while ((left[0] != '\n') && left < right) left++;
		left++;
		//std::memset(addr, 1, size);
		// Count number of lines
		// std::count_if is slow
		//size_t numLines = (size_t)std::count_if(caddr, caddr + size, [](const char a) {return (a == '\n') ? true : false; });
		size_t numLines = 0;
		for (const char* cleft = left; cleft < right; ++cleft) {
			if (cleft[0] == '\n') numLines++;
		}
		//std::vector<int> ints;
		//std::vector<float> floats;
		const size_t numInts = numLines * data_entries::SDBR_NUM_DATA_ENTRIES_INTS;
		const size_t numFloats = numLines * data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS;
		res->floatMat.resize(numLines, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		res->intMat.resize(numLines, data_entries::SDBR_NUM_DATA_ENTRIES_INTS);
		//ints.resize(numInts);
		//floats.resize(numFloats);
		uint64_t* ints = res->intMat.data();
		float* floats = res->floatMat.data();
		size_t line = 0;
		int col = 0;
		const size_t numDelims = 2;
		const char delims[numDelims] = { ',', '\n' };
		bool lineEnd = true;
		long curInt = 0, curFloat = 0;
		
		while (left < right) {
			// Skip non-digits
			// std::find_first_of is _very_ slow on windows.
			//const char* pEnd = std::find_first_of(delims, delims + numDelims, left, right);
			const char *pEnd = left;
			while ((pEnd[0] != '\n') && (pEnd[0] != ',') && pEnd < right) pEnd++;

			if (pEnd > right) pEnd = right;
			col = (lineEnd) ? 0 : col + 1;
			lineEnd = (pEnd[0] == '\n') ? true : false;
			using boost::spirit::qi::float_;
			using boost::spirit::qi::int_;
			using boost::spirit::qi::parse;
			// The Eigen matrices are stored in column-major format
			//using namespace scatdb::macros;
			switch (col) {
			case 0: // flaketype
				assert(numInts > curInt);
				parse(left, pEnd, int_, ints[curInt]);
				//ints[curInt] = m_atoi<int>(left, pEnd - left);
				++curInt;
				break;
			default: // All other columns
				assert(numFloats > curFloat);
				parse(left, pEnd, float_, floats[curFloat]);
				//floats[curFloat] = m_atof<float>(left, pEnd - left);
				++curFloat;
				break;
			}

			left = pEnd + 1;
		}
		SDBR_log("scatdb", scatdb::logging::DEBUG_2,
			"Overall database has "
			<< numLines << " lines of data that were successfully read.");
	}

	std::shared_ptr<const db> db::loadDB(const char* dbfile, const char* hdfinternalpath) {
		std::lock_guard<std::mutex> lock(m_db);
		if (!dbfile && loadedDB) return loadedDB;

		// Load the database
		SDBR_log("scatdb", scatdb::logging::DEBUG_2,
			"loadDB called. Need to load database.");
		std::string dbf;
		if (dbfile) dbf = std::string(dbfile);
		if (!dbf.size()) {
			findDB(dbf); // If dbfile not provided, take a guess.
			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"dbfile automatically determined as: " << dbf);
		} else {
			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"dbfile provided in call: " << dbf);
		}
		using namespace boost::filesystem;
		path p(dbf);
		if (!dbf.size()) {
			// Scattering database cannot be found.
			SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
				"Scattering database cannot be found.");
			std::cerr << "Scattering database cannot be found." << std::endl;
			SDBR_throw(::scatdb::error::error_types::xMissingFile)
				.add<std::string>("filename", std::string(scatdb_name));
		} else {
			if (!exists(p)) {
				// Supplied path to scattering database does not exist.
				// Scattering database cannot be found.
				SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
					"Scattering database cannot be found at " << dbf);
				std::cerr << "Scattering database cannot be found at " << dbf;
				SDBR_throw(scatdb::error::error_types::xMissingFile)
					.add<std::string>("filename", p.string());
			}
		}

		// From this point, it is established that the scattering database does exist.
		std::shared_ptr<db> newdb(new db);
		if (p.extension().string() == ".csv") readDBtext(newdb, dbf.c_str());
		else if (p.extension().string() == ".hdf5") readDBhdf5(newdb, dbf.c_str());
		else if (p.extension().string() == ".dda") readDBscatdb(newdb, dbf.c_str());
		else SDBR_throw(scatdb::error::error_types::xUnknownFileFormat)
			.add<std::string>("filename", p.string());

		// The usual case is that the database is loaded once. If so, store a copy for
		// subsequent function calls.
		if (!loadedDB) loadedDB = newdb;

		SDBR_log("scatdb", scatdb::logging::DEBUG_2, 
			"Database loaded successfully.");
		return newdb;
	}
}

