macro(adddocs )

find_package(Doxygen)
option (BUILD_DOCUMENTATION
    "Build the documentation for this library" OFF)

if(BUILD_DOCUMENTATION)

    option (BUILD_DOCUMENTATION_IN_ALL
        "Build documentation automatically with 'make all'. Also used for 'make install' and 'make package'" OFF)

    if (NOT DOXYGEN_FOUND)
        message(SEND_ERROR "Documentation build requested but Doxygen is not found.")
    endif()


    configure_file(Doxyfile.in
        "${PROJECT_BINARY_DIR}/Doxyfile" @ONLY)

    if (BUILD_DOCUMENTATION_IN_ALL)
        set (ALL_FLAG ALL)
    else()
        set (ALL_FLAG "")
    endif()

    # This builds the html docs
    add_custom_target(doc-html ${ALL_FLAG}
        ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating API html documentation with Doxygen" VERBATIM
    )
    # This builds the latex docs
    add_custom_target(doc-latex ${ALL_FLAG}
        latex refman.tex
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs/latex
        COMMENT "Generating API pdf documentation with Doxygen" VERBATIM
    )

    add_custom_target(docs ${ALL_FLAG} DEPENDS doc-html doc-latex)
endif()

if (BUILD_DOCUMENTATION_IN_ALL)
    # Provides html and pdf
    install(CODE "execute_process(COMMAND ${CMAKE_BUILD_TOOL} docs)")
    # html
    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/html/ DESTINATION ${INSTALL_DOC_DIR}/html)
    # pdf
    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/latex/ DESTINATION ${INSTALL_DOC_DIR}/latex)
endif()

endmacro(adddocs )

