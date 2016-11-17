add_executable (scatdb_example_c
	test-c.c
	)
target_link_libraries(scatdb_example_c ${libs} scatdb)
addapp(scatdb_example_c "Examples/scatdb")
