#pragma once

#include "ArgParser.hpp"
#include "Types.hpp"

#include <string>

struct Pronouns {
    std::string possessive;
    std::string objective;
    std::string subjective;
    std::string reflexive;
};
const std::pair<const Pronouns &, const Pronouns &> pronouns_for(const Char &ch);
const std::string &possessive(const Char &ch);
const std::string &subjective(const Char &ch);
const std::string &objective(const Char &ch);
const std::string &reflexive(const Char &ch);

enum class PronounParseState { Ok, EmptyArgs, MissingArgs };

struct PronounParseResult {
    Pronouns parsed;
    PronounParseState state;
};

PronounParseResult parse_pronouns(ArgParser args);
void do_pronouns(Char *ch, ArgParser args);
