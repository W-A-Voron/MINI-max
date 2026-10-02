include(FetchContent)
FetchContent_Declare(tomlc99
  GIT_REPOSITORY https://github.com/cktan/tomlc99.git
  GIT_TAG master
  SOURCE_SUBDIR _no_cmake_here)
FetchContent_MakeAvailable(tomlc99)
add_library(tomlc99 STATIC ${tomlc99_SOURCE_DIR}/toml.c)
target_include_directories(tomlc99 PUBLIC ${tomlc99_SOURCE_DIR})
if(MSVC)
  target_compile_options(tomlc99 PRIVATE /w)
else()
  target_compile_options(tomlc99 PRIVATE -w)
endif()

option(MINIMAX_POLL_BACKEND "Use the portable poll() reactor even on Linux (default: epoll on Linux)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT MINIMAX_POLL_BACKEND)
  set(MM_POLLER src/net/poller_epoll.c)
else()
  set(MM_POLLER src/net/poller_poll.c)
endif()

add_executable(minimax-server
  src/main.c src/config.c src/util/log.c
  src/net/sock.c ${MM_POLLER} src/net/server.c src/net/dispatch.c)
target_include_directories(minimax-server PRIVATE src)
target_link_libraries(minimax-server PRIVATE mmproto tomlc99)
if(WIN32)
  target_link_libraries(minimax-server PRIVATE ws2_32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(minimax-server PRIVATE Threads::Threads)
  target_compile_definitions(minimax-server PRIVATE _GNU_SOURCE)
endif()

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME server_integration
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_server.py
            $<TARGET_FILE:minimax-server> ${PROJECT_SOURCE_DIR}/configs/server.toml)
endif()
