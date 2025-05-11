/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#pragma once

#include "ArgParser.hpp"
#include "CommandSet.hpp"
#include "Interpreter.hpp"
#include "Position.hpp"
#include "Types.hpp"

#include <functional>
#include <memory>
#include <string>

struct Char;
struct Room;

enum class CommandLogLevel { Normal, Always, Never };

// Function object for commands run by the interpreter, the do_ functions.
using CommandFunc = std::function<void(Char *ch, std::string_view argument)>;
using CommandFuncNoArgs = std::function<void(Char *ch)>;
using CommandFuncArgParser = std::function<void(Char *ch, ArgParser)>;

struct CommandInfo {
    CommandFunc do_fun;
    Position::Type position;
    sh_int level;
    CommandLogLevel log;
    bool show;

    CommandInfo(CommandFunc do_fun, const Position::Type position, sh_int level, CommandLogLevel log, bool show)
        : do_fun(std::move(do_fun)), position(position), level(level), log(log), show(show) {}
};

//  Interpreter internals
//  Client code should include Interpreter.hpp instead of this file.
struct Interpreter::Impl {
    Impl();
    void add_command(const char *name, CommandFunc do_fun, const Position::Type position = Position::Type::Dead,
                     sh_int level = 0, CommandLogLevel log = CommandLogLevel::Normal, bool show = true);

    void add_command(const char *name, CommandFuncNoArgs do_fun, const Position::Type position = Position::Type::Dead,
                     sh_int level = 0, CommandLogLevel log = CommandLogLevel::Normal, bool show = true);
    void add_command(const char *name, CommandFuncArgParser do_fun,
                     const Position::Type position = Position::Type::Dead, sh_int level = 0,
                     CommandLogLevel log = CommandLogLevel::Normal, bool show = true);
    std::string apply_prefix(Char *ch, std::string_view command) const;
    bool check_social(Char *ch, std::string_view command, std::string_view argument) const;
    void interpret(Char *ch, std::string_view argument) const;
    void show_commands(Char *ch, const int min_level, const int max_level) const;
    CommandSet<CommandInfo> commands;
};
