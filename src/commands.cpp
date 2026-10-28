#include "commands.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ansi.hpp"
#include "content.hpp"
#include "editor.hpp"
#include "format.hpp"
#include "index.hpp"
#include "logging.hpp"
#include "meta.hpp"
#include "parser.hpp"
#include "sql.hpp"
#include "threading.hpp"
#include "tui.hpp"
#include "viewer.hpp"
#include "zettel.hpp"

using std::cout, std::endl, std::make_unique, std::set, std::string, std::stringstream, std::unique_ptr, std::vector;

namespace {

struct CheckTask {
    zettel::models::zettel subject;
    std::filesystem::path path;
    std::set<zettel::Zettel::Id> references;
    std::set<std::string> tags;
};

struct CheckResult {
    zettel::models::zettel subject;
    bool exists;
    std::optional<std::string> error;
    std::vector<zettel::Zettel::Id> refsMissingFromIndex;
    std::vector<zettel::Zettel::Id> refsMissingFromFile;
    std::vector<std::string> tagsMissingFromIndex;
    std::vector<std::string> tagsMissingFromFile;
};

}

static CheckResult checkZettel(const CheckTask& task) {
    using std::vector;
    using zettel::URI, zettel::Zettel, zettel::ZettelException;
    CheckResult check{ task.subject, false, std::nullopt };
    if (!std::filesystem::is_regular_file(task.path)) return check;
    check.exists = true;
    Zettel parsed;
    try {
        parsed = Zettel::load(task.path);
    } catch (const ZettelException& exc) {
        check.error = exc.what();
        return check;
    }
    set<Zettel::Id> refsInBody;
    URI refUri;
    unsigned long zid;
    for (const unique_ptr<zettel::ContentBlock>& block : parsed.content()) {
        if (zettel::ReferenceBlock* ref = dynamic_cast<zettel::ReferenceBlock*>(block.get())) {
            try {
                refUri = URI::parse(ref->uri());
                if (refUri.scheme().compare("zettel") == 0) {
                    try {
                        zid = std::stoul(refUri.path());
                        if (zid > 0xFFFFFFFF) throw std::out_of_range("");
                        refsInBody.insert((Zettel::Id)zid);
                        if (task.references.find(zid) == task.references.end()) check.refsMissingFromIndex.push_back((Zettel::Id)zid);
                    } catch (const std::exception& exc) {}
                }
            } catch (const URI::Exception& exc) {}
        }
    }
    std::set_difference(task.references.begin(), task.references.end(), refsInBody.begin(), refsInBody.end(), std::back_inserter(check.refsMissingFromFile));
    std::set_difference(task.tags.begin(), task.tags.end(), parsed.tags().begin(), parsed.tags().end(), std::back_inserter(check.tagsMissingFromFile));
    std::set_difference(parsed.tags().begin(), parsed.tags().end(), task.tags.begin(), task.tags.end(), std::back_inserter(check.tagsMissingFromIndex));
    return check;
}

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
    throw CommandException(fmt("Unknown search type '%d'", options.type));
}

#define ENSURE_EXISTS(ctx, id) if (!ctx.index->exists(id)) throw CommandException(fmt("No Zettel with ID %d", id))

