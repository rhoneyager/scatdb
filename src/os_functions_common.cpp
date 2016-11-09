#include <boost/shared_array.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <mutex>
#include <ctime>
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"
#include "../scatdb/splitSet.hpp"
#include "../private/info.hpp"
#include "../private/os_functions_common.hpp"
#include "../private/versioning.hpp"

#ifdef _WIN32
#include "../private/os_functions_win.hpp"
#endif
#ifdef __unix__
#include <dlfcn.h>
#include <link.h>
#include "../private/os_functions_linux.hpp"
#endif

namespace scatdb {
	namespace debug {
		namespace vars {
			size_t sys_num_threads = 0;
			std::mutex m_sys_num_threads, m_sys_names;
			std::string hostname, username, homeDir, appConfigDir, moduleCallbackBuffer;
			bool _consoleTerminated = false;
			// First element is name, second is path. Gets locked with m_sys_names.
			std::vector<std::pair<std::string, std::string> > loadedModulesList;
			/// Private flag that determines if the app waits for the user to press 'Enter' to terminate it at the end of execution.
			bool doWaitOnExit = false;

			currentAppInfo_p _currentAppInfo;
		}
		using namespace ::scatdb::debug::vars;
		

		/// Checks whether a process with the given pid exists.
		bool pidExists(int pid) {
#ifdef _WIN32
			return win::pidExists(pid);
#endif
#ifdef __unix__
			return linux::pidExists(pid);
#endif
			return false;
		}

		/**
		* \brief Entry function that gets called when a debugged application first loads
		*
		* This function gets called at the beginning of an application's execution
		* (generally). It:
		* - determines if the app should wait on exit (to keep the console open)
		* - resets the console title in case any other library overrides it.
		*   A good example of this is the CERN ROOT image lobraries.
		* - Overrides the console control key handlers on Windows. This lets a user
		*   exit with CTRL-C without the debug code causing the app to crash.
		*/
		void appEntry() {
			doWaitOnExit = waitOnExit();

			// Set appexit
			atexit(appExit);
#ifdef _WIN32
			win::appEntry();
#endif
#ifdef __unix__
#endif
		}

		/// Allows the linked app to force / prohibit waiting on exit
		void waitOnExit(bool val)
		{
			doWaitOnExit = val;
		}

		/// Will this app execution result in waiting on exit?
		bool waitOnExit()
		{
			static bool check = true;
			if (check) waitOnExit(waitOnExitForce());
			return doWaitOnExit;
		}

		/// Checks parent PID and name to see if the app should wait on exit.
		bool waitOnExitForce()
		{
#ifdef _WIN32
			return win::waitOnExitForce();
#endif
#ifdef __unix__
			return linux::waitOnExitForce();
#endif
		}

		/// Returns the PID of the running process
		int getPID()
		{
#ifdef _WIN32
			return win::getPID();
#endif
#ifdef __unix__
			return linux::getPID();
#endif
		}

		/// Returns the PID of the parent of a given process
		int getPPID(int pid)
		{
#ifdef _WIN32
			return win::getPPID(pid);
#endif
#ifdef __unix__
			return linux::getPPID(pid);
#endif
		}
		
		/// \todo Finish implementation using Windows and Linux system calls.
		size_t getConcurrentThreadsSupported()
		{
			std::lock_guard<std::mutex> lock(m_sys_num_threads);
			if (sys_num_threads) return sys_num_threads;
			sys_num_threads = static_cast<size_t> (std::thread::hardware_concurrency());
			if (!sys_num_threads) return 4;
			return sys_num_threads;
		}

