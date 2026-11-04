cmake_minimum_required(VERSION 3.16)
project(zettel)

# Options
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions(-DNDEBUG)
endif()

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
if (TARGET SQLite3::SQLite3)
    message(STATUS "Found SQLite3::SQLite3")
elseif (TARGET SQLite::SQLite3)
    message(status "Aliasing SQLite::SQLite3")
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
else()
    FetchContent_Declare(
        sqlite3
        GIT_REPOSITORY https://github.com/sjinks/sqlite3-cmake.git
        GIT_TAG v3.49.1
    )

    FetchContent_MakeAvailable(sqlite3)
endif()

set(COMMON_SOURCES
    "src/ansi.cpp"
    "src/buffer.cpp"
    "src/commands.cpp"
    "src/content.cpp"
    "src/editor.cpp"
    "src/format.cpp"
    "src/io.cpp"
    "src/index.cpp"
    "src/logging.cpp"
    "src/meta.cpp"
    "src/zettel.cpp"
    "src/sql.cpp"
    "src/parser.cpp"
    "src/search.cpp"
    "src/sys.cpp"
    "src/tui.cpp"
    "src/uri.cpp"
    "src/viewer.cpp"
)
set(SOURCES
    ${COMMON_SOURCES}
    "src/main.cpp"
)
set(TEST_SOURCES
    ${COMMON_SOURCES}
    "src/test_index.cpp"
    "src/test_sql.cpp"
    "src/test_parser.cpp"
    "src/test_search.cpp"
    "src/test_tui.cpp"
    "src/test_uri.cpp"
)

find_package(Boost CONFIG COMPONENTS regex)
if (NOT TARGET Boost::regex)
    message(FATAL_ERROR "Boost::regex not found, ensure Boost is installed and findable")
endif()

include_directories("include")

if (WIN32)
    add_executable(zettel ${SOURCES})
    find_path(PDCURSES_INCLUDE_DIR curses.h REQUIRED)
    find_library(PDCURSES_LIBRARY NAMES pdcurses REQUIRED)
    include_directories(zettel ${PDCURSES_INCLUDE_DIR})
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        add_executable(test   ${TEST_SOURCES})
        target_link_libraries(test PRIVATE argparse SQLite3::SQLite3 ${PDCURSES_LIBRARY} Boost::regex Catch2::Catch2WithMain)
    endif()
    target_link_libraries(zettel PRIVATE argparse SQLite3::SQLite3 ${PDCURSES_LIBRARY})
elseif (UNIX)
    add_executable(zettel ${SOURCES})
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        add_executable(test   ${TEST_SOURCES})
        target_link_libraries(test PRIVATE argparse SQLite3::SQLite3 Catch2::Catch2WithMain ncurses Boost::regex)
    endif()
    target_link_libraries(zettel PRIVATE argparse SQLite3::SQLite3 ncurses)
    install(TARGETS zettel
        RUNTIME DESTINATION /usr/local/bin)
endif()