unique_ptr<Zettel> edit(const Context& ctx, const EditOptions& options) {
    ENSURE_EXISTS(ctx, options.id);
    try {
        Zettel z = Zettel::load(ctx.root / fmt("%u.txt", options.id));
        if (options.title) z.setTitle(*options.title);

        stringstream query;
        if (options.strictAdd) {
            query << "SELECT tag.tag FROM tag WHERE zettel.id = ? AND tag.tag IN (";
            vector<string> params;
            params.push_back(std::to_string(z.id()));
            bool first = true;
            for (const string& tag : options.addTags) {
                if (first) first = false;
                else query << ',';
                query << tag;
                params.push_back(tag);
            }
            query << ") LIMIT 1";
            sql::SQLite::iterator<models::zettel_tag> rset = ctx.db->query<models::zettel_tag>(query.str(), sql::paramvec<string>{params});

            stringstream errmsg("The following tags already exist: ");
            first = true;
            for (; !rset.done(); ++rset) {
                if (first) first = false;
                else errmsg << ", ";
                errmsg << rset->tag;
            }
            throw CommandException(errmsg.str());
        }

        if (!options.detailsOnly) {
            if (options.use_stdin) {
                stringstream content;
                unique_ptr<char[]> buf = make_unique<char[]>(1024);
                size_t bytesRead;
                while ((bytesRead = fread(buf.get(), 1, 1024, stdin))) {
                    content << std::string_view(buf.get(), bytesRead);
                }
                if (ferror(stdin)) throw CommandException("Error reading stdin");
            } else {
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
        Viewer v(&z, ctx);
        tui::run_curses(std::bind(&Viewer::run, &v));
    } catch (const zettel::ZettelException& exc) {
        throw CommandException(fmt("Could not view Zettel %u: %s", options.id, exc.what()));
    }
}

#undef ENSURE_EXISTS

void list(const Context& ctx, const ListOptions& options) {
    vector<models::zettel> zettels = ctx.index->list();
    for (const models::zettel& z : zettels) {
        if (options.plain) {
            cout << z.id << " " << z.title << endl;
        } else {
            cout << ansi::block(std::to_string(z.id)).dim(true) << " " << ansi::block(z.title).bold(true) << endl;
        }
    }
}

uint32_t check(const Context& ctx) {
    vector<models::zettel> zettels = ctx.index->list();
    sys::ThreadSafeQueue<CheckTask> taskQ;
    sys::ThreadSafeQueue<CheckResult> resultQ;

    // TODO: parallelize this piece?
    set<Zettel::Id> refs;
    for (const models::zettel& zettel : zettels) {
        refs = ctx.index->getReferencesFor(zettel.id, false);
        taskQ.put({ zettel, ctx.root / zettel::Zettel::getFilenameForId(zettel.id), refs, ctx.index->getTagsFor(zettel.id) });
    }
    taskQ.shutdown();

    sys::ThreadPool<CheckTask, CheckResult> checker(taskQ, resultQ, &checkZettel);
    // TODO: don't hardcode workers!
    checker.execute(4);

    vector<CheckResult> results;
    resultQ.sink(results);

    uint32_t issues = 0;
    for (const CheckResult& result : results) {
        if (!result.exists) {
            issues++;
            logs::error("Zettel '%s' (ID %u) exists in database, but the file is missing.", result.subject.title.c_str(), result.subject.id);
            continue;
        }
        if (result.error) {
            issues++;
            logs::error("Zettel '%s' (ID %u) is malformed: %s", result.subject.title.c_str(), result.subject.id, result.error->c_str());
            continue;
        }
        for (const Zettel::Id& id : result.refsMissingFromIndex) {
            issues++;
            logs::warn("Zettel '%s' (ID %u) contains a reference to Zettel #%u, but this is not reflected in the database.",
                result.subject.title.c_str(), result.subject.id, id);
        }
        for (const Zettel::Id& id : result.refsMissingFromFile) {
            issues++;
            logs::warn("Zettel '%s' (ID %u) is said to reference Zettel #%u in the database, but this is not reflected in its file.",
                result.subject.title.c_str(), result.subject.id, id);
        }
        for (const string& tag : result.tagsMissingFromIndex) {
            issues++;
            logs::warn("Zettel '%s' (ID %u) contains tag '%s', but this is not reflected in the database.",
                result.subject.title.c_str(), result.subject.id, tag.c_str());
        }
        for (const string& tag : result.tagsMissingFromFile) {
            issues++;
            logs::warn("Zettel '%s' (ID %u) is said to be tagged as '%s' in the database, but this is not reflected in its file.",
                result.subject.title.c_str(), result.subject.id, tag.c_str());
        }
    }
    return issues;
}

}

}
