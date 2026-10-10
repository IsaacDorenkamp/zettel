#include "commands.hpp"

#include "content.hpp"
#include "editor.hpp"
#include "format.hpp"
#include "index.hpp"
#include "meta.hpp"
#include "sql.hpp"

#include "zettel.hpp"
#define ZETTEL_VERSION "0.0.1"

using std::make_unique, std::string, std::unique_ptr, std::vector;

namespace zettel {

namespace cmd {

bool Context::valid() const {
    return db->valid();
}

void initialize(const Context& ctx) {
    using std::filesystem::path, std::filesystem::is_directory, std::filesystem::create_directory;
    if (is_directory(ctx.dotdir)) {
        throw CommandException(fmt(".zettel already exists in %s", ctx.root.c_str()));
    } else {
        try {
            create_directory(ctx.dotdir);
        } catch (const std::filesystem::filesystem_error& exc) {
            throw CommandException("Unable to create .zettel");
        }
    }

    path metafile = ctx.dotdir / "META";
    zettel::meta::write(metafile, { {"version", ZETTEL_VERSION} });

    sql::SQLite::ConnectResult result = ctx.db->connect();
    sql::SQLite& db = *ctx.db;
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
            throw CommandException(fmt("Unable to create SQLite database at %s: %s", db.uri().c_str(), exc.what()));
        }
    } else {
        throw CommandException(fmt("Unable to open SQLite database at %s", db.uri().c_str()));
    }
}

unique_ptr<Zettel> make_new(const Context& ctx, const NewOptions& opts) {
    std::string content;
    std::filesystem::path dotdir = ctx.root / ".zettel";
    if (opts.edit) {
        unique_ptr<Editor> ed = Editor::getInstance(dotdir);
        try {
            content = ed->readInput();
        } catch (const zettel::Editor::Exception& exc) {
            throw CommandException(exc.what());
        }
    }

    uint32_t id;
    try {
        id = ctx.index->nextId();
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Could not get next ID: %s", exc.what()));
    }
    unique_ptr<Zettel> result = make_unique<Zettel>(id, opts.title, ctx.root / fmt("%u.txt", id));
    result->addContentBlock(make_unique<zettel::TextBlock>(0, content));
    for (const string& tag : opts.tags) result->addTag(tag);
    try {
        ctx.index->begin();
        ctx.index->insert(result.get());
        result->save();
        ctx.index->commit();
    } catch (const zettel::ZettelException& exc) {
        throw CommandException(fmt("Error saving Zettel: %s", exc.what()));
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Error updating index: %s", exc.what()));
    }
    return result;
}

vector<models::zettel> search(const Context& ctx, const SearchOptions& options) {
    std::filesystem::path dotdir = ctx.root / ".zettel";
    try {
        return ctx.index->search(options.tag);
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Error searching index: %s", exc.what()));
    }
}

unique_ptr<Zettel> edit(const Context& ctx, const EditOptions& options) {
    std::filesystem::path dotdir = ctx.root / ".zettel";
    try {
        Zettel z = Zettel::load(ctx.root / fmt("%u.txt", options.id));
        unique_ptr<Editor> ed = Editor::getInstance(dotdir);
        string updatedContent = ed->readInput(z.getContentBlock(0)->format(FormatOptions{ DisplayMode::ASCII, 0, 0 }));
        z.clearContent();
        z.addContentBlock(make_unique<zettel::TextBlock>(0, updatedContent));

        ctx.index->begin();
        ctx.index->update(&z);
        z.save();
        ctx.index->commit();

        return make_unique<Zettel>(z);
    } catch (const zettel::ZettelException& exc) {
        throw CommandException(fmt("Could not edit Zettel %u: %s", options.id, exc.what()));
    } catch (const zettel::Editor::Exception& exc) {
        throw CommandException(fmt("Error getting input from editor: %s", exc.what()));
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Error updating index: %s", exc.what()));
    }
}

}

}
