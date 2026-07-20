#include "sys.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <cstdio>

namespace zettel {

namespace sys {

int spawn(const char* executable, char* const argv[], char* const env[]) {
#ifndef WIN32
    using std::cout, std::endl;
    pid_t new_proc = fork();

    if (new_proc == 0) {
        // is child
        cout << "I am the child!" << endl;
        int result = execve(executable, argv, env);
        if (result == -1) {
            perror("Failed to spawn:");
            exit(EXIT_FAILURE);
        } else exit(EXIT_SUCCESS);
    } else if (new_proc == -1) {
        // FAILURE!
        return -1;
    } else {
        // is parent process
        cout << "Child proc: " << new_proc << endl;
        int status;
        waitpid(new_proc, &status, 0);
        return WEXITSTATUS(status);
    }
#else
    return -1;
#endif
}

}

}
