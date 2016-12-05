#ifndef SDBR_DEFS
#define SDBR_DEFS

/* defs.h - Header containing typedefs and other defines for many things */

#ifdef SHARED_EXPORT_SDBR
#undef SHARED_EXPORT_SDBR
#endif
#ifdef SHARED_IMPORT_SDBR
#undef SHARED_IMPORT_SDBR
#endif
#ifdef HIDDEN_SDBR
#undef HIDDEN_SDBR
#endif
#ifdef PRIVATE_SDBR
#undef PRIVATE_SDBR
#endif

/// \todo CMakeLists needs to set the appropriate flags EXPORTING_SDBR on all libraries and 
/// SHARED_SDBR_* for the libraries that are shared.

/// \def EXPORTING_SDBR is set in each library. Absence of flag indicates possible import of shared code.
/// \def SHARED_SDBR is set when building using shared libraries. 

/// \def DLEXPORT_SDBR takes three values. dllexport and dllimport are Windows-only, used with shared libraries.
/// Otherwise, this tag is removed by the preprocessor.

/// \def DEPRECATED_SDBR acts to deprecate a function or a class
/// \def UNIMPLEMENTED acts as a tag indicating that a function is not ready for use
#if defined _MSC_FULL_VER
#pragma warning( disable : 4251 ) // DLL C-interface MSVC warning
#define DEPRECATED_SDBR __declspec(DEPRECATED_SDBR)
#define WARN_UNIMPLEMENTED_SDBR
#define ERR_UNIMPLEMENTED_SDBR __declspec(DEPRECATED_SDBR("Function is unimplemented / is commented out"))
#elif defined __clang__
#define DEPRECATED_SDBR __attribute__ ((DEPRECATED_SDBR))
#define WARN_UNIMPLEMENTED_SDBR __attribute__ ((unavailable("Unimplemented function is used")))
#define ERR_UNIMPLEMENTED_SDBR __attribute__ ((unavailable("Unimplemented function is used")))
#elif defined __GNUC__
#define DEPRECATED_SDBR __attribute__ ((DEPRECATED_SDBR))
#define WARN_UNIMPLEMENTED_SDBR __attribute__ ((warning("Unimplemented function is used")))
#define ERR_UNIMPLEMENTED_SDBR __attribute__ ((error("Unimplemented function is used")))
#else
#define DEPRECATED_SDBR
#define WARN_UNIMPLEMENTED_SDBR
#define ERR_UNIMPLEMENTED_SDBR
#endif


#if defined _MSC_FULL_VER
#define COMPILER_EXPORTS_VERSION_A_SDBR
#elif defined __INTEL_COMPILER
#define COMPILER_EXPORTS_VERSION_B_SDBR
#elif defined __GNUC__
#define COMPILER_EXPORTS_VERSION_B_SDBR
#elif defined __MINGW32__
#define COMPILER_EXPORTS_VERSION_B_SDBR
#elif defined __clang__
#define COMPILER_EXPORTS_VERSION_B_SDBR
#else
#define COMPILER_EXPORTS_VERSION_UNKNOWN
#endif

// Defaults for static libraries
#define SHARED_EXPORT_SDBR
#define SHARED_IMPORT_SDBR
#define HIDDEN_SDBR
#define PRIVATE_SDBR

#if defined COMPILER_EXPORTS_VERSION_A_SDBR
	#undef SHARED_EXPORT_SDBR
	#undef SHARED_IMPORT_SDBR
	#define SHARED_EXPORT_SDBR __declspec(dllexport)
	#define SHARED_IMPORT_SDBR __declspec(dllimport)
#elif defined COMPILER_EXPORTS_VERSION_B_SDBR
	#undef SHARED_EXPORT_SDBR
	#undef SHARED_IMPORT_SDBR
	#undef HIDDEN_SDBR
	#undef PRIVATE_SDBR
	#define SHARED_EXPORT_SDBR __attribute__ ((visibility("default")))
	#define SHARED_IMPORT_SDBR __attribute__ ((visibility("default")))
	#define HIDDEN_SDBR __attribute__ ((visibility("hidden")))
	#define PRIVATE_SDBR __attribute__ ((visibility("internal")))
#else
	#pragma message("defs.h warning: compiler is unrecognized")
#endif

// If SHARED_(libname) is defined, then the target library both 
// exprts and imports. If not defined, then it is a static library.

// Macros defined as EXPORTING_(libname) are internal to the library.
// They indicate that SHARED_EXPORT_SDBR should be used.
// If EXPORTING_ is not defined, then SHARED_IMPORT_SDBR should be used.

#if SHARED_scatdb
	#if EXPORTING_scatdb
		#define DLEXPORT_SDBR SHARED_EXPORT_SDBR
	#else
		#define DLEXPORT_SDBR SHARED_IMPORT_SDBR
	#endif
#else
	#define DLEXPORT_SDBR
#endif

// Microsoft vs. standard functions...
#if (defined(_MSC_VER) && (_MSC_VER >= 1400) ) // _MSC_VER==MSVC 2005 
#else
#define strcpy_s(dest, count, source)    strncpy( (dest), (source), (count) )
//strncpy_s(out.vdate, versionInfo::charmax, __DATE__, versionInfo::charmax);
#define strncpy_s(dest, count, source, junk)    strncpy( (dest), (source), (count) )
//strnlen_s(in.vdate, versionInfo::charmax)
#define strnlen_s(source, count) strnlen((source), (count))
#define sprintf_s snprintf
#define memcpy_s(dest, count, source, junk)    memcpy( (dest), (source), (count) )

#endif

// OS definitions
#ifdef __unix__
#ifdef __linux__
#define SDBR_OS_LINUX
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#define SDBR_OS_UNIX
#endif
#endif
#ifdef _WIN32
#define SDBR_OS_WINDOWS
#endif
#if !defined(_WIN32) && !defined(SDBR_OS_UNIX) && !defined(SDBR_OS_LINUX)
#define SDBR_OS_UNSUPPORTED
#endif


// Common definitions
enum SDBR_write_type {
	SDBR_TRUNCATE,
	SDBR_READWRITE,
	SDBR_CREATE
};

// End the header block
#endif

