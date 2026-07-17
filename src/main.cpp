#include <iostream>

#include "argparse/argparse.hpp"
#include "zettel.hpp"
#include "kasten.hpp"

using namespace std;

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

    zettel::Zettelkasten zk(std::filesystem::current_path());

    if (program.is_subcommand_used("init")) {
        zk.initialize();
    } else {
        string title = new_cmd.get<string>("-t");
        zk.createZettel(title);
    }

    return EXIT_SUCCESS;
}
