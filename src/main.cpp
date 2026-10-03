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

    argparse::ArgumentParser init_cmd("init");
    init_cmd.add_description("Initialize a local Zettelkasten.");

    program.add_subparser(init_cmd);

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
        }
        else throw cmd::CommandException("No command specified.");
    } catch (const std::exception& exc) {
        cerr << ansi::block("ERROR: ").foreground(ansi::Color::RED).bold(true) << exc.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
