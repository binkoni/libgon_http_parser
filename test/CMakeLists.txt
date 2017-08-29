set(CMAKE_INSTALL_RPATH "/usr/local/lib")
add_executable(gon_http_parser_test gon_http_parser_test.c)
target_link_libraries(gon_http_parser_test gon_http_parser cmocka)
add_test(NAME gon_http_parser_test COMMAND gon_http_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
