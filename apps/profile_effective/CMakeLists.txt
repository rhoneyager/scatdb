ADD_LIBRARY (scatdb_profile_base STATIC
	input_parse.cpp
	output.cpp
	parser.cpp
	parser.hpp
	README.md
	)

ADD_EXECUTABLE(scatdb_profile_parse
	parse_main.cpp
	)
ADD_EXECUTABLE(scatdb_profile_evaluate
	evaluate_main.cpp
	)
ADD_EXECUTABLE(scatdb_profile_shape
	shape_main.cpp
	)
ADD_EXECUTABLE(scatdb_profile_shape_from_scatdb
	shape_from_scatdb.cpp
	)
target_link_libraries(scatdb_profile_base scatdb )
addlib(scatdb_profile_base Profiles/Forward STATIC)
target_link_libraries(scatdb_profile_parse scatdb_profile_base scatdb)
addapp(scatdb_profile_parse Profiles/Forward)
target_link_libraries(scatdb_profile_evaluate scatdb_profile_base scatdb)
addapp(scatdb_profile_evaluate Profiles/Forward)
target_link_libraries(scatdb_profile_shape scatdb_profile_base scatdb)
addapp(scatdb_profile_shape Profiles/Forward)
target_link_libraries(scatdb_profile_shape_from_scatdb scatdb_profile_base scatdb)
addapp(scatdb_profile_shape_from_scatdb Profiles/Forward)
