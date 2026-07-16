#include <iostream>

#include "argparse/argparse.hpp"
#include "zettel.hpp"

using namespace std;

void init() {
}

int main(int argc, char **argv) {
    argparse::ArgumentParser program("zettel");

    argparse::ArgumentParser init_cmd("init");
    init_cmd.add_description("Initialize a local Zettelkasten.");

    argparse::ArgumentParser new_cmd("new");
    new_cmd.add_description("Create a new Zettel.");

    program.add_subparser(new_cmd);

    return EXIT_SUCCESS;
}
