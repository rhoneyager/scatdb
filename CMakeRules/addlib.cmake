# CMake script to add a rtmath library

macro(addlib libname libshared )
	if ("" STREQUAL "${ARGV2}")
		set(headername ${libname})
	else()
		set(headername ${ARGV2})
	endif()

	SET_TARGET_PROPERTIES( ${libname} PROPERTIES RELEASE_POSTFIX _Release${configappend} )
	SET_TARGET_PROPERTIES( ${libname} PROPERTIES MINSIZEREL_POSTFIX _MinSizeRel${configappend} )
	SET_TARGET_PROPERTIES( ${libname} PROPERTIES RELWITHDEBINFO_POSTFIX _RelWithDebInfo${configappend} )
	SET_TARGET_PROPERTIES( ${libname} PROPERTIES DEBUG_POSTFIX _Debug${configappend} )
	set_target_properties( ${libname} PROPERTIES FOLDER "Libs")
	
	# This is for determining the build type (esp. used in registry code)
	#target_compile_definitions(${libname} PRIVATE
	#	BUILDTYPE="${CMAKE_BUILD_TYPE}")
	target_compile_definitions(${libname} PRIVATE BUILDCONF="${CMAKE_BUILD_TYPE}")
	target_compile_definitions(${libname} PRIVATE BUILDTYPE=BUILDTYPE_$<CONFIGURATION>)
	# These two are for symbol export
	target_compile_definitions(${libname} PRIVATE EXPORTING_${headername})
	target_compile_definitions(${libname} PUBLIC SHARED_${headername}=$<STREQUAL:${libshared},SHARED>)
	INSTALL(TARGETS ${libname} 
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_BIN_DIR}/bin${configappend}
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_LIB_DIR}/lib${configappend}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_LIB_DIR}/lib${configappend}
		COMPONENT Plugins)
	#delayedsigning( ${libname} )

endmacro(addlib libname headername)

macro(storebin objname)
set_target_properties( ${objname}
    PROPERTIES
    #  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo"
)

endmacro(storebin objname)

macro(storeplugin objname folder)
set_target_properties( ${objname}
    PROPERTIES
    #  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${folder}-plugins"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${folder}-plugins"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${folder}-plugins"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${folder}-plugins"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${folder}-plugins"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${folder}-plugins"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${folder}-plugins"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${folder}-plugins"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${folder}-plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${folder}-plugins"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${folder}-plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${folder}-plugins"
)

endmacro(storeplugin objname folder)

macro(addplugin appname foldername folder)
	set_target_properties( ${appname} PROPERTIES FOLDER "Plugins/${foldername}")
	INSTALL(TARGETS ${appname} 
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_BIN_DIR}/bin${configappend}/${folder}-plugins
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_LIB_DIR}/lib${configappend}/${folder}-plugins
		ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${REL_LIB_DIR}/lib${configappend}/${folder}-plugins
		COMPONENT Plugins)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	IF(DEFINED COMMON_CFLAGS) 
		set_target_properties(${appname} PROPERTIES COMPILE_FLAGS ${COMMON_CFLAGS})
	ENDIF()
	storeplugin(${appname} ${folder})
endmacro(addplugin appname)
