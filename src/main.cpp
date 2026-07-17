#include <filesystem>
#include <iostream>

#include "argparse/argparse.hpp"
#include "zettel.hpp"
#include "kasten.hpp"

using namespace std;

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
    new_cmd.add_description("Create a new Zettel.");
    new_cmd.add_argument("-t", "--title")
        .help("The title of the new Zettel.")
        .required();

    program.add_subparser(init_cmd);
    program.add_subparser(new_cmd);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& exc) {
        cerr << exc.what() << endl;
        return EXIT_FAILURE;
    }

    try {
        zettel::Zettelkasten zk(get_root_path());
        if (program.is_subcommand_used("init")) {
            zk.initialize();
        } else {
            zk.load();
            string title = new_cmd.get<string>("-t");
            zettel::Zettel* zettel = zk.createZettel(title);
        }
    } catch (const zettel::ZettelkastenException& exc) {
        cerr << "ERROR: " << exc.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
