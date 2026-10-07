#include "editor.hpp"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <stdio.h>

#include "format.hpp"
#include "io.hpp"

using std::filesystem::path, std::ifstream, std::ofstream, std::optional, std::string, std::unique_ptr, std::vector;

namespace zettel {

Editor::Editor(path dotdir) : m_dotdir(dotdir) {}
unique_ptr<Editor> Editor::getInstance(path dotdir) {
    // TODO: Don't hardcode vi!
    return unique_ptr<Editor>(new TerminalEditor(dotdir, "/bin/sh", {"-c", "vi -f /tmp/zettel/.zettel/INPUT"}));
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
        const char** args = new const char*[m_args.size() + 3];
        size_t index;
        args[0] = m_executable.c_str();
        for (index = 0; index < m_args.size(); index++) {
            args[index + 1] = m_args[index].c_str();
        }
        args[m_args.size() + 1] = toEdit.c_str();
        args[m_args.size() + 2] = nullptr;
        const char** env = new const char*[m_env.size() + 1];
        Env::const_iterator entry = m_env.cbegin();
        // necessary to hold string values in memory
        vector<string> envs;
        for (index = 0; entry != m_env.cend(); entry++, index++) {
            envs.push_back(fmt("%s=%s", entry->first.c_str(), entry->second.c_str()));
            env[index] = envs[index].c_str();
        }
        env[m_env.size() + 1] = nullptr;
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
