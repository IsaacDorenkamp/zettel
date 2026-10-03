#include "commands.hpp"

#include "meta.hpp"
#include "sql.hpp"

#define ZETTEL_VERSION "0.0.1"

namespace zettel {

namespace cmd {

void initialize(const Context& ctx) {
    using std::filesystem::path, std::filesystem::is_directory, std::filesystem::create_directory;
    path dotdir = ctx.root / ".zettel";
    if (is_directory(dotdir)) {
        throw CommandException(fmt(".zettel already exists in %s", ctx.root.c_str()));
    } else {
        try {
            create_directory(dotdir);
        } catch (const std::filesystem::filesystem_error& exc) {
            throw CommandException("Unable to create .zettel");
        }
    }

    path metafile = dotdir / "META";
    zettel::meta::write(metafile, { {"version", ZETTEL_VERSION} });

    path dbfile = dotdir / "struct.db";
    sql::SQLite db(dbfile.c_str());
    if (db.valid()) {
        try {
            db.begin();
            db.query("CREATE TABLE zettel (id TEXT PRIMARY KEY, title TEXT NOT NULL)");
            db.query("CREATE TABLE tag (zettel_id TEXT NOT NULL, tag TEXT NOT NULL)");
            db.query("CREATE UNIQUE INDEX zid_tag ON tag(zettel_id, tag)");
            db.query("CREATE TABLE reference (source TEXT, dest TEXT, FOREIGN KEY(source) REFERENCES zettel(id), FOREIGN KEY(dest) REFERENCES zettel(id))");
            db.query("CREATE UNIQUE INDEX source_dest ON reference(source, dest)");
            db.commit();
        } catch (const sql::SQLite::Exception& exc) {
            throw CommandException(fmt("Unable to create SQLite database at %s: %s", dbfile.c_str(), exc.what()));
        }
    } else {
        throw CommandException(fmt("Unable to open SQLite database at %s", dbfile.c_str()));
    }
}

}

}
