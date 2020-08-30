#include "AFFECT_DATA.hpp"

#include "Char.hpp"
#include "merc.h" // TODO remove once bits are more sensible.

void AFFECT_DATA::modify(Char &ch, bool apply) {
    if (apply) {
        SET_BIT(ch.affected_by, bitvector);
    } else {
        REMOVE_BIT(ch.affected_by, bitvector);
    }
    auto mod = apply ? modifier : -modifier;
    switch (location) {
    case APPLY_STR: ch.mod_stat[Stat::Str] += mod; break;
    case APPLY_DEX: ch.mod_stat[Stat::Dex] += mod; break;
    case APPLY_INT: ch.mod_stat[Stat::Int] += mod; break;
    case APPLY_WIS: ch.mod_stat[Stat::Wis] += mod; break;
    case APPLY_CON: ch.mod_stat[Stat::Con] += mod; break;

    case APPLY_SEX: ch.sex += mod; break;
    case APPLY_MANA: ch.max_mana += mod; break;
    case APPLY_HIT: ch.max_hit += mod; break;
    case APPLY_MOVE: ch.max_move += mod; break;

    case APPLY_AC:
        for (auto &ac : ch.armor)
            ac += mod;
        break;
    case APPLY_HITROLL: ch.hitroll += mod; break;
    case APPLY_DAMROLL: ch.damroll += mod; break;

    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_PETRI:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL: ch.saving_throw += mod; break;
    }
}

AFFECT_DATA::Value AFFECT_DATA::worth() const noexcept {
    switch (location) {
    case APPLY_STR:
    case APPLY_DEX:
    case APPLY_INT:
    case APPLY_WIS:
    case APPLY_CON:
        if (modifier > 0)
            return Value{0, 0, modifier};
        break;

    case APPLY_HITROLL: return Value{modifier, 0, 0};
    case APPLY_DAMROLL: return Value{0, modifier, 0};

    case APPLY_SAVING_PARA:
    case APPLY_SAVING_ROD:
    case APPLY_SAVING_PETRI:
    case APPLY_SAVING_BREATH:
    case APPLY_SAVING_SPELL:
    case APPLY_AC:
        if (modifier < 0)
            if (modifier > 0)
                return Value{0, 0, -modifier};
        break;

    case APPLY_MANA:
    case APPLY_HIT:
    case APPLY_MOVE:
        if (modifier > 0)
            return Value{0, 0, (modifier + 6) / 7};
    }
    return {};
}