		/**
		* \brief Function called on application exit to hold the console window open
		*
		* This function is the completion of the appEntry() code.
		* If the window is already closed (such as by the user clicking the X or
		* pressing CTRL-C), then it silently falls through.
		* Otherwise, it examines the doWaitOnExit flag.
		* If the application is spawned in its own window (parent is not cmd.exe),
		* then it prompts the user to press the return key.
		*
		* Also tries if possible to perform final logging of hook table.
		*/
		void appExit()
		{
			using namespace std;
			if (_consoleTerminated) return;
			if (doWaitOnExit)
			{
				cerr << endl << "Program terminated. Press return to exit." << endl;
				//std::getchar();
				// Ignore to the end of file
#ifdef max
#undef max
#endif
				cin.clear();
				//cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::string ln;
				std::getline(cin, ln);
			}
		}

		/**
		* \brief Prints compiler and library information that was present when the
		* scatdb library was compiled.
		*/
		void printDebugInfo(std::ostream &out)
		{
			using std::cerr;
			using std::string;
			using std::endl;
			using boost::filesystem::path;
			versioning::debug_preamble(out);
			getCurrentAppInfo();
#ifdef _WIN32
			string currentPath = win::GetModulePath();
#endif
#ifdef __unix__
			string currentPath = linux::GetModulePath();
#endif
			
			out << "Active location: " << currentPath << endl;
			out << "Loaded modules: \n";
			enumModules(getPID(), out);
			out << "Username: " << _currentAppInfo->username << endl
				<< "Machine name: " << _currentAppInfo->hostname << endl;
		}

		/**
		* \brief Provides information about the given process.
		*
		* Reads in process information from /proc or by querying the os.
		* Returns a structure containing the process:
		* - PID
		* - PPID
		* - Executable name
		* - Executable path
		* - Current working directory
		* - Environment
		* - Command-line
		* - Process start time
		*
		* \throws std::exception if the process does not exist
		**/
		processInfo_p getProcessInfo(int pid) {
#ifdef _WIN32
			return win::getInfo(pid);
#endif
#ifdef __unix__
			return linux::getInfo(pid);
#endif
		}

		currentAppInfo_p getCurrentAppInfo() {
			if (_currentAppInfo) return _currentAppInfo;
			std::shared_ptr<currentAppInfo> res(new currentAppInfo);
#ifdef _WIN32
			res->appConfigDir = win::getAppConfigDir();
			res->homeDir = win::getHomeDir();
			res->hostname = win::getHostname();
			res->username = win::getUsername();
#endif
#ifdef __unix__
			res->appConfigDir = linux::getAppConfigDir();
			res->homeDir = linux::getHomeDir();
			res->hostname = linux::getHostname();
			res->username = linux::getUsername();
#endif
			res->pInfo = getProcessInfo(getPID());
			std::shared_ptr<versioning::versionInfo> vl(new versioning::versionInfo);
			versioning::getLibVersionInfo(*(vl.get()));
			res->vInfoLib = vl;
			_currentAppInfo = res;
			return _currentAppInfo;
		}

		moduleInfo_p getModuleInfo(void* func) {
#ifdef _WIN32
			return win::getModuleInfo(func);
#endif
#ifdef __unix__
			return linux::getModuleInfo(func);
#endif
		}

		/// Enumerate the modules in a given process.
		void enumModules(int pid, std::ostream &out) {
#ifdef _WIN32
			win::enumModules(pid, out);
#endif
#ifdef __unix__
			linux::enumModules(pid, out);
#endif
		}

	}
}


/// Provides functionality to write processInfo in a stream.
std::ostream & operator<<(std::ostream &stream, const scatdb::debug::processInfo &obj)
{
	using std::endl;
	stream << "Process Info for " << obj.name << endl;
	stream << "\tName:\t" << obj.name << endl;
	stream << "\tPID:\t" << obj.pid << endl;
	stream << "\tPPID:\t" << obj.ppid << endl;
	stream << "\tPath:\t" << obj.path << endl;
	stream << "\tCWD:\t" << obj.cwd << endl;
	stream << "\tCmd Line:\t" << obj.cmdline << endl;
	stream << "\tStart:\t" << obj.startTime << endl;
	// stream << "\tEnviron:\n";
	// TODO: parse and write the environment

	//stream << endl;

	return stream;
}

