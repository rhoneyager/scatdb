add_executable (scatdb_envelopes envelopes.cpp)
target_link_libraries(scatdb_envelopes ${libs} scatdb)
addapp(scatdb_envelopes "Examples/scatdb")

