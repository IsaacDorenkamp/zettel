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

set(SOURCES
    "src/ansi.cpp"
    "src/content.cpp"
    "src/format.cpp"
    "src/ident.cpp"
    "src/io.cpp"
    "src/references.cpp"
    "src/zettel.cpp"
    "src/kasten.cpp"

    "src/main.cpp"
)

include_directories("include")

if (WIN32)
    add_executable(zettel WIN32 ${SOURCES})
elseif (UNIX)
    add_executable(zettel ${SOURCES})
endif()
target_link_libraries(zettel argparse)
