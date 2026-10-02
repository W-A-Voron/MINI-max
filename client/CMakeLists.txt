find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)

include(FetchContent)
FetchContent_Declare(tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0)
FetchContent_MakeAvailable(tomlplusplus)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(minimax-client
  src/main.cpp
  src/config.cpp
  src/net/connection.cpp
  src/core/chats_model.cpp
  src/core/messages_model.cpp
  src/ui/theme.cpp
  src/ui/icons.cpp
  src/ui/delegates.cpp
  src/ui/sidebar.cpp
  src/ui/chat_view.cpp
  src/ui/profile_panel.cpp
  src/ui/main_window.cpp
  src/resources/resources.qrc)
target_include_directories(minimax-client PRIVATE src)
if(WIN32)
  set_target_properties(minimax-client PROPERTIES WIN32_EXECUTABLE ON)  # no console window
endif()
target_link_libraries(minimax-client PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg mmproto tomlplusplus::tomlplusplus)

add_executable(test_client_config tests/test_config.cpp src/config.cpp)
target_include_directories(test_client_config PRIVATE src)
target_link_libraries(test_client_config PRIVATE Qt6::Core tomlplusplus::tomlplusplus)
add_test(NAME client_config COMMAND test_client_config ${PROJECT_SOURCE_DIR}/configs/client.toml)
