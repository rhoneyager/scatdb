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
set(REVISION 1)
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
SET(scatdb_os
	src/os_functions_common.cpp
	private/os_functions_common.hpp
	src/os_functions_linux.cpp
	private/os_functions_linux.hpp
	src/os_functions_win.cpp
	private/os_functions_win.hpp
	)
SET(scatdb_debug
	src/debug.cpp
	scatdb/debug.hpp
	src/error.cpp
	scatdb/error.hpp
	src/errorTypes.cpp
	scatdb/errorTypes.hpp
	src/hash.cpp
	scatdb/hash.hpp
	scatdb/hashForwards.hpp
	private/info.hpp
	private/info_fwd.hpp
	src/logging.cpp
	scatdb/logging.hpp
	src/MurmurHash3.cpp
	private/MurmurHash3.h
	src/options.cpp
	private/options.hpp
	scatdb/optionsForwards.hpp
	src/splitSet.cpp
	scatdb/splitSet.hpp
	src/versioning.cpp
	private/versioning.hpp
	private/versioningForwards.hpp
	)
SET(scatdb_main
	src/export-hdf5.cpp
	scatdb/export-hdf5.hpp
	scatdb/defs.h
	scatdb/defs.hpp
	scatdb/lowess.hpp
	scatdb/scatdb.h
	scatdb/scatdb.hpp
	scatdb/scatdb_liu.h
	scatdb/zeros.hpp
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
	src/zeros.cpp
	)
set(scatdb_shape
	src/shape.cpp
	src/shapeAlgsProj.cpp
	src/shapeIO.cpp
	src/shapeIOhdfRead.cpp
	src/shapeIOhdfWrite.cpp
	#src/shapeIOtext.cpp
	src/shapeIOtextParsers.cpp
	scatdb/shape/shape.hpp
	scatdb/shape/shapeIO.hpp
	scatdb/shape/shapeAlgs.hpp
	scatdb/shape/shapeForwards.hpp
	private/shapeBackend.hpp
	private/shapeIOtext.hpp
	)
set(scatdb_refract
	scatdb/refract/refract.hpp
	scatdb/refract/refractBase.hpp
	src/refract.cpp
	src/refractImpl.cpp
	private/linterp.h
	#src/refractStore.cpp
	#private/refractBackend.hpp
	)
SET(scatdb_units
	src/units.cpp
	src/unitsPlugins.cpp
	src/unitsSimple.cpp
	scatdb/units/units.hpp
	scatdb/units/unitsPlugins.hpp
	private/unitsBackend.hpp
	)
set(scatdb_docs
	README.md
	changelog.md
	dbformat.md
	INSTALL.md
	)
source_group("Docs" FILES ${scatdb_docs})
SOURCE_GROUP("OS" FILES ${scatdb_os})
SOURCE_GROUP("Debug" FILES ${scatdb_debug})
source_group("Main" FILES ${scatdb_main})
source_group("Shape" FILES ${scatdb_shape})
source_group("Refract" FILES ${scatdb_refract})
source_group("Units" FILES ${scatdb_units})
set( scatdb_srcs
	${scatdb_main}
	${scatdb_debug}
	${scatdb_os}
	${scatdb_refract}
	${scatdb_shape}
	${scatdb_units}
	${scatdb_docs}
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

ADD_SUBDIRECTORY(apps)
adddocs()
