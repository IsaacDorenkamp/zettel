#include "editor.hpp"

#include <fstream>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "format.hpp"
#include "io.hpp"
#include "sys.hpp"

using std::filesystem::path, std::ifstream, std::ofstream, std::map, std::optional, std::string, std::unique_ptr, std::vector;

extern char **environ;

namespace zettel {

Editor::Editor(path dotdir) : m_dotdir(dotdir) {}
unique_ptr<Editor> Editor::getInstance(path dotdir) {
    return unique_ptr<Editor>(new TerminalEditor(dotdir, "/bin/sh", {"-c", "vi \"$@\"", "/bin/sh", "<filename>"}));
}

TerminalEditor::TerminalEditor(path dotdir, string executable, const TerminalEditor::Args& args, const TerminalEditor::Env& env) : Editor(dotdir), m_executable(executable), m_args(args), m_env(env) {}
TerminalEditor::TerminalEditor(path dotdir, string executable, const TerminalEditor::Args& args) : TerminalEditor(dotdir, executable, args, {}) {}
TerminalEditor::TerminalEditor(path dotdir, string executable) : TerminalEditor(dotdir, executable, {}) {}

string TerminalEditor::readInput(optional<string> defaultText) {
    path toEdit = m_dotdir / "INPUT";

    // first, attempt to truncate the file
    ofstream truncStr(toEdit, ofstream::trunc);
    if (truncStr.is_open()) {
        if (defaultText) truncStr << *defaultText;
        truncStr.close();
    } else {
        throw Editor::Exception(fmt("Unable to truncate %s", toEdit.c_str()));
    }

    // FIXME: something breaking here
    vector<string> args(m_args);
    /*
    for (vector<string>::iterator it = args.begin(); it != args.end(); it++) {
        //if (it->compare("<filename>") == 0) *it = toEdit.string();
    }
    */
    args.insert(args.begin(), m_executable);

    try {
        sys::ExitResult result = sys::runcmd(m_executable, args, m_env);
        if (result.status == 0) {
            optional<string> content = io::readfile(toEdit);
            if (content) {
                // strip trailing newline
                if (content->size() > 0 && content->back() == '\n') content = content->substr(0, content->size() - 1);
                return *content;
            } else {
                throw Editor::Exception("Could not read the editor file!");
            }
        } else if (result.status > 0) {
            throw Editor::Exception("An error occurred while editing.");
        } else {
            throw Editor::Exception("The editor was terminated via a signal.");
        }
    } catch (const sys::SysException& exc) {
        throw Editor::Exception(fmt("Unable to start editor: %s", exc.what()));
    }
}

}
