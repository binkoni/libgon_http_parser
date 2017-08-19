add_library(gon_http_parser SHARED gon_http_parser.c)
install(TARGETS gon_http_parser
         LIBRARY DESTINATION lib)

