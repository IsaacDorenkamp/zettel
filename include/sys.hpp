#pragma once

#include <exception>
#include <map>
#include <string>
#include <vector>

#include "uri.hpp"

namespace sys {

enum SysErrReason {
    StartFailure
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

ExitResult runcmd(const std::string& executable, const std::vector<std::string>& args, const std::map<std::string, std::string>& env);
ExitResult open(const zettel::URI& uri);

}
