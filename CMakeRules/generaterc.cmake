# CMake script to generate a rc file
macro(generaterc OBJ_BASE EXTEN PRODNAME DESC RC_NAME)

	set(PRODUCTNAME ${PRODNAME})
	set(EXTENSION ${EXTEN})
	set(DESCRIPTION ${DESC})
	set(INTERNALNAME ${OBJ_BASE})
	string(TIMESTAMP YEAR "%Y")
	configure_file("${PROJECT_SOURCE_DIR}/CMakeRules/lib.rc.in"
		"${RC_NAME}"
		)

endmacro()

