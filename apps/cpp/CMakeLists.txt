add_executable (scatdb_example_cpp
	test.cpp
	)
target_link_libraries(scatdb_example_cpp ${libs} scatdb)
addapp(scatdb_example_cpp "Examples/scatdb")

