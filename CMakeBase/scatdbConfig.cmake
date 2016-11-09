# - Config file for the scatdb package
# It defines the following variables
#  SCATDB_INCLUDE_DIRS - include directories
#  SCATDB_LIBRARIES    - libraries to link against
#  SCATDB_EXECUTABLE   - an executable
 
# Compute paths
get_filename_component(SCATDB_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(SCATDB_INCLUDE_DIRS "@CONF_INCLUDE_DIRS@")
 
# Our library dependencies (contains definitions for IMPORTED targets)
include("${SCATDB_CMAKE_DIR}/scatdbTargets.cmake")
 
set(SCATDB_LIBRARIES scatdb)

