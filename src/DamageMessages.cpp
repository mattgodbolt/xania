/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "DamageMessages.hpp"
#include "AttackType.hpp"
#include "Char.hpp"
#include "merc.h"

#include <fmt/format.h>
#include <variant>

namespace {

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
        if (*at == &attack_table[0]) {
            return std::nullopt;
        } else {
            return (*at)->verb;
        }
    } else if (const auto attack_skill = std::get_if<const skill_type *>(&context.atk_type)) {
        return (*attack_skill)->verb;
    } else {
        bug("dam_message: bad attack type, using default attack verb");
        return attack_table[0].verb;
    }
}

constexpr auto DoUnspeakableThings = "do |RUNSPEAKABLE|w things to";
constexpr auto DoesUnspeakableThings = "does |RUNSPEAKABLE|w things to";

} // namespace

ImpactVerbs ImpactVerbs::create(const bool has_attack_verb, const int dam_proportion, const int dam_type) {
    const struct dam_string_type *found_type = nullptr;
    for (auto index = 0; dam_string_table[index].dam_types[DAM_NONE]; index++) {
        if (dam_proportion <= dam_string_table[index].damage_proportion) {
            found_type = &dam_string_table[index];
            break;
        }
    }
    if (!found_type) {
        return ImpactVerbs{has_attack_verb ? DoesUnspeakableThings : DoUnspeakableThings, DoesUnspeakableThings};
    }
    std::string singular, plural;
    if (dam_type > 0 && found_type->dam_types[dam_type]) {
        singular = found_type->dam_types[dam_type];
    } else { // there's no specific noun for the damage type being delivered and the proportion
        singular = found_type->dam_types[0];
    }
    plural = singular;
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
    const auto impact_verbs = ImpactVerbs::create(opt_attack_verb.has_value(), proportion, context.dam_type);
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
