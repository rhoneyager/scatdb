/**
* \brief The debugging file, where all of the error-handling
* and versioning code resides.
**/
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <boost/version.hpp>
#include <boost/program_options.hpp>
#include "../private/os_functions_common.hpp"
#include "../scatdb/debug.hpp"
#include "../scatdb/error.hpp"
#include "../private/info.hpp"
#include "../scatdb/logging.hpp"
#include "../scatdb/splitSet.hpp"
#include "../private/versioningGenerate.hpp"
#include "../scatdb/scatdb.hpp"

namespace {
	boost::program_options::options_description *pcmdline = nullptr;
	boost::program_options::options_description *pconfig = nullptr;
	boost::program_options::options_description *phidden = nullptr;
	size_t sys_num_threads = 0;
	std::mutex m_debug;
}

namespace scatdb
{
	namespace debug
	{
		std::string sConfigDefaultFile;
	
		void add_options(
			boost::program_options::options_description &cmdline,
			boost::program_options::options_description &config,
			boost::program_options::options_description &hidden)
		{
			namespace po = boost::program_options;
			using std::string;

			std::lock_guard<std::mutex> lock(m_debug);
			static bool added = false;
			if (added) return;
			added = true;

			scatdb::debug::appEntry();

			pcmdline = &cmdline;
			pconfig = &config;
			phidden = &hidden;

			/// \todo Add option for default Ryan_Debug.conf location

			cmdline.add_options()
				("version", "Print library version information and exit")
				("help-verbose", "Print out all possible program options")
				("dbfile,d", po::value<string>(), "Manually specify database location")
				;

			config.add_options()
				;
			
			hidden.add_options()
				("scatdb-version", "Print scatdb library version information and exit")
				("help-all", "Print out all possible program options")
				("help-full", "Print out all possible program options")
				("close-on-finish", po::value<bool>(), "Should the app automatically close on termination?")

				("log-level-console-threshold", po::value<int>()->default_value((int)::scatdb::logging::WARNING), "Threshold for console logging")
				//("log-channel", po::value<std::vector<std::string> >()->multitoken(), "Log only the specified channel(s)")
				("log-file", po::value<std::string>(), "Log everything to specified file.")

				("scatdb-config-file", po::value<std::string>(),
				"Specify the location of the scatdb configuration file. Overrides "
				"all other search locations. If it cannot be found, fall back to the "
				"next option.")
				;

			//registry::add_options(cmdline, config, hidden);
		}

		void process_static_options(
			boost::program_options::variables_map &vm)
		{
			namespace po = boost::program_options;
			using std::string;

			std::lock_guard<std::mutex> lock(m_debug);
			static bool done = false;
			if (done) return;
			done = true;

			int lt = vm["log-level-console-threshold"].as<int>();

			std::string lf;
			if (vm.count("log-file")) lf = vm["log-file"].as<std::string>();

			scatdb::logging::log_properties lps;
			lps.debugChannel = false;
			lps.logFile = lf;
			lps.consoleLogThreshold = lt;
			lps.debuggerLogThreshold = logging::INFO;
			scatdb::logging::setupLogging(0,0,&lps);



			if (vm.count("help-verbose") || vm.count("help-all") || vm.count("help-full"))
			{
				po::options_description oall("All Options");
				oall.add(*pcmdline).add(*pconfig).add(*phidden);

				std::cerr << oall << std::endl;
				exit(2);
			}
			
			if (vm.count("scatdb-config-file"))
			{
				debug::sConfigDefaultFile = vm["scatdb-config-file"].as<std::string>();
				SDBR_log("dll", ::scatdb::logging::NORMAL,
					"Console override of Ryan_Debug-config-file: " 
					<< debug::sConfigDefaultFile);
			}

			// Log the command-line arguments and environment variables to the file log.
			auto hp = debug::getCurrentAppInfo();
			std::string appName(hp->pInfo->name);
			std::string appPath(hp->pInfo->path);
			std::string cwd(hp->pInfo->cwd);
			size_t sEnv = 0, sCmd = 0;
			std::map<std::string, std::string> mEnv = hp->pInfo->expandedEnviron;
			std::vector<std::string> mCmd = hp->pInfo->expandedCmd;
			std::string sCmdP;
			for (auto it = mCmd.begin(); it != mCmd.end(); ++it) {
				if (it != mCmd.begin()) sCmdP.append(" ");
				else sCmdP.append("\t");
				sCmdP.append(*it);
			}
			std::string hdir(hp->homeDir);
			SDBR_log("dll", ::scatdb::logging::NORMAL, 
				"Starting application: \n"
				<< "\tName: " << appName
				<< "\n\tApp Path: " << appPath
				<< "\n\tLib Path: " << hp->pInfo->libpath
				<< "\n\tCWD: " << cwd
				<< "\n\tApp config dir: " << hp->appConfigDir
				<< "\n\tUsername: " << hp->username
				<< "\n\tHome dir: " << hp->homeDir
				<< "\n\tHostname: " << hp->hostname
				<< "\n\tCommand-Line Args: " << sCmdP
				);

			//if (vm.count("Ryan_Debug-conf"))
			//{
			//	BOOST_LOG_SEV(lg, notification) << "Loading custom Ryan_Debug.conf from " << vm["Ryan_Debug-conf"].as<string>();
			//	Ryan_Debug::config::loadRtconfRoot(vm["Ryan_Debug-conf"].as<string>());
			//} else { Ryan_Debug::config::loadRtconfRoot(); }
			//scatdb::config::loadRtconfRoot();

			//atexit(appExit);
			//Switch to more complete logging system.
			if (vm.count("close-on-finish")) {
				bool val = !(vm["close-on-finish"].as<bool>());
				debug::waitOnExit(val);
				SDBR_log("dll", ::scatdb::logging::NORMAL,
					"Console override of waiting on exit: " << val);
			}

			SDBR_log("dll", ::scatdb::logging::NORMAL,
				"Switching to primary logging system");
			//setupLogging(vm, sevlev);
			SDBR_log("dll", ::scatdb::logging::NORMAL, "Primary logging system started.");
			
			//registry::process_static_options(vm);

			std::ostringstream preambles;
			preambles << "scatdb library information: \n";
			versioning::versionInfo v;
			versioning::getLibVersionInfo(v);
			versioning::debug_preamble(v, preambles);

			std::string spreambles = preambles.str();

			if (vm.count("scatdb-version"))
			{
				std::cerr << spreambles
					<< "Loaded modules:\n";
				enumModules(getPID(), std::cerr);

				exit(2);
			}

			if (vm.count("version"))
			{
				std::cerr << spreambles;
			}

			std::string dbfile;
			if (vm.count("dbfile")) dbfile = vm["dbfile"].as<string>();
			db::findDB(dbfile);
			db::loadDB(dbfile.c_str());

			SDBR_log("dll", ::scatdb::logging::NORMAL, spreambles);
		}


	}
}
