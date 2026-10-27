cmake_minimum_required(VERSION 3.10.0)
project(CollegePlanner VERSION 0.1.0 LANGUAGES C CXX)


find_package(ftxui CONFIG QUIET)

file(GLOB PLANNER_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(CollegePlanner ${PLANNER_SOURCES})
target_compile_options(CollegePlanner PRIVATE -Wall -Wextra)
target_link_libraries(CollegePlanner PRIVATE ftxui::component ftxui::dom ftxui::screen)   

foreach(script usos_to_in.py parser.py models.py)
    configure_file(src/${script} ${CMAKE_CURRENT_BINARY_DIR}/src/${script} COPYONLY)
endforeach()