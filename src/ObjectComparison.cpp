/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2026 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectComparison.hpp"
#include "AFFECT_DATA.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "common/BitOps.hpp"

#include <array>

namespace ObjectComparison {

namespace {

AffectLocation counted_as(const AffectLocation location) {
    switch (location) {
    case AffectLocation::SavingPara:
    case AffectLocation::SavingRod:
    case AffectLocation::SavingPetri:
    case AffectLocation::SavingBreath:
    case AffectLocation::SavingSpell: return AffectLocation::SavingSpell;
    default: return location;
    }
}

struct StatLine {
    AffectLocation location;
    bool lower_is_better; // saving throws (and armor class modifiers, though AC is scored by armor_score)
    std::string_view better;
    std::string_view worse;
};

// In the order they're shown.
constexpr std::array StatLines{
    StatLine{AffectLocation::Ac, true, "$p would protect you better than $P.", "$p would protect you less than $P."},
    StatLine{AffectLocation::SavingSpell, true, "$p would guard you better against magic.",
             "$p would leave you more open to magic."},
    StatLine{AffectLocation::Hitroll, false, "$p would help you hit more often.", "$p would make you miss more often."},
    StatLine{AffectLocation::Damroll, false, "$p would make you hit harder.", "$p would make your blows weaker."},
    StatLine{AffectLocation::Str, false, "$p would make you stronger.", "$p would make you weaker."},
    StatLine{AffectLocation::Dex, false, "$p would make you more agile.", "$p would make you clumsier."},
    StatLine{AffectLocation::Int, false, "$p would make you cleverer.", "$p would make you duller."},
    StatLine{AffectLocation::Wis, false, "$p would make you wiser.", "$p would make you less wise."},
    StatLine{AffectLocation::Con, false, "$p would make you hardier.", "$p would make you frailer."},
    StatLine{AffectLocation::Hit, false, "$p would make you healthier.", "$p would make you less healthy."},
    StatLine{AffectLocation::Mana, false, "$p would give you more mana.", "$p would give you less mana."},
    StatLine{AffectLocation::Move, false, "$p would give you more stamina.", "$p would give you less stamina."},
};

bool share_a_wear_slot(const Object &a, const Object &b) {
    return (a.wear_flags & b.wear_flags & ~to_int(ObjectWearFlag::Take)) != 0;
}

}

int total_modifier(const Object &obj, const AffectLocation location) {
    const auto wanted = counted_as(location);
    int total = 0;
    const auto add = [&](const AffectList &affects) {
        for (const auto &af : affects)
            if (counted_as(af.location) == wanted)
                total += af.modifier;
    };
    if (!obj.enchanted && obj.objIndex)
        add(obj.objIndex->affected);
    add(obj.affected);
    return total;
}

int weapon_score(const Object &weapon) {
    return (1 + weapon.value[2]) * weapon.value[1] + 2 * total_modifier(weapon, AffectLocation::Damroll);
}

int armor_score(const Object &obj) {
    const auto values = obj.type == ObjectType::Armor ? obj.value[0] + obj.value[1] + obj.value[2] + obj.value[3] : 0;
    return values - 4 * total_modifier(obj, AffectLocation::Ac);
}

Result compare(const Object &a, const Object &b) {
    Result result;
    if (&a == &b) {
        result.lines.emplace_back("You compare $p to itself.  It looks about the same.");
        return result;
    }
    const auto both = [&](const ObjectType type) { return a.type == type && b.type == type; };
    // What the verdict already counts doesn't get a line of its own.
    auto in_verdict = AffectLocation::None;
    if (both(ObjectType::Weapon) || both(ObjectType::Armor)) {
        const auto weapons = both(ObjectType::Weapon);
        const auto score_a = weapons ? weapon_score(a) : armor_score(a);
        const auto score_b = weapons ? weapon_score(b) : armor_score(b);
        result.lines.emplace_back(score_a == score_b  ? "$p and $P look about the same."
                                  : score_a > score_b ? "$p looks better than $P."
                                                      : "$p looks worse than $P.");
        in_verdict = weapons ? AffectLocation::Damroll : AffectLocation::Ac;
    } else if (a.type == ObjectType::Weapon || b.type == ObjectType::Weapon || !share_a_wear_slot(a, b)) {
        result.lines.emplace_back("You can't compare $p and $P.");
        return result;
    }
    for (const auto &stat : StatLines) {
        if (stat.location == in_verdict)
            continue;
        // How much better a is than b: armor class counts the armor's own values too, as the verdict does.
        const auto advantage = stat.location == AffectLocation::Ac
                                   ? armor_score(a) - armor_score(b)
                                   : (total_modifier(a, stat.location) - total_modifier(b, stat.location))
                                         * (stat.lower_is_better ? -1 : 1);
        if (advantage == 0)
            continue;
        result.lines.emplace_back(advantage > 0 ? stat.better : stat.worse);
    }
    if (result.lines.empty())
        result.lines.emplace_back("$p and $P look about the same.");
    return result;
}

}
