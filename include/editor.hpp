#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "exception.hpp"

namespace zettel {

class Editor {
public:
    DEFINE_EXCEPTION;
    Editor(std::filesystem::path dotdir);
    virtual ~Editor() = default;
    virtual std::string readInput(std::optional<std::string> defaultText = std::nullopt) = 0;

    static std::unique_ptr<Editor> getInstance(std::filesystem::path dotdir);
protected:
    std::filesystem::path m_dotdir;
};

class TerminalEditor : public Editor {
public:
    using Env = std::map<std::string, std::string>;
    using Args = std::vector<std::string>;

    TerminalEditor(std::filesystem::path dotdir, std::string executable, const Args& args, const Env& env);
    TerminalEditor(std::filesystem::path dotdir, std::string executable, const Args& args);
    TerminalEditor(std::filesystem::path dotdir, std::string executable);
    virtual ~TerminalEditor() = default;

    virtual std::string readInput(std::optional<std::string> defaultText = std::nullopt);
private:
    std::string m_executable;
    Args m_args;
    Env m_env;
};

}
