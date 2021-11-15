/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageMessages.hpp"
#include "Attacks.hpp"
#include "Char.hpp"
#include "DamageType.hpp"
#include "Logging.hpp"
#include "SkillTables.hpp"

#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <unordered_map>
#include <variant>

namespace {

struct dam_proportion_verbs {
    const int damage_proportion;
    const std::unordered_map<const DamageType, std::string_view> verbs;
};

const std::array<struct dam_proportion_verbs, 27> dam_proportion_verbs_table = {
    {{0, {{DamageType::None, "miss"}}},
     {1,
      {{DamageType::None, "scrape"},
       {DamageType::Fire, "touch"},
       {DamageType::Lightning, "touch"},
       {DamageType::Acid, "brush"},
       {DamageType::Negative, "taint"},
       {DamageType::Holy, "touch"}}},
     {2, {{DamageType::None, "scratch"}}},
     {3,
      {{DamageType::None, "bruise"},
       {DamageType::Fire, "hurt"},
       {DamageType::Cold, "hurt"},
       {DamageType::Lightning, "hurt"},
       {DamageType::Acid, "hurt"},
       {DamageType::Poison, "sicken"},
       {DamageType::Negative, "entangle"}}},
     {4,
      {{DamageType::None, "graze"},
       {DamageType::Fire, "sting"},
       {DamageType::Cold, "cool"},
       {DamageType::Lightning, "jolt"},
       {DamageType::Acid, "gnaw"},
       {DamageType::Poison, "nauseate"},
       {DamageType::Negative, "stir"},
       {DamageType::Holy, "dismiss"}}},
     {5,
      {{DamageType::None, "hit"},
       {DamageType::Fire, "scald"},
       {DamageType::Cold, "chill"},
       {DamageType::Lightning, "zap"},
       {DamageType::Acid, "sting"},
       {DamageType::Poison, "ail"},
       {DamageType::Negative, "smolder"}}},
     {6,
      {{DamageType::None, "cut"},
       {DamageType::Fire, "blister"},
       {DamageType::Cold, "ice"},
       {DamageType::Lightning, "shock"},
       {DamageType::Acid, "blister"},
       {DamageType::Poison, "intoxicate"},
       {DamageType::Negative, "torment"},
       {DamageType::Holy, "pain"}}},
     {7,
      {{DamageType::None, "injure"},
       {DamageType::Fire, "burn"},
       {DamageType::Cold, "frost"},
       {DamageType::Lightning, "stun"},
       {DamageType::Acid, "scald"},
       {DamageType::Poison, "contaminate"},
       {DamageType::Negative, "torment"},
       {DamageType::Holy, "palpitate"}}},
     {8,
      {{DamageType::None, "wound"},
       {DamageType::Fire, "torch"},
       {DamageType::Cold, "freeze"},
       {DamageType::Lightning, "electrify"},
       {DamageType::Acid, "boil"},
       {DamageType::Poison, "corrupt"},
       {DamageType::Negative, "torture"},
       {DamageType::Holy, "torture"}}},
     {10,
      {{DamageType::None, "split"},
       {DamageType::Fire, "sizzle"},
       {DamageType::Cold, "solidify"},
       {DamageType::Lightning, "electrocute"},
       {DamageType::Acid, "combust"},
       {DamageType::Poison, "defile"},
       {DamageType::Negative, "agonize"},
       {DamageType::Holy, "agonize"}}},
     {12,
      {{DamageType::None, "gash"},
       {DamageType::Fire, "frazzle"},
       {DamageType::Cold, "crackle"},
       {DamageType::Lightning, "frazzle"},
       {DamageType::Acid, "deteriorate"},
       {DamageType::Poison, "defile"},
       {DamageType::Negative, "crush"},
       {DamageType::Holy, "purge"}}},
     {14,
      {{DamageType::None, "shred"},
       {DamageType::Fire, "scald"},
       {DamageType::Cold, "shatter"},
       {DamageType::Lightning, "crisp"},
       {DamageType::Acid, "perforate"},
       {DamageType::Poison, "befoul"},
       {DamageType::Negative, "defile"},
       {DamageType::Holy, "cleanse"}}},
     {16,
      {{DamageType::None, "maul"},
       {DamageType::Fire, "bake"},
       {DamageType::Cold, "crystalize"},
       {DamageType::Lightning, "split"},
       {DamageType::Acid, "dissipate"},
       {DamageType::Poison, "corrupt"},
       {DamageType::Negative, "smash"},
       {DamageType::Holy, "pacify"}}},
     {18,
      {{DamageType::None, "decimate"},
       {DamageType::Fire, "grill"},
       {DamageType::Lightning, "scorch"},
       {DamageType::Acid, "splutter"},
       {DamageType::Negative, "blacken"},
       {DamageType::Holy, "purify"}}},
     {22,
      {{DamageType::None, "eviscerate"},
       {DamageType::Fire, "roast"},
       {DamageType::Lightning, "sear"},
       {DamageType::Acid, "frizzle"},
       {DamageType::Negative, "curse"},
       {DamageType::Holy, "interrogate"}}},
     {26,
      {{DamageType::None, "devastate"},
       {DamageType::Fire, "sear"},
       {DamageType::Lightning, "cook"},
       {DamageType::Acid, "splatter"},
       {DamageType::Negative, "ruin"},
       {DamageType::Holy, "redeem"}}},
     {30,
      {{DamageType::None, "maim"},
       {DamageType::Fire, "char"},
       {DamageType::Lightning, "roast"},
       {DamageType::Acid, "sear"},
       {DamageType::Negative, "enslave"},
       {DamageType::Holy, "sanctify"}}},
     {35,
      {{DamageType::None, "MUTILATE"},
       {DamageType::Fire, "BRAND"},
       {DamageType::Cold, "FREEZE"},
       {DamageType::Lightning, "ELECTRIFY"},
       {DamageType::Acid, "DISSOLVE"},
       {DamageType::Holy, "VITRIFY"}}},
     {40,
      {{DamageType::None, "DISEMBOWEL"},
       {DamageType::Fire, "MELT"},
       {DamageType::Lightning, "ELECTROCUTE"},
       {DamageType::Acid, "MELT"},
       {DamageType::Negative, "PURGE"},
       {DamageType::Holy, "PURGE"}}},
     {45,
      {{DamageType::None, "DISMEMBER"},
       {DamageType::Fire, "ENGULF"},
       {DamageType::Lightning, "CAUTERIZE"},
       {DamageType::Acid, "DECOMPOSE"},
       {DamageType::Negative, "CRUCIFY"},
       {DamageType::Holy, "CRUCIFY"}}},
     {50,
      {{DamageType::None, "MASSACRE"},
       {DamageType::Fire, "NUKE"},
       {DamageType::Lightning, "NUKE"},
       {DamageType::Acid, "DISINTEGRATE"},
       {DamageType::Negative, "CONDEMN"},
       {DamageType::Holy, "CONDEMN"}}},
     {56, {{DamageType::None, "MANGLE"}, {DamageType::Fire, "ATOMIZE"}, {DamageType::Acid, "ATOMIZE"}}},
     {62, {{DamageType::None, "DEMOLISH"}}},
     {70, {{DamageType::None, "DEVASTATE"}}},
     {80, {{DamageType::None, "OBLITERATE"}}},
     {90, {{DamageType::None, "ANNIHILATE"}}},
     {120, {{DamageType::None, "ERADICATE"}}}}};

int calc_dam_proportion(const Char *ch, const Char *victim, const int damage, Rng &rng) {
    int proportion = 0;
    if (damage != 0) { /* don't do any funny business if we have missed them! */
        if (victim->max_hit != 0)
            proportion = (damage * 100) / victim->max_hit; /* proportionate dam */
        else
            proportion = (damage * 100) / 1;
        proportion += rng.number_range(0, ch->level / 5);
    }
    return proportion;
}

std::pair<std::string, std::string> make_prefix_suffix(const int proportion) {
    if (proportion >= 0 && proportion <= 12) {
        return {"|w", "|w"};
    } else if (proportion >= 16 && proportion <= 30) {
        return {"|G", "|w"};
    } else if (proportion >= 31 && proportion <= 61) {
        return {"|Y", "|w"};
    } else if (proportion >= 62 && proportion <= 80) {
        return {"|W***  ", "  ***|w"};
    } else if (proportion >= 81 && proportion <= 120) {
        return {"|R<<<  ", "  >>>|w"};
    } else {
        return {"|w", "|w"};
    }
}

std::string make_damage_amount_label(const Char *ch, const int damage) {
    return damage > 0 && ch->level >= 20 ? fmt::format(" ({})", damage) : "";
}

std::optional<std::string_view> get_attack_verb(const DamageContext &context) {
    if (const auto at = std::get_if<const attack_type *>(&context.atk_type)) {
        // The first entry in the attack table represents the raw notion of being
        // hit by _something_ unspecific, so we generate a damage message containing no attack verb.
        if (*at == Attacks::at(0)) {
            return std::nullopt;
        } else {
            return (*at)->verb;
        }
    } else if (const auto attack_skill = std::get_if<const skill_type *>(&context.atk_type)) {
        return (*attack_skill)->verb;
    } else {
        bug("dam_message: bad attack type, using default attack verb");
        return Attacks::at(0)->verb;
    }
}

constexpr auto DoUnspeakableThings = "do |RUNSPEAKABLE|w things to";
constexpr auto DoesUnspeakableThings = "does |RUNSPEAKABLE|w things to";

} // namespace

