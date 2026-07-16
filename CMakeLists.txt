cmake_minimum_required(VERSION 3.16)
project(zettel)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(SOURCES
    "src/content.cpp"
    "src/ident.cpp"
    "src/references.cpp"
    "src/zettel.cpp"
)

include_directories("include")

if (WIN32)
    add_executable(lib WIN32 ${SOURCES})
elseif (UNIX)
    add_executable(lib ${SOURCES})
endif()
