#include <filesystem>
#include <iostream>

#include "argparse/argparse.hpp"
#include "ansi.hpp"
#include "zettel.hpp"
#include "kasten.hpp"
#include "io.hpp"

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
    new_cmd.add_argument("-e", "--edit")
        .help("Whether to immediately edit the content of the Zettel.")
        .flag();
    new_cmd.add_argument("--draft")
        .flag();

    argparse::ArgumentParser edit_cmd("edit");
    edit_cmd.add_description("Edit an existing Zettel");
    edit_cmd.add_argument("id")
        .help("The ID of the Zettel to edit.")
        .required();

    argparse::ArgumentParser read_cmd("read");
    read_cmd.add_description("Read a Zettel.");
    read_cmd.add_argument("id")
        .help("The ID of the Zettel to read.")
        .required();

    program.add_subparser(init_cmd);
    program.add_subparser(new_cmd);
    program.add_subparser(edit_cmd);
    program.add_subparser(read_cmd);

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
            cout << ansi::block("Initialized").foreground(ansi::Color::GREEN).bold(true) << " workspace at " << ansi::block(zk.root().string()).italic(true) << endl;
        } else if (program.is_subcommand_used("new")) {
            zk.load();
            string title = new_cmd.get<string>("-t");

            zettel::Zettel* zettel;
            bool isDraft = new_cmd.get<bool>("--draft");
            if (isDraft) {
                zettel = zk.draftZettel(title);
            } else {
                throw zettel::ZettelkastenException("Creating non-draft Zettels is not supported yet.");
            }
            if (new_cmd.get<bool>("-e")) {
                filesystem::path contentFile = zk.edit();
                string content = zettel::io::readfile(contentFile);
                zettel->clearContent();
                zettel->addContentBlock(unique_ptr<zettel::ContentBlock>(new zettel::TextBlock(zettel::NumericId(0), content)));
                zettel->save();
            }
            ansi::block block(zettel->id().represent());
            if (isDraft) {
                block.setText(zettel::fmt("Draft #%s", zettel->id().represent().c_str())).italic(true);
            } else {
                block.setText(zettel->id().represent()).italic(true);
            }
            cout << ansi::block("Created").foreground(ansi::Color::GREEN).bold(true) << " " << block << endl;
        } else if (program.is_subcommand_used("edit")) {
            zk.load();
            string id = edit_cmd.get<string>("id");
            unique_ptr<zettel::Id> parsed;
            try {
                parsed = zk.parseId(id);
            } catch (const zettel::Id::Exception& exc) {
                throw zettel::ZettelkastenException(zettel::fmt("Invalid ID '%s'. %s", id.c_str(), exc.what()));
            }
            zk.editZettel(*parsed);
        } else if (program.is_subcommand_used("read")) {
            zk.load();
            string id = read_cmd.get<string>("id");
            unique_ptr<zettel::Id> parsed;
            try {
                parsed = zk.parseId(id);
            } catch (const zettel::Id::Exception& exc) {
                throw zettel::ZettelkastenException(zettel::fmt("Invalid ID '%s'. %s", id.c_str(), exc.what()));
            }
            const zettel::Zettel* note = zk.getZettelById(*parsed);

            if (!note) {
                throw zettel::ZettelkastenException(zettel::fmt("No note with ID %s", parsed->represent().c_str()));
            }

            zettel::FormatOptions options{
                .mode = zettel::DisplayMode::ANSI,
                .line_size = 75,
                .first_line_offset = 0
            };
            
            // print Zettel
            cout << ansi::block(note->title()).bold(true) << " (" << ansi::block(note->id().represent()).italic(true) << ")" << endl;
            cout << endl;

            vector<string> lines;
            for (const unique_ptr<zettel::ContentBlock>& block : note->content()) {
                lines = block->format(options);
                if (lines.empty()) continue;
                bool first = true;
                for (const string& line : lines) {
                    if (first) first = false;
                    else cout << endl;
                    cout << line;
                }
                if (lines.back().size() == options.line_size) {
                    cout << endl;
                } else {
                    options.first_line_offset = options.line_size - lines.back().size();
                }
            }
        } else {
            cout << program.usage() << endl;
            return EXIT_FAILURE;
        }
    } catch (const zettel::ZettelkastenException& exc) {
        cerr << ansi::block("ERROR: ").foreground(ansi::Color::RED).bold(true) << exc.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
