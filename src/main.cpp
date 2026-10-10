#include <filesystem>
#include <iostream>

#include "argparse/argparse.hpp"
#include "ansi.hpp"
#include "commands.hpp"

using namespace std;
using namespace zettel;

std::filesystem::path get_root_path() {
#ifndef NDEBUG
    std::filesystem::path tmp_path("/tmp/zettel");
    if (!std::filesystem::is_directory(tmp_path)) {
        std::filesystem::create_directory(tmp_path);
    }
    return tmp_path;
#else
    return std::filesystem::current_path();
#endif
}

int main(int argc, char **argv) {
    argparse::ArgumentParser program("zettel");
    program.add_argument("--quiet", "-q").flag().help("Only outputs the minimal information requested for a command, with no visible success messages. Error messages are still printed to stderr.");

    argparse::ArgumentParser init_cmd("init");
    init_cmd.add_description("Initialize a local Zettelkasten.");

    argparse::ArgumentParser new_cmd("new");
    new_cmd.add_argument("title").required().help("The title of the new Zettel.");
    new_cmd.add_argument("--tag", "-t")
        .default_value<vector<string>>({})
        .append()
        .help("A tag which is used to identify the Zettel.");
    new_cmd.add_argument("--noedit").flag().help("If specified, create the Zettel without any content instead of immediately editing.");
    new_cmd.add_description("Create a new Zettel.");

    argparse::ArgumentParser search_cmd("search");
    search_cmd.add_argument("tag").required().help("The tag to search against.");
    search_cmd.add_description("Search for Zettels by tag.");

    argparse::ArgumentParser edit_cmd("edit");
    edit_cmd.add_argument("id").scan<'u', uint32_t>().required().help("The ID of the Zettel to edit.");

    program.add_subparser(init_cmd);
    program.add_subparser(new_cmd);
    program.add_subparser(search_cmd);
    program.add_subparser(edit_cmd);

    filesystem::path root = get_root_path();
    std::shared_ptr<sql::SQLite> db = make_shared<sql::SQLite>((root / ".zettel" / "struct.db").c_str());
    cmd::Context ctx{
        .root = root,
        .dotdir = root / ".zettel",
        .db = db,
        .index = make_unique<Index>(db)
    };

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& exc) {
        cerr << exc.what() << endl;
        return EXIT_FAILURE;
    }

    bool quiet = program.get<bool>("--quiet");
    try {
        if (program.is_subcommand_used("init")) {
            cmd::initialize(ctx);
            if (!quiet) cout << ansi::block("Successfully").foreground(ansi::Color::GREEN).bold(true) << " initialized Zettelkasten at "
                << ansi::block(ctx.root.string()).bold(true).italic(true) << endl;
        } else if (program.is_subcommand_used("new")) {
            cmd::NewOptions opts{
                new_cmd.get<string>("title"),
                new_cmd.get<vector<string>>("--tag"),
                !new_cmd.get<bool>("--noedit")
            };
            unique_ptr<Zettel> z = cmd::make_new(ctx, opts);
            if (!quiet) cout << ansi::block("Created").foreground(ansi::Color::GREEN).bold(true) << " Zettel "
                << ansi::block(fmt("%u", z->id())) << endl;
        } else if (program.is_subcommand_used("search")) {
            cmd::SearchOptions opts{
                search_cmd.get<string>("tag"),
            };
            vector<models::zettel> results = cmd::search(ctx, opts);
            for (const models::zettel& entry : results) cout << entry.title << " (ID " << entry.id << ")" << endl;
            if (!quiet) cout << ansi::block("Found ").italic(true) << ansi::block(
                    std::to_string(results.size())
                ).bold(true).italic(true).foreground(results.size() == 0 ? ansi::Color::RED : ansi::Color::WHITE)
                << ansi::block(" result" + string(results.size() != 1 ? "s" : "")).italic(true) << endl;
        } else if (program.is_subcommand_used("edit")) {
            cmd::EditOptions opts{
                edit_cmd.get<uint32_t>("id")
            };
            unique_ptr<Zettel> zet = cmd::edit(ctx, opts);
            if (!quiet) cout << ansi::block("Updated ").italic(true) << ansi::block(zet->title()).bold(true) << endl;
        }
        else throw cmd::CommandException("No command specified.");
    } catch (const std::exception& exc) {
        cerr << ansi::block("ERROR: ").foreground(ansi::Color::RED).bold(true) << exc.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
