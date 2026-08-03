cmake_minimum_required(VERSION 3.16)
project(zettel)

# Options
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Dependencies
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

find_package(SQLite3)
if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

set(COMMON_SOURCES
    "src/ansi.cpp"
    "src/buffer.cpp"
    "src/content.cpp"
    "src/format.cpp"
    "src/ident.cpp"
    "src/io.cpp"
    "src/references.cpp"
    "src/zettel.cpp"
    "src/kasten.cpp"
    "src/sql.cpp"
)
set(SOURCES
    ${COMMON_SOURCES}
    "src/main.cpp"
)
set(TEST_SOURCES
    ${COMMON_SOURCES}
    "src/test.cpp"
)

include_directories("include")

if (WIN32)
    add_executable(zettel WIN32 ${SOURCES})
    add_executable(test   WIN32 ${TEST_SOURCES})
elseif (UNIX)
    add_executable(zettel ${SOURCES})
    add_executable(test   ${TEST_SOURCES})
endif()
target_link_libraries(zettel PRIVATE argparse SQLite3::SQLite3)
target_link_libraries(test PRIVATE argparse SQLite3::SQLite3 Catch2::Catch2WithMain)
