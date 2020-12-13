#include "Pronouns.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "string_utils.hpp"

const Pronouns &pronouns_for(int sex) { // TODO: strong type for sex.
    static Pronouns male{"his", "him", "he", "himself"};
    static Pronouns female{"her", "her", "she", "herself"};
    static Pronouns neutral{"their", "them", "they", "themself"};
    switch (sex) {
    case 0: return neutral;
    case 1: return male;
    case 2: return female;
    default: break;
    }
    // TODO: reintroduce? throw? out of line?
    //    bug("{}", fmt::format("Bad sex {} in pronouns ", ch->sex).c_str());
    return neutral;
}

const std::pair<const Pronouns &, const Pronouns &> pronouns_for(const Char &ch) {
    const Pronouns &standard = pronouns_for(ch.sex);
    const Pronouns &maybe_custom = ch.is_pc() ? ch.pcdata.get()->pronouns : standard;
    return {maybe_custom, standard};
}

const std::string &possessive(const Char &ch) {
    const auto pronouns = pronouns_for(ch);
    return !pronouns.first.possessive.empty() ? pronouns.first.possessive : pronouns.second.possessive;
}

const std::string &subjective(const Char &ch) {
    const auto pronouns = pronouns_for(ch);
    return !pronouns.first.subjective.empty() ? pronouns.first.subjective : pronouns.second.subjective;
}

const std::string &objective(const Char &ch) {
    const auto pronouns = pronouns_for(ch);
    return !pronouns.first.objective.empty() ? pronouns.first.objective : pronouns.second.objective;
}

const std::string &reflexive(const Char &ch) {
    const auto pronouns = pronouns_for(ch);
    return !pronouns.first.reflexive.empty() ? pronouns.first.reflexive : pronouns.second.reflexive;
}

void pronouns_syntax(Char *ch) {
    ch->send_line("To configure:");
    ch->send_line("      pronouns set <possessive> <objective> <subjective> <reflexive>");
    ch->send_line("Examples:");
    ch->send_line("      pronouns set his him he himself");
    ch->send_line("      pronouns set hir hir ze hirself");
    ch->send_line("Reset to the defaults:");
    ch->send_line("      pronouns reset");
    ch->send_line("Show your pronouns:");
    ch->send_line("      pronouns show");
    ch->send_line(
        "Custom pronouns disregard your character's in-game sex, even if it changes due to a magical episode!");
}

PronounParseResult parse_pronouns(ArgParser args) {
    static PronounParseResult EMPTY_ARGS = {"", "", "", "", PronounParseState::EmptyArgs};
    if (args.empty()) {
        return EMPTY_ARGS;
    }
    PronounParseResult res;
    res.parsed.possessive = smash_tilde(args.shift());
    res.parsed.objective = smash_tilde(args.shift());
    res.parsed.subjective = smash_tilde(args.shift());
    res.parsed.reflexive = smash_tilde(args.shift());
    if (res.parsed.possessive.empty() || res.parsed.objective.empty() || res.parsed.subjective.empty()
        || res.parsed.reflexive.empty()) {
        res.state = PronounParseState::MissingArgs;
    } else {
        res.state = PronounParseState::Ok;
    }
    return res;
}

void do_pronouns(Char *ch, ArgParser args) {
    if (ch->is_npc()) {
        ch->send_line("Sorry, only players can do that.");
        return;
    }
    if (args.empty()) {
        pronouns_syntax(ch);
        return;
    }
    auto cmd = args.shift();
    if (matches(cmd, "set")) {
        const auto res = parse_pronouns(args);
        if (res.state != PronounParseState::Ok) {
            ch->send_line("pronouns set requires a value for the possessive, objective, subject and reflexive terms.");
            return;
        }
        ch->pcdata.get()->pronouns.possessive = res.parsed.possessive;
        ch->pcdata.get()->pronouns.objective = res.parsed.objective;
        ch->pcdata.get()->pronouns.subjective = res.parsed.subjective;
        ch->pcdata.get()->pronouns.reflexive = res.parsed.reflexive;
        ch->send_line("Pronouns set.");
        ch->send_line("Skills, spells and built-in socials will use them in messages to other players.");
    } else if (matches(cmd, "reset")) {
        ch->pcdata.get()->pronouns.possessive = "";
        ch->pcdata.get()->pronouns.objective = "";
        ch->pcdata.get()->pronouns.subjective = "";
        ch->pcdata.get()->pronouns.reflexive = "";
        ch->send_line("Pronouns reset to the defaults.");
    } else if (matches(cmd, "show")) {
        const auto pronouns = pronouns_for(*ch);
        ch->send_line("           Standard   ||  Your Choice  (if set)");
        ch->send_line("----------------------------------------------------");
        ch->send_line("Possessive {:<11}||  {}", pronouns.second.possessive, pronouns.first.possessive);
        ch->send_line("Objective  {:<11}||  {}", pronouns.second.objective, pronouns.first.objective);
        ch->send_line("Subjective {:<11}||  {}", pronouns.second.subjective, pronouns.first.subjective);
        ch->send_line("Reflexive  {:<11}||  {}", pronouns.second.reflexive, pronouns.first.reflexive);
    } else {
        pronouns_syntax(ch);
    }
}