ImpactVerbs ImpactVerbs::create(const bool has_attack_verb, const int dam_proportion, const DamageType damage_type) {

    const auto proportion_verbs = std::find_if(
        dam_proportion_verbs_table.begin(), dam_proportion_verbs_table.end(),
        [&dam_proportion](const struct dam_proportion_verbs &e) { return dam_proportion <= e.damage_proportion; });
    if (proportion_verbs == std::end(dam_proportion_verbs_table)) {
        return ImpactVerbs{has_attack_verb ? DoesUnspeakableThings : DoUnspeakableThings, DoesUnspeakableThings};
    }
    std::string singular;
    const auto &verbs = proportion_verbs->verbs;
    const auto damage_verb = verbs.find(damage_type);
    if (damage_verb != verbs.end() && damage_type > DamageType::None) {
        singular = damage_verb->second;
    } else { // there's no specific noun for the damage type being delivered and the proportion
        singular = verbs.at(DamageType::None);
    }
    std::string plural = singular;
    // Pluralize it
    const auto last = std::prev(plural.end());
    switch (*last) {
    case 'y':
        plural.pop_back();
        plural += "ies";
        break;
    case 'Y':
        plural.pop_back();
        plural += "IES";
        break;
    case 'h':
    case 's':
    case 'x': plural += "es"; break;
    case 'H':
    case 'S':
    case 'X': plural += "ES"; break;
    default: plural += std::isupper(*last) ? "S" : "s";
    }
    return {singular, plural};
}

