#include "../scatdb/defs.hpp"
#ifdef SDBR_OS_UNIX
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
#include <sys/user.h>
#include <sys/sysctl.h>
#include <libutil.h>
#include <libprocstat.h>
#include <pwd.h>
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <errno.h>
#include "../private/os_functions_freebsd.hpp"

namespace scatdb{
	namespace debug {

		namespace bsd {
			
			using namespace ::scatdb::debug::vars;
			bool pidExists(int pid)
			{
				int i, mib[4];
				size_t len;
				struct kinfo_proc kp;

				/* Fill out the first three components of the mib */
				len = 4;
				sysctlnametomib("kern.proc.pid", mib, &len);
				mib[3] = pid;
				len = sizeof(kp);
				int res = sysctl(mib, 4, &kp, &len, NULL, 0);
				if (res == -1) {
					// Either the pid does not exist, or some other error
					int err = errno;
					if (err == ENOENT) return false;
					const int buflen = 200;
					char strerrbuf[buflen] = "\0";
					// strerror_r will always yield a null-terminated string.
					int ebufres = strerror_r(err, strerrbuf, buflen);
					SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<int>("pid", pid)
					.add<std::string>("Description", "When attempting to "
						"determine if a PID exists, an unhandled error occurred.")
					.add<int>("errno", err)
					.add<std::string>("Error_Description", std::string(strerrbuf));
					;
				}
				else if ((res == 0) && (len > 0))
					return true;
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
				struct kinfo_proc *proc = kinfo_getproc(pid);
				if (!proc)
					SDBR_throw(::scatdb::error::error_types::xBadInput)
						.add<int>("pid", pid)
						.add<std::string>("Description", "Cannot get kinfo_proc "
							"structure for this pid, but the process does exist.");
				res->name = std::string(proc->ki_comm);
				// TODO: fix to get absolute path
				// TODO: also fix getUsername
				// TODO: getHomeDir also is wrong (should resolve symlink).
				//	 Affects app config dir also.
				struct procstat * pss = procstat_open_sysctl();
				if (!pss) SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "Cannot open procstat structure.");
				const int nchr = 65535;
				char *errbuf;
				char pathname[nchr] = "\0";

				char** argv = procstat_getargv(pss, proc, 0);
				if (!argv) SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "Cannot get argv.");
				// Should process immediately in case of buffer re-use.
				int i=0;
				while (argv[i]) {
					res->expandedCmd.push_back(std::string(argv[i]));
					++i;
				}
				if (argv) procstat_freeargv(pss);


				char** envv = procstat_getenvv(pss, proc, 0);
				if (!envv) SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "Cannot get envv.");
				i=0;
				while (envv[i]) {
					splitSet::splitNullMap(std::string(envv[i]), res->expandedEnviron);
					//res->expandedEnviron.push_back(std::string(envv[i]));
					++i;
				}
				if (envv) procstat_freeenvv(pss);

				int r3 = procstat_getpathname(pss, proc, pathname, nchr);
				if (r3) SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "procstat_getpathname failed")
					.add<int>("rval", r3)
					.add<std::string>("returned-pathname", std::string(pathname));

				res->path = std::string(pathname);
				char cwd[nchr] = "\0";
				char *cwdres = getcwd(cwd, nchr);
				if (!cwdres) SDBR_throw(::scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "getcwd failed")
					;
				res->cwd = std::string(cwd);
				res->argv_v = res->expandedCmd;
				//res->startTime = "UNIMPLEMENTED";
				moduleInfo_p mdll = getModuleInfo((void*)(getInfo));
				res->libpath = mdll->path;
freeAllocs:
				if (proc) free(proc);
				if (pss) procstat_close(pss);

				{
					/*
					std::ifstream scmdline(pcmd.string().c_str());
					const int length = 1024;
					char *buffer = new char[length];
					while (scmdline.good())
					{
						scmdline.read(buffer, length);
						res->cmdline.append(buffer, scmdline.gcount());
						res->argv_v.push_back(std::string(buffer));
					}

					std::ifstream senviron(penv.string().c_str());

					while (senviron.good())
					{
						senviron.read(buffer, length);
						res->environment.append(buffer, senviron.gcount());
					}
					// Replace environment null symbols with newlines
					//std::replace(res->environment.begin(),res->environment.end(),
					//		'\0', '\n');
					delete[] buffer;

					// start time is the timestamp of the /proc/pid folder.
					std::time_t st = last_write_time(pp);
					string ct(ctime(&st));
					res->startTime = ct;
					*/
				}
				//scatdb::splitSet::splitNullMap(environment, res->expandedEnviron);
				//scatdb::splitSet::splitNullVector(cmdline, res->expandedCmd);
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
