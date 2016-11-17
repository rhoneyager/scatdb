add_executable (scatdb_example_fortran
	test-f.f90
	)
set_target_properties( scatdb_example_fortran PROPERTIES LINKER_LANGUAGE Fortran)
target_link_libraries(scatdb_example_fortran ${libs} scatdb)
addapp(scatdb_example_fortran "Examples/scatdb")