ImpactVerbs::ImpactVerbs(const std::string singular, const std::string plural) : singular_(singular), plural_(plural) {}

std::string_view ImpactVerbs::singular() const { return singular_; }

std::string_view ImpactVerbs::plural() const { return plural_; }

DamageMessages DamageMessages::create(const Char *ch, const Char *victim, const DamageContext &context, Rng &rng) {
    std::string to_char, to_victim, to_room;
    const auto proportion = calc_dam_proportion(ch, victim, context.damage, rng);
    const auto punct = (proportion <= 20) ? '.' : '!';
    const auto pref_suff = make_prefix_suffix(proportion);
    const auto opt_attack_verb = get_attack_verb(context);
    const auto impact_verbs = ImpactVerbs::create(opt_attack_verb.has_value(), proportion, context.damage_type);
    const auto ch_dam_label = make_damage_amount_label(ch, context.damage);
    const auto vict_dam_label = make_damage_amount_label(victim, context.damage);

    if (!opt_attack_verb) {
        if (ch == victim) {
            to_char = fmt::format("You {}{}{} your own {}{}|w{}", pref_suff.first, impact_verbs.singular(),
                                  pref_suff.second, context.injured_part.description, punct, ch_dam_label);
            to_room = fmt::format("$n {}{}{} $s {}{}|w", pref_suff.first, impact_verbs.plural(), pref_suff.second,
                                  context.injured_part.description, punct);
        } else {
            to_char = fmt::format("You {}{}{} $N's {}{}|w{}", pref_suff.first, impact_verbs.singular(),
                                  pref_suff.second, context.injured_part.description, punct, ch_dam_label);
            to_victim = fmt::format("$n {}{}{} your {}{}|w{}", pref_suff.first, impact_verbs.plural(), pref_suff.second,
                                    context.injured_part.description, punct, vict_dam_label);
            to_room = fmt::format("$n {}{}{} $N's {}{}|w", pref_suff.first, impact_verbs.plural(), pref_suff.second,
                                  context.injured_part.description, punct);
        }
        return {to_char, to_victim, to_room};
    }
    if (context.immune) {
        if (ch == victim) {
            to_char = fmt::format("Luckily, you are immune to that.|w");
            to_room = fmt::format("$n is |Wunaffected|w by $s own {}.|w", *opt_attack_verb);
        } else {
            to_char = fmt::format("$N is |Wunaffected|w by your {}!|w", *opt_attack_verb);
            to_victim = fmt::format("$n's {} is powerless against you.|w", *opt_attack_verb);
            to_room = fmt::format("$N is |Wunaffected|w by $n's {}.|w", *opt_attack_verb);
        }
    } else {
        if (ch == victim) {
            to_char = fmt::format("Your {} {}{}{} you{}|w{}", *opt_attack_verb, pref_suff.first, impact_verbs.plural(),
                                  pref_suff.second, punct, ch_dam_label);
            to_room = fmt::format("$n's {} {}{}{} $m{}|w", *opt_attack_verb, pref_suff.first, impact_verbs.plural(),
                                  pref_suff.second, punct);
        } else {
            to_char =
                fmt::format("Your {} {}{}{} $N's {}{}|w{}", *opt_attack_verb, pref_suff.first, impact_verbs.plural(),
                            pref_suff.second, context.injured_part.description, punct, ch_dam_label);
            to_victim =
                fmt::format("$n's {} {}{}{} your {}{}|w{}", *opt_attack_verb, pref_suff.first, impact_verbs.plural(),
                            pref_suff.second, context.injured_part.description, punct, vict_dam_label);
            to_room = fmt::format("$n's {} {}{}{} $N's {}{}|w", *opt_attack_verb, pref_suff.first,
                                  impact_verbs.plural(), pref_suff.second, context.injured_part.description, punct);
        }
    }
    return {to_char, to_victim, to_room};
}

DamageMessages::DamageMessages(const std::string to_char, const std::string to_victim, const std::string to_room)
    : to_char_(to_char), to_victim_(to_victim), to_room_(to_room) {}

std::string_view DamageMessages::to_char() const { return to_char_; }

std::string_view DamageMessages::to_victim() const { return to_victim_; }

std::string_view DamageMessages::to_room() const { return to_room_; }
