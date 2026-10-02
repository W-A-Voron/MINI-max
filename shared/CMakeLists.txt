add_library(mmproto STATIC src/mmproto.c)
target_include_directories(mmproto PUBLIC include)
target_compile_definitions(mmproto PUBLIC MINIMAX_VERSION="${PROJECT_VERSION}")
set_target_properties(mmproto PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(test_mmproto tests/test_mmproto.c)
target_link_libraries(test_mmproto PRIVATE mmproto)
add_test(NAME mmproto COMMAND test_mmproto)
