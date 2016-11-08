#pragma once
#include "defs.hpp"

#include <string>
#include <iostream>
#include <ostream>
namespace boost {
	namespace program_options { class options_description; class variables_map; }
	namespace filesystem { class path; }
}

namespace scatdb
{
	namespace debug {
		HIDDEN_SDBR bool pidExists(int pid);
		HIDDEN_SDBR void appEntry();
		HIDDEN_SDBR void waitOnExit(bool val);
		HIDDEN_SDBR bool waitOnExit();
		HIDDEN_SDBR bool waitOnExitForce();
		HIDDEN_SDBR int getPID();
		HIDDEN_SDBR int getPPID(int pid);
		HIDDEN_SDBR size_t getConcurrentThreadsSupported();
		HIDDEN_SDBR void appExit();
		HIDDEN_SDBR void printDebugInfo(std::ostream &out);

		HIDDEN_SDBR void enumModules(int pid, std::ostream &out = std::cerr);

		/**
		* \brief Adds options to a program
		*
		* \item cmdline provides options only allowed on the command line
		* \item config provides options available on the command line and in a config file
		* \item hidden provides options allowed anywhere, but are not displayed to the user
		**/
		DLEXPORT_SDBR void add_options(
			boost::program_options::options_description &cmdline,
			boost::program_options::options_description &config,
			boost::program_options::options_description &hidden);
		/// Processes static options defined in add_options
		DLEXPORT_SDBR void process_static_options(
			boost::program_options::variables_map &vm);
	}
}


