#pragma once

#include "Descriptor.hpp"
#include "ExtraFlags.hpp"
#include "PC_DATA.hpp"
#include "Stats.hpp"
#include "Types.hpp"

#include <memory>

struct AFFECT_DATA;
struct MOB_INDEX_DATA;
struct NOTE_DATA;
struct OBJ_DATA;
struct ROOM_INDEX_DATA;
struct GEN_DATA;
struct MPROG_ACT_LIST;

/*
 * One character (PC or NPC).
 */
struct CHAR_DATA {
    CHAR_DATA *next{};
    CHAR_DATA *next_in_room{};
    CHAR_DATA *master{};
    CHAR_DATA *leader{};
    CHAR_DATA *fighting{};
    CHAR_DATA *reply{};
    CHAR_DATA *pet{};
    CHAR_DATA *riding{};
    CHAR_DATA *ridden_by{};
    SpecialFunc spec_fun{};
    MOB_INDEX_DATA *pIndexData{};
    Descriptor *desc{};
    AFFECT_DATA *affected{};
    NOTE_DATA *pnote{};
    OBJ_DATA *carrying{};
    ROOM_INDEX_DATA *in_room{};
    ROOM_INDEX_DATA *was_in_room{};
    std::unique_ptr<PC_DATA> pcdata;
    GEN_DATA *gen_data{};
    char *name{};

    sh_int version{};
    char *short_descr{};
    char *long_descr{};
    char *description{};
    char *sentient_victim{};
    sh_int sex{};
    sh_int class_num{};
    sh_int race{};
    sh_int level{};
    sh_int trust{};
    Seconds played{};
    int lines{}; /* for the pager */
    Time logon{};
    Time last_note{};
    sh_int timer{};
    sh_int wait{};
    sh_int hit{};
    sh_int max_hit{};
    sh_int mana{};
    sh_int max_mana{};
    sh_int move{};
    sh_int max_move{};
    long gold{};
    long exp{};
    unsigned long act{};
    unsigned long comm{}; /* RT added to pad the vector */
    unsigned long imm_flags{};
    unsigned long res_flags{};
    unsigned long vuln_flags{};
    sh_int invis_level{};
    unsigned int affected_by{};
    sh_int position{};
    sh_int practice{};
    sh_int train{};
    sh_int carry_weight{};
    sh_int carry_number{};
    sh_int saving_throw{};
    sh_int alignment{};
    sh_int hitroll{};
    sh_int damroll{};
    sh_int armor[4]{};
    sh_int wimpy{};
    /* stats */
    Stats perm_stat;
    Stats mod_stat;
    /* parts stuff */
    unsigned long form{};
    unsigned long parts{};
    sh_int size{};
    sh_int material{};
    unsigned long hit_location{}; /* for verbose combat sequences */
    /* mobile stuff */
    unsigned long off_flags{};
    sh_int damage[3]{};
    sh_int dam_type{};
    sh_int start_pos{};
    sh_int default_pos{};

    char *clipboard{};
    unsigned long extra_flags[(MAX_EXTRA_FLAGS / 32) + 1]{};

    MPROG_ACT_LIST *mpact{}; /* Used by MOBprogram */
    int mpactnum{}; /* Used by MOBprogram */

    [[nodiscard]] Seconds total_played() const;

    // True if char can see victim.
    [[nodiscard]] bool can_see(const CHAR_DATA &victim) const;
    // True if char can see object.
    [[nodiscard]] bool can_see(const OBJ_DATA &object) const;
    // True if char can see a room.
    [[nodiscard]] bool can_see(const ROOM_INDEX_DATA &room) const;

    [[nodiscard]] bool is_npc() const;
    [[nodiscard]] bool is_pc() const { return !is_npc(); }
    [[nodiscard]] bool is_blind() const;
    [[nodiscard]] bool is_warrior() const;
    [[nodiscard]] bool is_thief() const;
    [[nodiscard]] bool is_invisible() const;
    [[nodiscard]] bool is_sneaking() const;
    [[nodiscard]] bool is_hiding() const;
    [[nodiscard]] bool is_berserk() const;
    [[nodiscard]] bool has_detect_invis() const;
    [[nodiscard]] bool has_detect_hidden() const;
    [[nodiscard]] bool has_infrared() const;

    // Is the player wizinvis/prowl at all, and are they invisible to a particular character?
    [[nodiscard]] bool is_wizinvis() const;
    [[nodiscard]] bool is_wizinvis_to(const CHAR_DATA &victim) const;
    [[nodiscard]] bool is_prowlinvis() const;
    [[nodiscard]] bool is_prowlinvis_to(const CHAR_DATA &victim) const;

    // Returns whether this is a PC with brief set.
    [[nodiscard]] bool is_comm_brief() const;
    // Returns whether this is a PC with autoexits
    [[nodiscard]] bool should_autoexit() const;

    // Retrieve a character's trusted level for permission checking.
    [[nodiscard]] int get_trust() const;

    // Returns the PC character controlling this character,or nullptr if not controlled by a pc.
    // That is:
    // * For a player: return this
    // * For an NPC controlled by a switched IMM, return that IMM
    // * For a normal NPC, return null.
    [[nodiscard]] const CHAR_DATA *player() const { return desc ? desc->person() : nullptr; }
    [[nodiscard]] CHAR_DATA *player() { return desc ? desc->person() : nullptr; }

    [[nodiscard]] bool has_holylight() const;
    [[nodiscard]] bool is_immortal() const;
    [[nodiscard]] bool is_hero() const;

    // Return a character's skill at the given skill number
    [[nodiscard]] int get_skill(int skill_number) const;

    // Get current and maximum stats.
    [[nodiscard]] sh_int curr_stat(Stat stat) const;
    [[nodiscard]] sh_int max_stat(Stat stat) const;

    // Return true if a char is affected by a spell.
    [[nodiscard]] bool is_affected_by(int skill_number) const;

    // Return a pointer to the character's overall clan if they have one.
    [[nodiscard]] const CLAN *clan() const;
    // Return a pointer to the character's individual clan membership info, if they have one.
    [[nodiscard]] PCCLAN *pc_clan();
    [[nodiscard]] const PCCLAN *pc_clan() const;

    // Send text to this character's user (if they have one).
    void send_to(std::string_view txt) const;

    // Page text to this character's user (if they have one).
    void page_to(std::string_view txt) const;

    // Sets a PC's title.
    void set_title(std::string title);

    // Gets an item in a character's inventory. Returns nullptr if not found. Supports numbered argument.
    [[nodiscard]] OBJ_DATA *find_in_inventory(std::string_view argument) const;

    // Gets an item a character is wearing. Returns nullptr if not found. Supports numbered argument.
    [[nodiscard]] OBJ_DATA *find_worn(std::string_view argument) const;

private:
    template <typename Func>
    [[nodiscard]] OBJ_DATA *find_filtered_obj(std::string_view argument, Func filter) const;
};
