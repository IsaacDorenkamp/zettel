#include "commands.hpp"

#include <memory>
#include <sstream>
#include <vector>

#include "content.hpp"
#include "editor.hpp"
#include "format.hpp"
#include "index.hpp"
#include "meta.hpp"
#include "parser.hpp"
#include "sql.hpp"
#include "tui.hpp"
#include "viewer.hpp"
#include "zettel.hpp"

using std::make_unique, std::set, std::string, std::stringstream, std::unique_ptr, std::vector;

namespace zettel {

namespace cmd {

bool Context::valid() const {
    return db->valid();
}

void initialize(const Context& ctx) {
    using std::filesystem::path, std::filesystem::is_directory, std::filesystem::create_directory;
    if (!ctx.dotdir.empty()) {
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
    }

    sql::SQLite::ConnectResult result = ctx.db->connect();
    sql::SQLite& db = *ctx.db;
    if (db.valid()) {
        try {
            db.begin();
            db.query("CREATE TABLE zettel (id INTEGER PRIMARY KEY, title TEXT NOT NULL)");
            db.query("CREATE TABLE tag (zettel_id INTEGER NOT NULL, tag TEXT NOT NULL, FOREIGN KEY(zettel_id) REFERENCES zettel(id) ON DELETE CASCADE)");
            db.query("CREATE UNIQUE INDEX zid_tag ON tag(zettel_id, tag)");
            // NOTE: dest does not reference "zettel" because it is allowed to fail to refer
            db.query("CREATE TABLE reference (source INTEGER, dest INTEGER, valid INTEGER, FOREIGN KEY(source) REFERENCES zettel(id) ON DELETE CASCADE)");
            db.query("CREATE UNIQUE INDEX source_dest ON reference(source, dest)");
            db.commit();
        } catch (const sql::SQLite::Exception& exc) {
            db.rollback();
            throw CommandException(fmt("Unable to create SQLite database at %s: %s", db.uri().c_str(), exc.what()));
        }
    } else {
        throw CommandException(fmt("Unable to open SQLite database at %s", db.uri().c_str()));
    }
}

unique_ptr<Zettel> make_new(const Context& ctx, const NewOptions& opts) {
    std::string content;
    if (opts.edit) {
        unique_ptr<Editor> ed = Editor::getInstance(ctx.dotdir);
        try {
            content = ed->readInput();
        } catch (const zettel::Editor::Exception& exc) {
            throw CommandException(fmt("Unable to read input from editor: %s", exc.what()));
        }
    }

    uint32_t id;
    try {
        id = ctx.index->nextId();
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Could not get next ID: %s", exc.what()));
    }

    unique_ptr<Zettel> result = make_unique<Zettel>(id, opts.title, ctx.root / fmt("%u.txt", id));

    std::istringstream ss(content);
    unique_ptr<Parser> p(new DefaultParser(ss));
    while (!p->done()) result->addContentBlock(p->next());
    for (const string& tag : opts.tags) result->addTag(tag);
    try {
        ctx.index->begin();
        ctx.index->insert(result.get());
        result->save();
        ctx.index->commit();
    } catch (const zettel::ZettelException& exc) {
        ctx.index->rollback();
        throw CommandException(fmt("Error saving Zettel: %s", exc.what()));
    } catch (const sql::SQLite::Exception& exc) {
        ctx.index->rollback();
        throw CommandException(fmt("Error updating index: %s", exc.what()));
    }
    return result;
}

vector<models::zettel> search(const Context& ctx, const SearchOptions& options) {
    try {
        switch (options.type) {
        case SearchType::Tag:
            return ctx.index->searchTag(options.term);
        case SearchType::Title:
            return ctx.index->searchTitle(options.term);
        case SearchType::FullText:
            return ctx.index->searchFullText(options.term, ctx.root);
        }
    } catch (const sql::SQLite::Exception& exc) {
        throw CommandException(fmt("Error searching index: %s", exc.what()));
    }
}

#define ENSURE_EXISTS(ctx, id) if (!ctx.index->exists(id)) throw CommandException(fmt("No Zettel with ID %d", id))

unique_ptr<Zettel> edit(const Context& ctx, const EditOptions& options) {
    ENSURE_EXISTS(ctx, options.id);
    try {
        Zettel z = Zettel::load(ctx.root / fmt("%u.txt", options.id));
        if (options.title) z.setTitle(*options.title);

        if (!options.detailsOnly) {
            unique_ptr<Editor> ed = Editor::getInstance(ctx.dotdir);
            stringstream defaultContent;
            for (const unique_ptr<ContentBlock>& block : z.content()) {
                defaultContent << block->format(PLAINTEXT_FORMAT);
            }
            string updatedContent = ed->readInput(defaultContent.str());
            z.clearContent();

            stringstream rawContent(updatedContent);
            rawContent.seekg(0);
            unique_ptr<Parser> p(new DefaultParser(rawContent));
            while (!p->done()) z.addContentBlock(p->next());
        }

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
        ctx.index->rollback();
        throw CommandException(fmt("Error updating index: %s", exc.what()));
    }
}

void remove(const Context& ctx, const DeleteOptions& options) {
    ENSURE_EXISTS(ctx, options.id);
    try {
        Zettel z = Zettel::load(ctx.root / fmt("%u.txt", options.id));
        set<Zettel::Id> pointedToBy = ctx.index->getReferencesTo(z.id());
        if (pointedToBy.size() > 0 && !options.force) {
            stringstream ss("This Zettel is referenced by the following Zettels: ");
            ss.seekp(0, std::ios_base::end);
            bool first = true;
            for (const Zettel::Id& id : pointedToBy) {
                if (!first) ss << ", ";
                else first = false;
                ss << id;
            }
            throw CommandException(ss.str());
        }

        ctx.index->begin();
        ctx.index->remove(&z);
        std::filesystem::remove(z.file());
        ctx.index->commit();
    } catch(const zettel::ZettelException& exc) {
        throw CommandException(fmt("Could not edit Zettel %u: %s", options.id, exc.what()));
    } catch (const sql::SQLite::Exception& exc) {
        ctx.index->rollback();
        throw CommandException(fmt("Error removing Zettel: %s", exc.what()));
    }
}

void view(const Context& ctx, const ViewOptions& options) {
    ENSURE_EXISTS(ctx, options.id);
    try {
        Zettel z = Zettel::load(ctx.root / fmt("%u.txt", options.id));
        Viewer v(&z);
        tui::run_curses(std::bind(&Viewer::run, &v, std::ref(ctx)));
    } catch (const zettel::ZettelException& exc) {
        throw CommandException(fmt("Could not view Zettel %u: %s", options.id, exc.what()));
    }
}

#undef ENSURE_EXISTS

}

}
