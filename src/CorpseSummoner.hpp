#pragma once

#include "Act.hpp"
#include "Char.hpp"
#include "db.h"
#include <chrono>
#include <vector>

using namespace std::literals;

class CorpseSummoner {
public:
    // Dependencies on static functions and variables in the mud.
    struct Dependencies {
        virtual ~Dependencies() = default;
        virtual void interpret(Char *, std::string_view) = 0;
        virtual void act(std::string_view, const Char *, Act1Arg, Act2Arg, const To, const MobTrig,
                         const Position::Type) = 0;
        virtual void act(std::string_view, const Char *, Act1Arg, Act2Arg, const To) = 0;
        virtual void obj_from_char(Object *) = 0;
        virtual void obj_to_char(Object *, Char *) = 0;
        virtual void obj_from_room(Object *) = 0;
        virtual void obj_to_room(Object *, Room *) = 0;
        virtual void extract_obj(Object *) = 0;
        virtual void affect_to_char(Char *, const AFFECT_DATA &paf) = 0;
        virtual std::vector<std::unique_ptr<Object>> &object_list() = 0;
        virtual SpecialFunc spec_fun_summoner() const = 0;
        virtual int weaken_sn() const = 0;
    };

    explicit CorpseSummoner(Dependencies &dependencies);

    /**
     * Find a player's corpse and teleport it into the room. Consumes the catalyst used to trigger.
     */
    void SummonCorpse(Char *player, Char *summoner, Object *catalyst);

    void summoner_awaits(Char *ch, const time_t time_secs);

    bool check_summoner_preconditions(Char *player, Char *summoner);

    std::optional<std::string_view> is_catalyst_invalid(Char *player, Object *catalyst);

    bool check_catalyst(Char *player, Char *summoner, Object *catalyst);

    /**
     * Find a player corpse in the world.
     * Similar to get_obj_world(), it ignores corpses in ch's current room as well as object visibility and it
     * matches on short description because corpse names (keywords) don't include the
     * corpse owner's name.
     */
    std::optional<Object *> get_pc_corpse_world(Char *ch, std::string_view corpse_short_descr);

private:
    void apply_summoning_fatigue(Char *player);

    static inline constexpr auto object_wrong_type{"Sorry, this item cannot be used to summon your corpse."sv};
    static inline constexpr auto object_too_weak{"Sorry, this item is not powerful enough to summon your corpse."sv};
    static inline constexpr auto ShardLevelRange{9};

    Dependencies &mud_;
    time_t last_advice_time_;
};
