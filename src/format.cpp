#include "format.hpp"

#include <cctype>
#include <sstream>

using std::string, std::stringstream;

namespace zettel {

string stripWhitespace(string other) {
    size_t start;
    size_t end;

    for (start = 0; start < other.size() && std::isspace(other[start]); start++);
    for (end = other.size() - 1; end > 0 && end >= start && std::isspace(other[end]); end--);

    if (start < end) return other.substr(start, (end - start) + 1);
    else return other;
}

string wrapString(string str, const FormatOptions& options) {
    if (options.line_size == 0) return str;

    stringstream result;
    uint16_t lineSize = options.line_size - options.first_line_offset;
    uint16_t index = 0;
    do {
        if (index > 0) result << '\n';
        string portion = str.substr(index, lineSize);
        result << portion;
        index += portion.size();
        lineSize = options.line_size;
    } while (index < str.size());
    return result.str();
}

}
