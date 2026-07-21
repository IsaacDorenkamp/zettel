#pragma once

#include <cctype>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace zettel {

enum class DisplayMode {
    ASCII, ANSI
};

struct FormatOptions {
    DisplayMode mode;
    uint16_t line_size;
    uint16_t first_line_offset;
};

template<typename ... Args>
std::string fmt( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

std::string stripWhitespace(std::string other);


}
