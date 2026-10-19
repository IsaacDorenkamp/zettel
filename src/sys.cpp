#include "sys.hpp"

#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "format.hpp"

using std::map, std::string, std::vector;
using zettel::fmt, zettel::URI;

extern char** environ;

namespace sys {

SysException::SysException(SysErrReason reason) : m_reason(reason) {}
const char* SysException::what() const throw() {
    switch (m_reason) {
    case StartFailure:
        return "Unable to start process.";
    }
    return "";
}

#if defined(__linux__) || defined(__APPLE__)

ExitResult runcmd(const string& executable, const vector<string>& args, const map<string, string>& env) {
    pid_t pid = fork();

    if (pid == 0) {
        // we are the child
        char** proc_args = new char*[args.size() + 2];
        size_t index;
        std::string arg;
        proc_args[0] = strdup(executable.c_str());
        for (index = 0; index < args.size(); index++) {
            arg = args[index];
            proc_args[index + 1] = strdup(args[index].c_str());
        }
        proc_args[args.size() + 1] = NULL;

        map<string, string> totalEnv(env);
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
        map<string, string>::const_iterator entry = totalEnv.cbegin();
        string line;
        for (index = 0; entry != totalEnv.cend(); entry++, index++) {
            line = fmt("%s=%s", entry->first.c_str(), entry->second.c_str());
            env[index] = strdup(line.data());
        }
        env[totalEnv.size()] = nullptr;
        execve(executable.c_str(), (char* const*)proc_args, (char* const*)env);
        exit(EXIT_FAILURE);
    } else if (pid == -1) {
        // something went wrong
        throw SysException(sys::StartFailure);
    } else {
        // we are the parent
        int status;
        pid_t finished = waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return {
                WEXITSTATUS(status),
                -1
            };
        } else if (WIFSIGNALED(status)) {
            return {
                -1,
                WTERMSIG(status)
            };
        } else {
            throw SysException(sys::StartFailure);
        }
    }
}

void open(const URI& uri) {
    // TODO: implement
}

#else

ExitResult runcmd(const string& executable, const vector<string>& args, const map<string, string>& env) {
    return {
        -1,
        -1
    };
}

void open(const URI& uri) {
    logs::error("No support for opening URIs.");
}

#endif

}
