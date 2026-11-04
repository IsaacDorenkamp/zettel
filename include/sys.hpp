#pragma once

#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "uri.hpp"

namespace sys {

enum SysErrReason {
    StartFailure,
    UnsupportedAction
};

struct ExitResult {
    int status;
    int signal;
};

class SysException : public std::exception {
public:
    SysException(SysErrReason reason);
    const char* what() const throw();
private:
    SysErrReason m_reason;
};

void settermhandler(const std::function<void(int)>& handler);
ExitResult runcmd(const std::string& executable, const std::vector<std::string>& args, const std::map<std::string, std::string>& env);
ExitResult open(const zettel::URI& uri);

}
