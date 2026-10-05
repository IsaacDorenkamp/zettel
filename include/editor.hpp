#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace zettel {

class Editor {
public:
    Editor(std::filesystem::path dotdir);
    virtual std::optional<std::string> readInput() = 0;

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

    virtual std::optional<std::string> readInput();
private:
    std::string m_executable;
    Args m_args;
    Env m_env;
};

}
