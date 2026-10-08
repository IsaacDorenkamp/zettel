#include "editor.hpp"

#include <fstream>
#include <unistd.h>
#include <stdio.h>

#include "format.hpp"
#include "io.hpp"

using std::filesystem::path, std::ifstream, std::ofstream, std::map, std::optional, std::string, std::unique_ptr;

extern char **environ;

namespace zettel {

Editor::Editor(path dotdir) : m_dotdir(dotdir) {}
unique_ptr<Editor> Editor::getInstance(path dotdir) {
    return unique_ptr<Editor>(new TerminalEditor(dotdir, "/bin/sh", {"-c", "vi \"$@\"", "<filename>"}));
}

TerminalEditor::TerminalEditor(path dotdir, string executable, const TerminalEditor::Args& args, const TerminalEditor::Env& env) : Editor(dotdir), m_executable(executable), m_args(args), m_env(env) {}
TerminalEditor::TerminalEditor(path dotdir, string executable, const TerminalEditor::Args& args) : TerminalEditor(dotdir, executable, args, {}) {}
TerminalEditor::TerminalEditor(path dotdir, string executable) : TerminalEditor(dotdir, executable, {}) {}

optional<string> TerminalEditor::readInput() {
    path toEdit = m_dotdir / "INPUT";

    // first, attempt to truncate the file
    ofstream truncStr(toEdit, ofstream::trunc);
    if (truncStr.is_open()) truncStr.close();

    pid_t pid = fork();

    if (pid == 0) {
        // we are the child
        char** args = new char*[m_args.size() + 2];
        size_t index;
        std::string arg;
        args[0] = strdup(m_executable.c_str());
        for (index = 0; index < m_args.size(); index++) {
            arg = m_args[index];
            if (arg.compare("<filename>") == 0) {
                args[index + 1] = strdup(toEdit.c_str());
            } else {
                args[index + 1] = strdup(m_args[index].c_str());
            }
        }
        args[m_args.size() + 1] = NULL;
        map<string, string> totalEnv(m_env);
        size_t nameSize;
        string name;
        for (char** environVar = environ; *environVar != NULL; environVar++) {
            char* eq = strchr(*environVar, '=');
            if (eq == NULL) continue;
            nameSize = (size_t)(eq - *environVar);
            name = string(*environVar, nameSize);
            totalEnv.insert({ name, eq + 1 });
        }
        char** env = new char*[totalEnv.size() + 1];
        Env::const_iterator entry = totalEnv.cbegin();
        string line;
        for (index = 0; entry != totalEnv.cend(); entry++, index++) {
            line = fmt("%s=%s", entry->first.c_str(), entry->second.c_str());
            env[index] = strdup(line.data());
        }
        env[totalEnv.size()] = nullptr;
        execve(m_executable.c_str(), (char* const*)args, (char* const*)env);
        exit(EXIT_FAILURE);
    } else if (pid == -1) {
        // something went wrong
        return std::nullopt;
    } else {
        // we are the parent

        int status;
        pid_t finished = waitpid(pid, &status, 0);
        if (finished == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // read file and return contents
            return io::readfile(toEdit);
        } else return std::nullopt;
    }
}

}
