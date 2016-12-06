#ifdef _WIN32
#define _BIND_TO_CURRENT_VCLIBS_VERSION 1
#include <boost/shared_array.hpp>
#include <boost/filesystem.hpp>
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
#include "../scatdb/splitSet.hpp"
#include "../private/os_functions_win.hpp"
#include <Windows.h>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "../scatdb/logging.hpp"
#include "../private/info.hpp"
#pragma comment(lib, "Psapi")
#pragma comment(lib, "Ws2_32")
#pragma comment(lib, "Advapi32")
#pragma comment(lib, "Shell32")

//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>

namespace scatdb {
	namespace debug {

		namespace win {
			namespace vars {
				HIDDEN_SDBR BOOL WINAPI _CloseHandlerRoutine(DWORD dwCtrlType); // Helps gracefully close console
			}

			using namespace ::scatdb::debug::vars;
			/*
			std::string convertStr(const LPWSTR instr)
			{
				size_t origsize = wcslen(instr) + 1;

				const size_t newsize = origsize * 4;
				size_t convertedChars = 0;

				boost::shared_array<char> nstring(new char[newsize]);
				//char nstring[newsize];
				wcstombs_s(&convertedChars, nstring.get(), origsize, instr, _TRUNCATE);
				// Destination string was always null-terminated!
				std::string res(nstring.get());

				return std::move(res);
			}
			*/
			std::string convertStr(const LPTSTR instr)
			{

#ifdef UNICODE
				size_t origsize = wcslen(instr) + 1;

				const size_t newsize = origsize * 4;
				size_t convertedChars = 0;

				boost::shared_array<char> nstring(new char[newsize]);
				//char nstring[newsize];
				wcstombs_s(&convertedChars, nstring.get(), origsize, instr, _TRUNCATE);
				// Destination string was always null-terminated!
				std::string res(nstring.get());
#else
				std::string res(instr);
#endif
				return std::move(res);
			}

			std::string convertStr(const PWSTR instr)
			{
				size_t origsize = wcslen(instr) + 1;

				const size_t newsize = origsize * 4;
				size_t convertedChars = 0;
				boost::shared_array<char> nstring(new char[newsize]);
				//char nstring[newsize];
				wcstombs_s(&convertedChars, nstring.get(), origsize, instr, _TRUNCATE);
				// Destination string was always null-terminated!
				std::string res(nstring.get());

				return std::move(res);
			}

			bool getPathWIN32(DWORD pid, boost::filesystem::path &modPath, boost::filesystem::path &filename)
			{
				HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (NULL == h) return false;
				CloseHandle(h);
				// Get parent process name
				h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION
					//| PROCESS_VM_READ
					, FALSE, pid);
				if (NULL == h) return false;
				TCHAR szModName[MAX_PATH];
				DWORD success = 0;
				DWORD sz = sizeof(szModName) / sizeof(TCHAR);
				success = QueryFullProcessImageName(h, 0, szModName, &sz);
				//success = GetModuleFileNameEx(h,NULL,szModName,sizeof(szModName) / sizeof(TCHAR));

				std::string strModName = convertStr(szModName); // See previous function
				boost::filesystem::path modPathm(strModName);

				boost::filesystem::path filenamem = modPathm.filename();

				modPath = modPathm;
				filename = filenamem;

				CloseHandle(h);
				if (!success)
				{
					success = GetLastError();
					SDBR_log("os_functions", ::scatdb::logging::ERROR,
						"Failure in getPathWIN32: " << success);
					return false;
				}
				return true;
			}

			bool IsAppRunningAsAdminMode()
			{
				BOOL fIsRunAsAdmin = FALSE;
				DWORD dwError = ERROR_SUCCESS;
				PSID pAdministratorsGroup = NULL;

				// Allocate and initialize a SID of the administrators group.
				SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
				if (!AllocateAndInitializeSid(
					&NtAuthority,
					2,
					SECURITY_BUILTIN_DOMAIN_RID,
					DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0,
					&pAdministratorsGroup))
				{
					dwError = GetLastError();
					goto Cleanup;
				}

				// Determine whether the SID of administrators group is enabled in 
				// the primary access token of the process.
				if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
				{
					dwError = GetLastError();
					goto Cleanup;
				}

			Cleanup:
				// Centralized cleanup for all allocated resources.
				if (pAdministratorsGroup)
				{
					FreeSid(pAdministratorsGroup);
					pAdministratorsGroup = NULL;
				}

				// Throw the error if something failed in the function.
				if (ERROR_SUCCESS != dwError)
				{
					SDBR_throw(::scatdb::error::error_types::xOtherError)
						.add<std::string>("Description", "Something failed")
						.add<DWORD>("win-error-code", dwError);
				}

				if (fIsRunAsAdmin) return true;
				return false;
			}

