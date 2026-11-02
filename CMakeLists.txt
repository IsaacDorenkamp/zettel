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

if(NOT TARGET SQLite::SQLite3)
    FetchContent_Declare(
        sqlite3
        GIT_REPOSITORY https://github.com/sjinks/sqlite3-cmake.git
        GIT_TAG v3.49.1
    )

    FetchContent_MakeAvailable(sqlite3)
endif()

find_package(SQLite3)
if (Curses_FOUND AND NOT TARGET Curses::Curses)
    add_library(Curses::Curses INTERFACE IMPORTED)
    set_target_properties(
        Curses::Curses
        PROPERTIES
            INTERFACE_LINK_LIBRARIES "${CURSES_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURSES_INCLUDE_DIRS}"
    )
elseif(NOT Curses_FOUND)
    message(FATAL_ERROR "Curses was not found and is needed for zettel!")
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
    "src/test_tui.cpp"
    "src/test_uri.cpp"
)

include_directories("include")

if (WIN32)
    add_executable(zettel WIN32 ${SOURCES})
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        add_executable(test   WIN32 ${TEST_SOURCES})
        target_link_libraries(test PRIVATE argparse SQLite::SQLite3 Catch2::Catch2WithMain Curses::Curses)
    endif()
elseif (UNIX)
    add_executable(zettel ${SOURCES})
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        add_executable(test   ${TEST_SOURCES})
        target_link_libraries(test PRIVATE argparse SQLite::SQLite3 Catch2::Catch2WithMain Curses::Curses)
    endif()
    install(TARGETS zettel
        RUNTIME DESTINATION /usr/local/bin)
endif()
target_link_libraries(zettel PRIVATE argparse SQLite::SQLite3 Curses::Curses)
