#ifdef __linux__
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


#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <errno.h>
#include "../private/os_functions_linux.hpp"

namespace scatdb{
	namespace debug {

		namespace linux {
			namespace vars {
				
			}
			
			using namespace ::scatdb::debug::vars;
			bool pidExists(int pid)
			{
				// Need to check existence of directory /proc/pid
				std::ostringstream pname;
				pname << "/proc/" << pid;
				using namespace boost::filesystem;
				path p(pname.str().c_str());
				if (exists(p)) return true;
				return false;
			}
			bool waitOnExitForce() { return false; }

			int getPID()
			{
				//pid_t getpid(void);
				return (int)getpid();
			}

			int getPPID(int pid)
			{
				// pid_t getppid(void);
				return (int)getppid();
			}

			std::string GetModulePath(void *addr)
			{
				std::string out;
				Dl_info info;
				void *addrb = addr;
				if (!addrb) addrb = (void*)GetModulePath;
				if (dladdr(addrb, &info))
				{
					out = std::string(info.dli_fname);
				}
				return out;
			}

			/// \note Keeping function definition this way to preserve compatibility with gcc 4.7
			int moduleCallback(dl_phdr_info *info, size_t sz, void* data)
			{
				//std::ostream &out = std::cerr;
				std::ostringstream out;
				std::string name(info->dlpi_name);
				if (!name.size()) return 0;
				out << "\t" << info->dlpi_name << " (" << info->dlpi_phnum
					<< " segments)" << std::endl;
				/*for (int j=0; j< info->dlpi_phnum; ++j)
				{
				out << "\t\theader " << j << ": address="
				<< (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr)
				<< "\n";
				}
				*/
				moduleCallbackBuffer.append(out.str());
				return 0;
			}

			const std::string& getUsername()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (username.size()) return username;

				auto hInfo = scatdb::debug::getProcessInfo(getPID());

				if (hInfo->expandedEnviron.count("LOGNAME")) {
					username = hInfo->expandedEnviron.at("LOGNAME");
					SDBR_log("os_functions", scatdb::logging::NOTIFICATION,
						"getUsername succeeded by reading LOGNAME.");
				} else { 
					username = "UNKNOWN";
					SDBR_log("os_functions", scatdb::logging::ERROR,
						"getUsername failed to determine the username."
						);
				}


				return username;
			}

			const std::string& getHostname()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (hostname.size()) return hostname;

				const size_t len = 256;
				char hname[len];
				int res = 0;
				res = gethostname(hname, len);
				if (hname[0] != '\0')
					hostname = std::string(hname);

				return hostname;
			}

			const std::string& getAppConfigDir()
			{
				getHomeDir(); // Call early to avoid mutex lock

				std::lock_guard<std::mutex> lock(m_sys_names);
				if (appConfigDir.size()) return appConfigDir;

				auto hInfo = scatdb::debug::getProcessInfo(getPID());

				if (hInfo->expandedEnviron.count("XDG_CONFIG_HOME")) {
					appConfigDir = hInfo->expandedEnviron.at("XDG_CONFIG_HOME");
					return appConfigDir;
				} 
				if (homeDir.size()) {
					appConfigDir = homeDir;
					appConfigDir.append("/.config");
				}
				return appConfigDir;
			}

			const std::string& getHomeDir()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (homeDir.size()) return homeDir;

				// First, test the HOME environment variable. If not set, 
				// then query the passwd database.
				auto hInfo = scatdb::debug::getProcessInfo(getPID());
				if (hInfo->expandedEnviron.count("HOME")) {
					homeDir = hInfo->expandedEnviron.at("HOME");
				}

				if (!homeDir.size())
				{
					struct passwd pw, *pwp;
					const size_t buflen = 4096;
					char buf[buflen];
					int res = getpwuid_r(getuid(), &pw, buf, buflen, &pwp);
					if (res == 0) {
						const char *homedir = pw.pw_dir;
						homeDir = std::string(homedir);
					}

				}

				return homeDir;
			}

			processInfo_p getInfo(int pid)
			{
				std::shared_ptr<processInfo> res(new processInfo);
				res->pid = pid;
				if (!pidExists(pid))
					SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<int>("pid", pid)
					.add<std::string>("Description", "PID does not exist.");
				res->ppid = getPPID(pid);
				std::string cmdline;
				std::string environment;
				{
					using namespace boost::filesystem;
					using namespace std;
					ostringstream procpath;
					procpath << "/proc/" << pid;
					string sp = procpath.str();
					// exe and cwd are symlinks. Read them.
					// path = exe. executable name is the file name from the path.
					path pp(sp);
					path pexe = pp / "exe";
					path pcwd = pp / "cwd";

					path psexe = read_symlink(pexe);
					path pscwd = read_symlink(pcwd);
					path pexename = psexe.filename();

					res->name = pexename.string();
					res->path = psexe.string();
					res->cwd = pscwd.string();

					// environ and cmdline can be read as files
					// both internally use null-terminated strings
					// TODO: figure out what to do with this.
					path pcmd(pp / "cmdline");
					std::ifstream scmdline(pcmd.string().c_str());
					const int length = 1024;
					char *buffer = new char[length];
					while (scmdline.good())
					{
						scmdline.read(buffer, length);
						cmdline.append(buffer, scmdline.gcount());
						res->argv_v.push_back(std::string(buffer));
					}
					//scmdline >> res->cmdline;
					// Replace command-line null symbols with spaces
					//std::replace(res->cmdline.begin(),res->cmdline.end(),
					//		'\0', ' ');

					path penv(pp / "environ");
					std::ifstream senviron(penv.string().c_str());

					while (senviron.good())
					{
						senviron.read(buffer, length);
						environment.append(buffer, senviron.gcount());
					}
					// Replace environment null symbols with newlines
					//std::replace(res->environment.begin(),res->environment.end(),
					//		'\0', '\n');
					delete[] buffer;

					// start time is the timestamp of the /proc/pid folder.
					//std::time_t st = last_write_time(pp);
					//string ct(ctime(&st));
					//res->startTime = ct;

				}
				moduleInfo_p mdll = getModuleInfo((void*)(getInfo));
				res->libpath = mdll->path;

				scatdb::splitSet::splitNullMap(environment, res->expandedEnviron);
				scatdb::splitSet::splitNullVector(cmdline, res->expandedCmd);
				return res;
			}

			moduleInfo_p getModuleInfo(void* func)
			{

				std::string modpath;
				modpath = GetModulePath(func);
				std::shared_ptr<moduleInfo> h(new moduleInfo);
				h->path = modpath;
				return h;
			}

			/// Enumerate the modules in a given process.
			void enumModules(int pid, std::ostream &out)
			{
				// Depends on dladdr existence. Found in gcc, clang, intel.
				// Also depends on dl_iterate_phdr. IGNORES PID!
				/*
				dl_iterate_phdr(std::bind(moduleCallback,
				std::placeholders::_1,
				std::placeholders::_2,
				std::placeholders::_3,
				out)
				, NULL);
				*/
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (pid != getPID()) return;
				if (!moduleCallbackBuffer.size()) {
					dl_iterate_phdr(moduleCallback, NULL);
				}
				out << moduleCallbackBuffer;
			}

		}
	}
}
#endif
