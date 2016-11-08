# Take the variable {appname}, and link libraries,
# set properties and create an INSTALL target
macro(addapp appname foldername)
	set_target_properties( ${appname} PROPERTIES FOLDER "Apps/${foldername}")
	INSTALL(TARGETS ${appname} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_BIN_DIR}/bin${configappend} COMPONENT Applications)
	
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	storebin(${appname})
endmacro(addapp appname)

macro(add_header_files srcs)
  if( hds )
    set_source_files_properties(${hds} PROPERTIES HEADER_FILE_ONLY ON)
    list(APPEND ${srcs} ${hds})
  endif()
endmacro(add_header_files srcs)

macro(addbasicapp appname)
	set(${appname}_LIBRARIES "")
	set(${appname}_INCLUDE_DIRS "")

	target_link_libraries (${appname} ${${appname}_LIBRARIES})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	include_directories(${${appname}_INCLUDE_DIRS})

	INSTALL(TARGETS ${appname} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_BIN_DIR}/bin${configappend} COMPONENT Applications)
	set_target_properties(${appname} PROPERTIES FOLDER "Apps")
endmacro(addbasicapp appname)


