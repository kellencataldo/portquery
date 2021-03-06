cmake_minimum_required (VERSION 3.10.2)
project (portquery)
message(STATUS "STARTING TOP LEVEL CMAKELISTS.TXT")

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

# Setting as debug by default
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this is a cmake generator expression, if the config is debug it will define
# the DEBUG preprocessor definition
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# the header for portquery is kept separetely from the source files
include_directories(libportquery/include)
get_property(directories DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(directory ${directories})
    message(STATUS "INCLUDE DIRECTORY: ${directory}")
endforeach()

add_subdirectory(libportquery)
add_subdirectory(cliportquery)

# this will add the gtest folder with the associated tests
option(BUILD_TEST_BINARIES "Setting this will download and build the googletest project so that tests can be run" ON)
if (BUILD_TEST_BINARIES)
    add_subdirectory(tests)
endif()

set_source_files_properties(tags PROPERTIES GENERATED true)
add_custom_target(tags
    COMMAND ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

message(STATUS "SEARCHING FOR CTAGS")
find_program(CTAGS_PATH ctags)
if(CTAGS_PATH)
    message(STATUS "FOUND CTAGS: ${CTAGS_PATH}. NOW GENERATING")
    add_dependencies(portquery tags)
endif(CTAGS_PATH)
