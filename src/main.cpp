#include <filesystem>
#include <iostream>

#include "argparse/argparse.hpp"
#include "ansi.hpp"
#include "commands.hpp"
#include "editor.hpp"

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

    argparse::ArgumentParser init_cmd("init");
    init_cmd.add_description("Initialize a local Zettelkasten.");

    argparse::ArgumentParser new_cmd("new");
    new_cmd.add_argument("title");
    new_cmd.add_argument("--tag", "-t")
        .default_value<vector<string>>({})
        .append()
        .help("A tag which is used to identify the Zettel.");
    new_cmd.add_argument("--edit").flag();
    new_cmd.add_description("Create a new Zettel.");

    argparse::ArgumentParser test_cmd("test");

    program.add_subparser(init_cmd);
    program.add_subparser(new_cmd);
    program.add_subparser(test_cmd);

    cmd::Context ctx{ get_root_path() };

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& exc) {
        cerr << exc.what() << endl;
        return EXIT_FAILURE;
    }

    try {
        if (program.is_subcommand_used("init")) {
            cmd::initialize(ctx);
            cout << ansi::block("Successfully").foreground(ansi::Color::GREEN).bold(true) << " initialized Zettelkasten at "
                << ansi::block(ctx.root.string()).bold(true).italic(true) << endl;
        } else if (program.is_subcommand_used("new")) {
            cmd::NewOptions opts{
                new_cmd.get<string>("title"),
                new_cmd.get<vector<string>>("--tag"),
                new_cmd.get<bool>("--edit")
            };
            unique_ptr<Zettel> z = cmd::make_new(ctx, opts);
            cout << ansi::block("Created").foreground(ansi::Color::GREEN).bold(true) << " Zettel " << ansi::block(fmt("%u", z->id()))
                << endl;
        } else if (program.is_subcommand_used("test")) {
            unique_ptr<Editor> ed = Editor::getInstance(ctx.root / ".zettel");
            try {
                ed->readInput();
            } catch (const Editor::Exception& exc) {
                cerr << ansi::block("ERROR: ").foreground(ansi::Color::RED).bold(true) << exc.what() << endl;
                return EXIT_FAILURE;
            }
        }
        else throw cmd::CommandException("No command specified.");
    } catch (const std::exception& exc) {
        cerr << ansi::block("ERROR: ").foreground(ansi::Color::RED).bold(true) << exc.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