			/// Windows handler for window close events.
			BOOL WINAPI _CloseHandlerRoutine(DWORD dwCtrlType)
			{
				/*
				if (dwCtrlType == CTRL_CLOSE_EVENT)
				{
				_consoleTerminated = true;
				//return true;
				}
				*/
				_consoleTerminated = true;
				return false;
			}

			bool pidExists(int pid)
			{
				// Function needed because Qt is insufficient, and Windows / Unix have 
				// different methods of ascertaining this.
				HANDLE h;
				h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
				if (h)
				{
					DWORD code = 0;
					if (GetExitCodeProcess(h, &code))
					{
						CloseHandle(h);
						if (code == STILL_ACTIVE)
						{
							return true;
						}
						else {
							return false;
						}
					}
					else {
						CloseHandle(h);
						return false;
					}
					CloseHandle(h);
					return true;
				}
				else {
					return false;
				}
				// Execution should not reach this point
			}

			void appEntry()
			{
				// Get PID
				DWORD pid = 0;
				HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (NULL == h) return;
				PROCESSENTRY32 pe = { 0 };
				pe.dwSize = sizeof(PROCESSENTRY32);
				pid = GetCurrentProcessId();
				CloseHandle(h);

				// Get parent process name
				h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION
					//| PROCESS_VM_READ
					, FALSE, pid);
				if (NULL == h) return;
				TCHAR szModName[600];
				DWORD success = 0;
				DWORD sz = sizeof(szModName) / sizeof(TCHAR);
				success = QueryFullProcessImageName(h, 0, szModName, &sz);

				// Set Console Title
				SetConsoleTitle(szModName);


				// Also, set the window closing routine
				// This allows for the user to click the X (or press ctrl-c)
				// without causing a fault.
				// The fault is because the window closes before the atexit 
				// functions can write output.
				SetConsoleCtrlHandler(_CloseHandlerRoutine, true);
				CloseHandle(h);
			}

			bool waitOnExitForce()
			{
				// Get pid and parent pid
				DWORD pid = 0, ppid = 0;
				HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (NULL == h) return false;
				PROCESSENTRY32 pe = { 0 };
				pe.dwSize = sizeof(PROCESSENTRY32);
				pid = GetCurrentProcessId();
				if (Process32First(h, &pe)) {
					do {
						if (pe.th32ProcessID == pid) {
							ppid = pe.th32ParentProcessID;
							//printf("PID: %i; PPID: %i\n", pid, pe.th32ParentProcessID);
						}
					} while (Process32Next(h, &pe));
				}

				//std::cout << "Pid " << pid << "\nPPID " << ppid << std::endl;
				CloseHandle(h);

				// Get parent process name
				boost::filesystem::path filename, filepath;
				getPathWIN32(ppid, filepath, filename);

				//std::cout << filename.string() << std::endl;
				// If run from cmd, no need to wait
				if (filename.string() == "cmd.exe") return false;
				// Cygwin
				if (filename.string() == "bash.exe") return false;
				if (filename.string() == "tcsh.exe") return false;
				// Don't need these due to end return. Just for reference.
				//if (filename.string() == "devenv.exe") return true;
				//if (filename.string() == "explorer.exe") return true;
				return true;
			}

			int getPID()
			{
				DWORD pid = 0;
				/// \note The HANDLE stuff can be removed.
				HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (NULL == h) return false;
				PROCESSENTRY32 pe = { 0 };
				pe.dwSize = sizeof(PROCESSENTRY32);
				pid = GetCurrentProcessId();
				CloseHandle(h);
				return (int)pid;
			}

			int getPPID(int pid)
			{
				DWORD Dpid = pid, ppid = 0;
				HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (NULL == h) return false;
				PROCESSENTRY32 pe = { 0 };
				pe.dwSize = sizeof(PROCESSENTRY32);
				if (!pid) Dpid = GetCurrentProcessId();
				if (Process32First(h, &pe)) {
					do {
						if (pe.th32ProcessID == Dpid) {
							ppid = pe.th32ParentProcessID;
							//printf("PID: %i; PPID: %i\n", pid, pe.th32ParentProcessID);
						}
					} while (Process32Next(h, &pe));
				}

				//std::cout << "Pid " << pid << "\nPPID " << ppid << std::endl;
				CloseHandle(h);
				return (int)ppid;
			}

