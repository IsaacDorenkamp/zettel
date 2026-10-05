#include "commands.hpp"

#include "editor.hpp"
#include "index.hpp"
#include "meta.hpp"
#include "sql.hpp"

#define ZETTEL_VERSION "0.0.1"

using std::string, std::unique_ptr;

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
            db.query("CREATE TABLE zettel (id INTEGER PRIMARY KEY, title TEXT NOT NULL)");
            db.query("CREATE TABLE tag (zettel_id INTEGER NOT NULL, tag TEXT NOT NULL)");
            db.query("CREATE UNIQUE INDEX zid_tag ON tag(zettel_id, tag)");
            db.query("CREATE TABLE reference (source INTEGER, dest INTEGER, FOREIGN KEY(source) REFERENCES zettel(id), FOREIGN KEY(dest) REFERENCES zettel(id))");
            db.query("CREATE UNIQUE INDEX source_dest ON reference(source, dest)");
            db.commit();
        } catch (const sql::SQLite::Exception& exc) {
            throw CommandException(fmt("Unable to create SQLite database at %s: %s", dbfile.c_str(), exc.what()));
        }
    } else {
        throw CommandException(fmt("Unable to open SQLite database at %s", dbfile.c_str()));
    }
}

void cmd_new(const Context& ctx, const NewOptions& opts) {
    std::string content;
    if (opts.edit) {
        unique_ptr<Editor> ed = Editor::getInstance(ctx.root / ".zettel");
    }

    Index idx(ctx.root.c_str());
    uint32_t id = idx.nextId();
    Zettel result(id, opts.title, ctx.root / fmt("%u.txt"));
    for (const string& tag : opts.tags) result.addTag(tag);
    result.save();
}

}

}
