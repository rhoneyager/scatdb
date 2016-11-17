cmake_minimum_required (VERSION 2.8)
project(scatdb C CXX)
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/CMakeRules"
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

include(addBaseProject)
include(addBoostUniform)
include(adddocs)
include(addapp)
include(addlib)
include(generaterc)
include(GetGitRevisionDescription)
include(addInstallDirs)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include(addapp)
include(addlib)
set(MAJOR 2)
set(MINOR 0)
set(REVISION 0)
set(SCATDB_VERSION
	${MAJOR}.${MINOR}.${REVISION} )
addBaseProject()
addInstallDirs(scatdb)

addBoostUniform(filesystem program_options system iostreams date_time )

include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
link_directories(${Boost_LIBRARY_DIR_DEBUG})
link_directories(${Boost_LIBRARY_DIR_RELEASE})

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
find_package(HDF5 COMPONENTS C CXX REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR_CPP})
add_definitions(${HDF5_DEFINITIONS})

set(HDF5_USE_STATIC_LIBRARIES OFF)
find_package(HDF5 COMPONENTS C CXX REQUIRED)
include_directories(${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIR_CPP})
add_definitions(${HDF5_DEFINITIONS})
if(DEFINED HDF5_C_LIBRARIES)
	set (h5libs ${HDF5_C_LIBRARIES} ${HDF5_CXX_LIBRARIES})
else()
	set (h5libs hdf5-shared hdf5_hl-shared hdf5_cpp-shared hdf5_hl_cpp-shared)
endif()
if(NOT ZLIB_INCLUDE_DIR)
    if (DEFINED ENV{ZLIB_ROOT})
        set(ZLIB_ROOT $ENV{ZLIB_ROOT})
    ENDIF()
endif()
find_package(ZLIB)


if (ZLIB_FOUND)
    set(COMPRESS_ZLIB 1)
    set(COMPRESS_GZIP 1)
    include_directories(${ZLIB_INCLUDE_DIRS})
    #set(liblist ${liblist} ${ZLIB_LIBRARIES})
endif()
set(COMPRESS_SZIP 0)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeBase/cmake-settings.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake-settings.h"
    )
generaterc("scatdb" "dll"
	"scatdb"
	"Particle scattering database of Ryan Honeyager, Guosheng Liu and Holly Nowell"
	 "${CMAKE_CURRENT_BINARY_DIR}/lib.rc"
	 )
configure_file (
        "${CMAKE_CURRENT_SOURCE_DIR}/CMakeBase/module-scatdb.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Modules/scatdb"
        )

set (REL_BIN_DIR bin)
set (REL_LIB_DIR lib)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

SET(scatdb_main
	src/debug.cpp
	scatdb/debug.hpp
	src/error.cpp
	scatdb/error.hpp
	src/errorTypes.cpp
	scatdb/errorTypes.hpp
	src/export-hdf5.cpp
	scatdb/export-hdf5.hpp
	src/hash.cpp
	private/hash.hpp
	private/hashForwards.hpp
	private/info.hpp
	private/info_fwd.hpp
	src/logging.cpp
	scatdb/logging.hpp
	src/MurmurHash3.cpp
	private/MurmurHash3.h
	src/options.cpp
	private/options.hpp
	scatdb/optionsForwards.hpp
	src/os_functions_common.cpp
	private/os_functions_common.hpp
	src/os_functions_linux.cpp
	private/os_functions_linux.hpp
	src/os_functions_win.cpp
	private/os_functions_win.hpp
	src/splitSet.cpp
	scatdb/splitSet.hpp
	src/versioning.cpp
	private/versioning.hpp
	private/versioningForwards.hpp
	scatdb/defs.h
	scatdb/defs.hpp
	scatdb/lowess.hpp
	scatdb/scatdb.h
	scatdb/scatdb.hpp
	scatdb/scatdb_liu.h
	src/scatdb.cpp
	src/scatdb_c.cpp
	src/scatdb_stats.cpp
	src/filters.cpp
	src/lowess.cpp
	src/io.cpp
	src/io_hdf5.cpp
	src/io_simple.cpp
	src/scatdb_liu.c
	src/scatdb_liu_supplemental.cpp
	)
source_group("Main" FILES ${scatdb_main})
set( scatdb_srcs
	${scatdb_main}
	"${CMAKE_CURRENT_BINARY_DIR}/Modules/scatdb"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake-settings.h"
   )
IF (WIN32 AND NOT CYGWIN)
SET(scatdb_srcs ${scatdb_srcs} 
	"${CMAKE_CURRENT_BINARY_DIR}/lib.rc")
ENDIF()
add_library (scatdb SHARED
	${scatdb_srcs}
	)
if(WIN32 AND NOT CYGWIN)
	set(libs ${h5libs})
else()
	set (libs ${h5libs} ${Boost_FILESYSTEM_LIBRARY} ${Boost_IOSTREAMS_LIBRARY}
		${Boost_PROGRAM_OPTIONS_LIBRARY}
		${Boost_SYSTEM_LIBRARY})
endif()
target_link_libraries(scatdb ${libs})
addlib(scatdb SHARED)
storebin(scatdb "Libraries")

option (BUILD_APPS "Build applications" ON)
IF (BUILD_APPS)
	option (BUILD_EXAMPLE_CPP "Build C++ app example" ON)
	option (BUILD_EXAMPLE_C "Build C app example" ON)
	option (BUILD_ENVELOPES "Build envelopes app" ON)

	if (BUILD_EXAMPLE_CPP)
add_executable (scatdb_example_cpp
	apps/cpp/test.cpp
	)
target_link_libraries(scatdb_example_cpp ${libs} scatdb)
addapp(scatdb_example_cpp "Examples/scatdb")

endif()

if (BUILD_EXAMPLE_C)
add_executable (scatdb_example_c
	apps/c/test-c.c
	)
target_link_libraries(scatdb_example_c ${libs} scatdb)
addapp(scatdb_example_c "Examples/scatdb")
ENDIF()
#add_executable (scatdb_example_fortran
#	apps/fortran/test-f.f90
#	)
#set_target_properties( scatdb_example_fortran PROPERTIES LINKER_LANGUAGE Fortran)
#target_link_libraries(scatdb_example_fortran ${libs} scatdb)
#addapp(scatdb_example_fortran "Examples/scatdb")

if (BUILD_ENVELOPES)
add_executable (scatdb_envelopes apps/envelopes/envelopes.cpp)
target_link_libraries(scatdb_envelopes ${libs} scatdb)
addapp(scatdb_envelopes "Examples/scatdb")

ENDIF()

ENDIF(BUILD_APPS)

INSTALL(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/share/scatdb.csv
	${CMAKE_CURRENT_SOURCE_DIR}/share/scatdb.hdf5
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/scatdb
	COMPONENT Data
	)
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/Modules/scatdb"
	DESTINATION ${CMAKE_INSTALL_PREFIX}/share/scatdb/Modules
	COMPONENT Data
	)
INSTALL(DIRECTORY scatdb 
	DESTINATION ${INSTALL_INCLUDE_DIR}
	COMPONENT Headers
	)