			/** \brief Get the current module that a Ryan_Debug function is executing from.
			*
			* Used because sxs loading means that multiple copies may be lying around,
			* and we want to figure out who is using which (to indicate what needs to be recompiled).
			*
			* \note Borrowed from http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code
			**/
			HMODULE GetCurrentModule()
			{ // NB: XP+ solution!
				HMODULE hModule = NULL;
				GetModuleHandleEx(
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
					(LPCTSTR)GetCurrentModule,
					&hModule);

				return hModule;
			}

			std::string GetModulePath(HMODULE mod)
			{
				std::string out;
				bool freeAtEnd = false;
				if (!mod)
				{
					mod = GetCurrentModule();
					if (!mod) return std::move(out);
					freeAtEnd = true;
				}
				const DWORD nSize = MAX_PATH * 4;
				TCHAR filename[nSize];
				DWORD sz = GetModuleFileName(mod, filename, nSize);
				out = convertStr(filename);
				if (freeAtEnd)
					FreeLibrary(mod);
				return std::move(out);
			}

			const std::string& getUsername()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (username.size()) return username;

				BOOL res = false;
				const DWORD clen = 256;
				DWORD len = clen;
				TCHAR hname[clen];
				res = GetUserName(hname, &len);
				if (res)
				{
					username = convertStr(hname);
				}
				else {
					DWORD err = GetLastError();
					SDBR_log("os_functions", scatdb::logging::ERROR,
						"getUsername failed with error " << err);
				}
				return username;
			}

			const std::string& getHostname()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (hostname.size()) return hostname;

				BOOL res = false;
				const DWORD clen = MAX_COMPUTERNAME_LENGTH + 1;
				DWORD len = clen;
				TCHAR hname[clen];
				res = GetComputerName(hname, &len);
				if (res)
				{
					hostname = convertStr(hname);
				}
				else {
					DWORD err = GetLastError();
					SDBR_log("os_functions", scatdb::logging::ERROR,
						"getHostname failed with error " << err);
				}
				return hostname;
			}

			const std::string& getAppConfigDir()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (appConfigDir.size()) return appConfigDir;

				HRESULT res = false;
				wchar_t* hname = nullptr;
				res = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &hname);
				if (res == S_OK)
				{
					appConfigDir = convertStr(hname);
				}
				else {
					DWORD err = GetLastError();
					SDBR_log("os_functions", scatdb::logging::ERROR,
						"SHGetFolderPathA failed with error " << err);
				}
				CoTaskMemFree(static_cast<void*>(hname));
				return appConfigDir;
			}

			const std::string& getHomeDir()
			{
				std::lock_guard<std::mutex> lock(m_sys_names);
				if (homeDir.size()) return homeDir;

				HRESULT res = false;
				wchar_t* hname = nullptr;
				res = SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &hname);
				if (res == S_OK)
				{
					homeDir = convertStr(hname);
				}
				else {
					DWORD err = GetLastError();
					SDBR_log("os_functions", scatdb::logging::ERROR,
						"SHGetFolderPathA failed with error " << err);
				}
				CoTaskMemFree(static_cast<void*>(hname));
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

				boost::filesystem::path filename, filepath;
				getPathWIN32((DWORD)pid, filepath, filename); // int always fits in DWORD
				res->name = filename.string();
				res->path = filepath.string();
				moduleInfo_p mdll = getModuleInfo((void*)(getInfo));
				res->libpath = mdll->path;

				//res->startTime;
				std::string environment, cmdline;
				int mypid = getPID();
				if (pid == mypid || IsAppRunningAsAdminMode())
				{
					LPTCH penv = GetEnvironmentStrings();
					LPTCH pend = penv, pprev = '\0';
					while (*pend || *pprev)
					{
						pprev = pend;
						++pend;
					}

					// UNICODE not covered by these functions, I think.....
					// If using unicode and not multibyte...
					// Convert wchar to char in preparation for string and path conversion
					//#ifdef UNICODE
					/*
					size_t origsize = pend - penv + 1;

					const size_t newsize = 3000;
					size_t convertedChars = 0;
					char nstring[newsize];
					wcstombs_s(&convertedChars, nstring, origsize, penv, _TRUNCATE);
					res->environ = std::string(nstring, nstring+newsize);
					*/
					//#else
					environment = std::string(penv, pend);
					//#endif
					FreeEnvironmentStrings(penv);

					cmdline = std::string(GetCommandLine());
					
					int nArgs;
					LPWSTR *szArglist;
					szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
					if (szArglist) {
						for (int i = 0; i < nArgs; ++i) {
							std::string sarg = win::convertStr(szArglist[i]);
							res->argv_v.push_back(sarg);
						}
						LocalFree(szArglist);
					}

					DWORD sz = GetCurrentDirectory(0, NULL);
					LPTSTR cd = new TCHAR[sz];
					DWORD result = GetCurrentDirectory(2500, cd);
					res->cwd = std::string(cd);
					delete[] cd;

				}
				else {
					// Privilege escalation required. Need to handle this case.
					SDBR_log("os_functions", ::scatdb::logging::ERROR,
						"Privilege escalation required to get full process information for another process. UNIMPLEMENTED.");
				}

				// Get parent process name
				HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION
					//| PROCESS_VM_READ
					, FALSE, pid);
				if (NULL == h)
					SDBR_throw(::scatdb::error::error_types::xOtherError)
					.add<std::string>("Description", "Error in getting handle for process!");
				FILETIME pCreation, pExit, pKernel, pUser;
				if (!GetProcessTimes(h, &pCreation, &pExit, &pKernel, &pUser))
					SDBR_throw(::scatdb::error::error_types::xOtherError)
					.add<std::string>("Description", "Error in getting handle for process times!");

				std::ostringstream outCreation;
				SYSTEMTIME pCreationSystem, pCreationLocal;
				if (!FileTimeToSystemTime(&pCreation, &pCreationSystem))
					SDBR_throw(::scatdb::error::error_types::xOtherError)
					.add<std::string>("Description", "Error in converting process times to system times!");
				SystemTimeToTzSpecificLocalTime(NULL, &pCreationSystem, &pCreationLocal);
				outCreation << pCreationLocal.wYear << "-" << pCreationLocal.wMonth << "-" << pCreationLocal.wDay << " "
					<< pCreationLocal.wHour << ":" << pCreationLocal.wMinute << ":" << pCreationLocal.wSecond << "."
					<< pCreationLocal.wMilliseconds;
				//res->startTime = outCreation.str();

				CloseHandle(h);

				scatdb::splitSet::splitNullMap(environment, res->expandedEnviron);
				scatdb::splitSet::splitNullVector(cmdline, res->expandedCmd);
				return res;
			}


			moduleInfo_p getModuleInfo(void* func)
			{
				std::string modpath;

				BOOL success = false;
				if (func)
				{
					// Get path of func
					DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
					LPCTSTR lpModuleName = (LPCTSTR)func;
					HMODULE mod;
					success = GetModuleHandleEx(flags, lpModuleName, &mod);
					if (!success) return nullptr;
					modpath = GetModulePath(mod);
					FreeLibrary(mod);
				}
				else {
					// Get Ryan_Debug dll path
					modpath = GetModulePath();
				}
				std::shared_ptr<moduleInfo> h(new moduleInfo);
				h->path = modpath;
				return h;
			}

			void enumModules(int pid, std::ostream &out)
			{
				HANDLE h = NULL, snapshot = NULL;
				try {

					h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
					if (!h || h == INVALID_HANDLE_VALUE)
						SDBR_throw(::scatdb::error::error_types::xOtherError)
						.add<std::string>("Description", "Error in getting handle for process!");
					snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
					if (!snapshot || snapshot == INVALID_HANDLE_VALUE)
						SDBR_throw(::scatdb::error::error_types::xOtherError)
						.add<std::string>("Description", "Cannot get handle snapshot!");
					std::shared_ptr<MODULEENTRY32> mod(new MODULEENTRY32);
					mod->dwSize = sizeof(MODULEENTRY32); // Annoying requirement
					if (!Module32First(snapshot, mod.get()))
						SDBR_throw(::scatdb::error::error_types::xOtherError)
						.add<std::string>("Description", "Cannot list first module!");
					out << "\tName\tPath\n";
					do {
						//std::string modPath = GetModulePath(mod->hModule);
						std::string modName = convertStr(mod->szModule);
						std::string modPath = convertStr(mod->szExePath);

						out << "\t" << modName << "\t" << modPath << std::endl;
					} while (Module32Next(snapshot, mod.get()));
				}
				catch (const char *err) {
					out << "\t" << err << std::endl;
				}
				if (snapshot && snapshot != INVALID_HANDLE_VALUE) CloseHandle(snapshot);
				if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h);
			}

			void writeDebugStr(const std::string & instr) {
				OutputDebugString(instr.c_str());
			}
		}

	}

}

#endif
