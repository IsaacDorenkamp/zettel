#include "index.hpp"

#include <string>

using std::string, std::unique_ptr, std::vector;

namespace zettel {

namespace models {

struct tag tag::from(const vector<sqlite3_value*>& row) {
    return {
        (const char*)sqlite3_value_text(row[0]),
        (uint32_t)sqlite3_value_int(row[1]),
    };
}

struct zettel zettel::from(const vector<sqlite3_value*>& row) {
    int length = sqlite3_value_bytes(row[0]);
    char* blob = new char[length];
    std::memcpy(blob, sqlite3_value_blob(row[0]), length);
    int blobLength = length;

    length = sqlite3_value_bytes(row[1]);
    char* title = new char[length + 1];
    std::strncpy(title, (const char*)sqlite3_value_text(row[1]), length);

    length = sqlite3_value_bytes(row[2]);
    char* filename = new char[length + 1];
    std::strncpy(filename, (const char*)sqlite3_value_text(row[2]), length);

    return {
        Buffer(blob, blobLength),
        title,
        filename,
        (uint32_t)sqlite3_value_int(row[3])
    };
}

struct zettel_tag zettel_tag::from(const vector<sqlite3_value*>& row) {
    return {
        (uint32_t)sqlite3_value_int(row[0]),
        (uint32_t)sqlite3_value_int(row[1]),
        (uint32_t)sqlite3_value_int(row[2])
    };
}

}

Index::Index(const char* db) : m_db(db) {}

vector<unique_ptr<Id>> Index::search(string tag) {
}

}
