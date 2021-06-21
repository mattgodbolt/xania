/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_comm.c: communications facilities within the MUD                 */
/*                                                                       */
/*************************************************************************/

#include "act_comm.hpp"
#include "ArgParser.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "TimeInfoData.hpp"
#include "challenge.hpp"
#include "chat/chatlink.h"
#include "comm.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "handler.hpp"
#include "info.hpp"
#include "interp.h"
#include "ride.hpp"
#include "save.hpp"

#include <fmt/format.h>

void do_delet(Char *ch) { ch->send_line("You must type the full command to delete yourself."); }

void do_delete(Char *ch, const char *argument) {

    if (ch->is_npc())
        return;

    if (ch->pcdata->confirm_delete) {
        if (argument[0] != '\0') {
            ch->send_line("Delete status removed.");
            ch->pcdata->confirm_delete = false;
            return;
        } else {
            /* Added by Rohan - to delete the cached info, if it has been
             cached of course! */
            remove_info_for_player(ch->name);
            auto strsave = filename_for_player(ch->name);
            do_quit(ch); // ch is invalid after this
            unlink(strsave.c_str());
            return;
        }
    }

    if (argument[0] != '\0') {
        ch->send_line("Just type delete. No argument.");
        return;
    }

    ch->send_line("Type delete again to confirm this command.");
    ch->send_line("WARNING: this command is irreversible.");
    ch->send_line("Typing delete with an argument will undo delete status.");
    ch->pcdata->confirm_delete = true;
}

void announce(std::string_view buf, const Char *ch) {
    if (ch->in_room == nullptr)
        return; /* special case on creation */

    for (auto &victim : descriptors().all_who_can_see(*ch) | DescriptorFilter::to_person()) {
        if (!IS_SET(victim.comm, COMM_NOANNOUNCE) && !IS_SET(victim.comm, COMM_QUIET))
            act(buf, &victim, nullptr, ch, To::Char, Position::Type::Dead);
    }
}

void do_say(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("|cSay what?\n\r|w");
        return;
    }

    ch->say(argument);
}

void do_afk(Char *ch, std::string_view argument) {
    static constexpr auto MaxAfkLength = 45u;
    if (ch->is_npc() || IS_SET(ch->comm, COMM_NOCHANNELS))
        return;

    if (IS_SET(ch->act, PLR_AFK) && argument.empty())
        ch->set_not_afk();
    else
        ch->set_afk(argument.empty() ? "afk" : argument.substr(0, MaxAfkLength));
}

static void tell_to(Char *ch, Char *victim, const char *text) {
    ch->set_not_afk();

    if (victim == nullptr || text == nullptr || text[0] == '\0') {
        ch->send_line("|cTell whom what?|w");

    } else if (IS_SET(ch->comm, COMM_NOTELL)) {
        ch->send_line("|cYour message didn't get through.|w");

    } else if (IS_SET(ch->comm, COMM_QUIET)) {
        ch->send_line("|cYou must turn off quiet mode first.|w");

    } else if (victim->desc == nullptr && victim->is_pc()) {
        act("|W$N|c seems to have misplaced $S link...try again later.|w", ch, nullptr, victim, To::Char);

    } else if (IS_SET(victim->comm, COMM_QUIET) && ch->is_mortal()) {
        act("|W$E|c is not receiving replies.|w", ch, nullptr, victim, To::Char);

    } else if (IS_SET(victim->act, PLR_AFK) && victim->is_pc()) {
        act(fmt::format("|W$N|c is {}.|w", victim->pcdata->afk), ch, nullptr, victim, To::Char, Position::Type::Dead);
        if (IS_SET(victim->comm, COMM_SHOWAFK)) {
            // TODO(#134) use the victim's timezone info.
            act(fmt::format("|c\007AFK|C: At {}, $n told you '{}|C'.|w", formatted_time(current_time), text).c_str(),
                ch, nullptr, victim, To::Vict, Position::Type::Dead);
            act("|cYour message was logged onto $S screen.|w", ch, nullptr, victim, To::Char, Position::Type::Dead);
            victim->reply = ch;
        }

    } else {
        act("|CYou tell $N '$t|C'|w", ch, text, victim, To::Char, Position::Type::Dead);
        act("|C$n tells you '$t|C'|w", ch, text, victim, To::Vict, Position::Type::Dead);
        victim->reply = ch;
        chatperform(victim, ch, text);
    }
}

void do_tell(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    const char *message = one_argument(argument, arg);

    if (arg[0] == '\0' || message[0] == '\0') {
        ch->send_line("|cTell whom what?|w");
        return;
    }
    victim = get_char_world(ch, arg);
    // TM: victim /may/ be null here, so don't check this if so
    if (victim && // added :)
        victim->is_npc() && victim->in_room != ch->in_room)
        victim = nullptr;
    tell_to(ch, victim, message);
}

void do_reply(Char *ch, const char *argument) { tell_to(ch, ch->reply, argument); }

void do_yell(Char *ch, std::string_view argument) {
    if (IS_SET(ch->comm, COMM_NOSHOUT)) {
        ch->send_line("|cYou can't yell.|w");
        return;
    }

    if (argument.empty()) {
        ch->send_line("|cYell what?|w");
        return;
    }

    ch->set_not_afk();
    ch->yell(argument);
}

void do_emote(Char *ch, const char *argument) {
    if (ch->is_pc() && IS_SET(ch->comm, COMM_NOEMOTE)) {
        ch->send_line("|cYou can't show your emotions.|w");

    } else if (argument[0] == '\0') {
        ch->send_line("|cEmote what?|w");

    } else {
        ch->set_not_afk();
        act("$n $T", ch, nullptr, argument, To::Room);
        act("$n $T", ch, nullptr, argument, To::Char);
        chatperformtoroom(argument, ch);
    }
}

/*
 * All the posing stuff.
 */
struct pose_table_type {
    const char *message[2 * MAX_CLASS];
};

const struct pose_table_type pose_table[] = {
    {{"You sizzle with energy.", "$n sizzles with energy.", "You feel very holy.", "$n looks very holy.",
      "You parade around in your fine armour.", "$n parades around in $s gleaming armour.",
      "You show your bulging muscles.", "$n shows $s bulging muscles."}},

    {{"You turn into a butterfly, then return to your normal shape.",
      "$n turns into a butterfly, then returns to $s normal shape.", "You nonchalantly turn wine into water.",
      "$n nonchalantly turns wine into water.", "You throw an apple into the air and dice it with your blade.",
      "$n throws an apple into the air and dices it with $s blade.", "You crack nuts between your fingers.",
      "$n cracks nuts between $s fingers."}},

    {{"Blue sparks fly from your fingers.", "Blue sparks fly from $n's fingers.", "A halo appears over your head.",
      "A halo appears over $n's head.", "You begin to recount your noble ancestry.",
      "$n begins to recount $s noble ancestry.", "You grizzle your teeth and look mean.",
      "$n grizzles $s teeth and looks mean."}},

    {{"Little red lights dance in your eyes.", "Little red lights dance in $n's eyes.", "You recite words of wisdom.",
      "$n recites words of wisdom.", "You start telling stories about your quest for the Holy Grail.",
      "$n starts telling stories about $s quest for the Holy Grail.", "You hit your head, and your eyes roll.",
      "$n hits $s head, and $s eyes roll."}},

    {{"A slimy green monster appears before you and bows.", "A slimy green monster appears before $n and bows.",
      "Deep in prayer, you levitate.", "Deep in prayer, $n levitates.",
      "On bended knee, you swear solemnly to defend the world from intruders.",
      "On bended knee, $n swears solemnly to protect you from harm.", "Crunch, crunch -- you munch a bottle.",
      "Crunch, crunch -- $n munches a bottle."}},

    {{"You turn everybody into a little pink elephant.", "You are turned into a little pink elephant by $n.",
      "An angel consults you.", "An angel consults $n.", "Your razor edged sword slices the air in two!",
      "$n slices the air in two with $s razor edged sword!", "... 98, 99, 100 ... you do pushups.",
      "... 98, 99, 100 ... $n does pushups."}},

    {{"A small ball of light dances on your fingertips.", "A small ball of light dances on $n's fingertips.",
      "Your body glows with an unearthly light.", "$n's body glows with an unearthly light.",
      "You start polishing up your gleaming armour plates.", "$n starts polishing up $s gleaming armour plates.",
      "Arnold Schwarzenegger admires your physique.", "Arnold Schwarzenegger admires $n's physique."}},

    {{"Smoke and fumes leak from your nostrils.", "Smoke and fumes leak from $n's nostrils.", "A spot light hits you.",
      "A spot light hits $n.", "You recite your motto of valour and honour.",
      "$n recites $s motto of valour and honour.", "Watch your feet, you are juggling granite boulders.",
      "Watch your feet, $n is juggling granite boulders."}},

    {{"The light flickers as you rap in magical languages.", "The light flickers as $n raps in magical languages.",
      "Everyone levitates as you pray.", "You levitate as $n prays.", "You hone your blade with a piece of silk.",
      "$n hones $s blade with a piece of silk.", "Oomph!  You squeeze water out of a granite boulder.",
      "Oomph!  $n squeezes water out of a granite boulder."}},

    {{"Your head disappears.", "$n's head disappears.", "A cool breeze refreshes you.", "A cool breeze refreshes $n.",
      "Your band of minstrels sing of your heroic quests.", "$n's band of minstrels sing of $s heroic quests.",
      "You pick your teeth with a spear.", "$n picks $s teeth with a spear."}},

    {{"A fire elemental singes your hair.", "A fire elemental singes $n's hair.",
      "The sun pierces through the clouds to illuminate you.", "The sun pierces through the clouds to illuminate $n.",
      "Your laugh in the face of peril.", "$n laughs in the face of peril.",
      "Everyone is swept off their foot by your hug.", "You are swept off your feet by $n's hug."}},

    {{"The sky changes color to match your eyes.", "The sky changes color to match $n's eyes.",
      "The ocean parts before you.", "The ocean parts before $n.",
      "You lean on your weapon and it's blade sinks into the solid ground.",
      "$n leans on $s weapon and it's blade sinks into the ground.", "Your karate chop splits a tree.",
      "$n's karate chop splits a tree."}},

    {{"The stones dance to your command.", "The stones dance to $n's command.", "A thunder cloud kneels to you.",
      "A thunder cloud kneels to $n.", "A legendary king passes by but stops to buy you a beer .",
      "A legendary king passes by but stops to buy $n a beer.", "A strap of your armor breaks over your mighty thews.",
      "A strap of $n's armor breaks over $s mighty thews."}},

    {{"The heavens and grass change colour as you smile.", "The heavens and grass change colour as $n smiles.",
      "The Burning Man speaks to you.", "The Burning Man speaks to $n.", "Your own shadow bows to your greatness.",
      "$n's shadow bows to $n greatness.", "A boulder cracks at your frown.", "A boulder cracks at $n's frown."}},

    {{"Everyone's clothes are transparent, and you are laughing.", "Your clothes are transparent, and $n is laughing.",
      "An eye in a pyramid winks at you.", "An eye in a pyramid winks at $n.",
      "You whistle and ten wild horses arrive to do your bidding.",
      "$n whistles and ten wild horses arrive to do $s bidding.", "Mercenaries arrive to do your bidding.",
      "Mercenaries arrive to do $n's bidding."}},

    {{"A black hole swallows you.", "A black hole swallows $n.", "Valentine Michael Smith offers you a glass of water.",
      "Valentine Michael Smith offers $n a glass of water.",
      "There isn't enough space in the world for both you AND your ego!",
      "$n and $s ego are too big to fit in only one world!", "Four matched Percherons bring in your chariot.",
      "Four matched Percherons bring in $n's chariot."}},

    {{"The world shimmers in time with your whistling.", "The world shimmers in time with $n's whistling.",
      "The great god gives you a staff.", "The great god gives $n a staff.",
      "You flutter your eyelids and a passing daemon falls dead.",
      "$n flutters $s eyelids and accidentally slays a passing daemon.", "Atlas asks you to relieve him.",
      "Atlas asks $n to relieve him."}}};

void do_pose(Char *ch) {
    int level;
    int pose;

    if (ch->is_npc())
        return;

    level = UMIN(ch->level, (int)(sizeof(pose_table) / sizeof(pose_table[0]) - 1));
    pose = number_range(0, level);

    act(pose_table[pose].message[2 * ch->class_num + 0], ch, nullptr, nullptr, To::Char);
    act(pose_table[pose].message[2 * ch->class_num + 1], ch);
}

void do_bug(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Please provide a brief description of the bug!");
        return;
    }
    append_file(ch, Configuration::singleton().bug_file().c_str(), argument);
    ch->send_line("|RBug logged! If you're lucky it may even get fixed!|w");
}

void do_idea(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("Please provide a brief description of your idea!");
        return;
    }
    append_file(ch, Configuration::singleton().ideas_file().c_str(), argument);
    ch->send_line("|WIdea logged. This is |RNOT|W an identify command.|w");
}

void do_typo(Char *ch, const char *argument) {
    if (argument[0] == '\0') {
        ch->send_line("A typo you say? Tell us where!");
        return;
    }
    append_file(ch, Configuration::singleton().typo_file().c_str(), argument);
    ch->send_line("|WTypo logged. One day we'll fix it, or buy a spellchecker.|w");
}

void do_qui(Char *ch) { ch->send_line("|cIf you want to |RQUIT|c, you have to spell it out.|w"); }

void do_quit(Char *ch) {
    Descriptor *d;

    if (ch->is_npc())
        return;

    if (ch->pnote && (ch->desc != nullptr)) {
        /* Allow linkdeads to be auto-quitted even if they have a note */
        ch->send_line("|cDon't you want to post your note first?|w");
        return;
    }

    if (ch->is_pos_fighting()) {
        ch->send_line("|RNo way! You are fighting.|w");
        return;
    }

    if (ch->is_pos_dying()) {
        ch->send_line("|RYou're not DEAD yet.|w");
        return;
    }
    do_chal_canc(ch);
    ch->send_line("|WYou quit reality for the game.|w");
    act("|W$n has left reality for the game.|w", ch);
    log_string("{} has quit.", ch->name);
    announce(fmt::format("|W### |P{}|W departs, seeking another reality.|w", ch->name), ch);

    /*
     * After extract_char the ch is no longer valid!
     */
    /* Rohan: if char's info is in cache, update login time and host */
    update_info_cache(ch);

    save_char_obj(ch);
    d = ch->desc;
    extract_char(ch, true);
    if (d != nullptr)
        d->close();
}

void do_save(Char *ch) {
    if (ch->is_npc())
        return;

    save_char_obj(ch);
    ch->send_line("|cTo Save is wisdom, but don't forget |WXania|c does it automagically!|w");
    WAIT_STATE(ch, 5 * PULSE_VIOLENCE);
}

void do_follow(Char *ch, ArgParser args) {
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    if (args.empty()) {
        ch->send_line("Follow whom?");
        return;
    }

    auto victim = get_char_room(ch, args.shift());
    if (!victim) {
        ch->send_line("They aren't here.");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != nullptr) {
        act("But you'd rather follow $N!", ch, nullptr, ch->master, To::Char);
        return;
    }

    if (victim == ch) {
        if (ch->master == nullptr) {
            ch->send_line("You already follow yourself.");
            return;
        }
        stop_follower(ch);
        return;
    }

    if (victim->is_pc() && IS_SET(victim->act, PLR_NOFOLLOW) && ch->is_mortal()) {
        act("$N doesn't seem to want any followers.\n\r", ch, nullptr, victim, To::Char);
        return;
    }

    REMOVE_BIT(ch->act, PLR_NOFOLLOW);

    if (ch->master != nullptr)
        stop_follower(ch);

    add_follower(ch, victim);
}

void add_follower(Char *ch, Char *master) {
    if (ch->master != nullptr) {
        bug("Add_follower: non-null master.");
        return;
    }

    ch->master = master;
    ch->leader = nullptr;

    if (can_see(master, ch))
        act("$n now follows you.", ch, nullptr, master, To::Vict);

    act("You now follow $N.", ch, nullptr, master, To::Char);
}

void stop_follower(Char *ch) {
    if (ch->master == nullptr) {
        bug("Stop_follower: null master.");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        REMOVE_BIT(ch->affected_by, AFF_CHARM);
        affect_strip(ch, gsn_charm_person);
    }

    if (can_see(ch->master, ch) && ch->in_room != nullptr) {
        act("$n stops following you.", ch, nullptr, ch->master, To::Vict);
        act("You stop following $N.", ch, nullptr, ch->master, To::Char);
    }
    if (ch->master->pet == ch) {
        if (ch->master->riding == ch) {
            unride_char(ch->master, ch);
        }
        ch->master->pet = nullptr;
    }

    ch->master = nullptr;
    ch->leader = nullptr;
}

/* nukes charmed monsters and pets */
void nuke_pets(Char *ch) {
    Char *pet;

    if ((pet = ch->pet) != nullptr) {
        stop_follower(pet);
        if (pet->ridden_by != nullptr) {
            unride_char(ch, pet);
        }

        if (pet->in_room != nullptr) {
            act("$N slowly fades away.", ch, nullptr, pet, To::NotVict);
            extract_char(pet, true);
        }
    }
    ch->pet = nullptr;
}

void die_follower(Char *ch) {
    if (ch->master != nullptr) {
        if (ch->master->pet == ch)
            ch->master->pet = nullptr;
        stop_follower(ch);
    }

    ch->leader = nullptr;

    for (auto *fch : char_list) {
        if (fch->master == ch)
            stop_follower(fch);
        if (fch->leader == ch)
            fch->leader = fch;
    }
}

void do_order(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Char *victim;
    bool found;
    bool fAll;

    auto *command_remainder = one_argument(argument, arg);
    one_argument(command_remainder, arg2);

    if (!str_cmp(arg2, "delete")) {
        ch->send_line("That will NOT be done.");
        return;
    }

    if (arg[0] == '\0' || command_remainder[0] == '\0') {
        ch->send_line("Order whom to do what?");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        ch->send_line("You feel like taking, not giving, orders.");
        return;
    }

    if (!str_cmp(arg, "all")) {
        fAll = true;
        victim = nullptr;
    } else {
        fAll = false;
        if ((victim = get_char_room(ch, arg)) == nullptr) {
            ch->send_line("They aren't here.");
            return;
        }

        if (victim == ch) {
            ch->send_line("Aye aye, right away!");
            return;
        }

        if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch) {
            ch->send_line("Do it yourself!");
            return;
        }
    }

    found = false;
    for (auto *och : ch->in_room->people) {
        if (IS_AFFECTED(och, AFF_CHARM) && och->master == ch && (fAll || och == victim)) {
            found = true;
            act(fmt::format("|W$n|w orders you to '{}'.", command_remainder), ch, nullptr, och, To::Vict);
            WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
            // We know this points into the remainder of "argument"
            interpret(och, command_remainder);
        }
    }

    if (found)
        ch->send_line("Ok.");
    else
        ch->send_line("You have no followers here.");
}

void do_group(Char *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    Char *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        ch->send_line("{}'s group:", pers(ch->leader ? ch->leader : ch, ch));

        for (auto *gch : char_list) {
            if (is_same_group(gch, ch)) {
                ch->send_line("[{:3} {}] {:<16} {:4}/{:4} hp {:4}/{:4} mana {:4}/{:4} mv {:5} xp", gch->level,
                              gch->is_npc() ? "Mob" : class_table[gch->class_num].who_name, pers(gch, ch), gch->hit,
                              gch->max_hit, gch->mana, gch->max_mana, gch->move, gch->max_move, gch->exp);
            }
        }
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }

    if (ch->master != nullptr || (ch->leader != nullptr && ch->leader != ch)) {
        ch->send_line("But you are following someone else!");
        return;
    }

    if (victim->master != ch && ch != victim) {
        act("|W$N|w isn't following you.", ch, nullptr, victim, To::Char);
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM)) {
        ch->send_line("You can't remove charmed mobs from your group.");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        act("You like your master too much to leave $m!", ch, nullptr, victim, To::Vict);
        return;
    }

    if (is_same_group(victim, ch) && ch != victim) {
        victim->leader = nullptr;
        act("$n removes $N from $s group.", ch, nullptr, victim, To::NotVict);
        act("$n removes you from $s group.", ch, nullptr, victim, To::Vict);
        act("You remove $N from your group.", ch, nullptr, victim, To::Char);
        return;
    }
    /*
        if ( ch->level - victim->level < -8
        ||   ch->level - victim->level >  8 )
        {
         act( "$N cannot join $n's group.",     ch, nullptr, victim, To::NotVict );
         act( "You cannot join $n's group.",    ch, nullptr, victim, To::Vict    );
         act( "$N cannot join your group.",     ch, nullptr, victim, To::Char    );
         return;
        }
    */

    victim->leader = ch;
    act("$N joins $n's group.", ch, nullptr, victim, To::NotVict);
    act("You join $n's group.", ch, nullptr, victim, To::Vict);
    act("$N joins your group.", ch, nullptr, victim, To::Char);
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split(Char *ch, ArgParser args) {
    if (args.empty()) {
        ch->send_line("Split how much?");
        return;
    }

    split_coins(ch, args.try_shift_number().value_or(-1));
}

void split_coins(Char *ch, int amount) {
    if (amount < 0) {
        ch->send_line("Your group wouldn't like that.");
        return;
    }

    if (amount == 0) {
        ch->send_line("You hand out zero coins, but no one notices.");
        return;
    }

    if (ch->gold < amount) {
        ch->send_line("You don't have that much gold.");
        return;
    }

    int members = 0;
    for (auto *gch : ch->in_room->people) {
        if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
            members++;
    }

    if (members < 2) {
        ch->send_line("You'd share the gold if there was someone here to split it with!");
        return;
    }

    int share = amount / members;
    int extra = amount % members;

    if (share == 0) {
        ch->send_line("Don't even bother, cheapskate.");
        return;
    }

    ch->gold -= amount;
    ch->gold += share + extra;

    ch->send_line("You split {} gold coins.  Your share is {} gold coins.", amount, share + extra);

    auto message = fmt::format("$n splits {} gold coins.  Your share is {} gold coins.", amount, share);

    for (auto *gch : ch->in_room->people) {
        if (gch != ch && is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM)) {
            act(message, ch, nullptr, gch, To::Vict);
            gch->gold += share;
        }
    }
}

void do_gtell(Char *ch, std::string_view argument) {
    if (argument.empty()) {
        ch->send_line("|cTell your group what?|w");
        return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL)) {
        ch->send_line("|cYour message didn't get through!|w");
        return;
    }

    // Note use of send_line (not act), so gtell works on sleepers.
    ch->send_line("|CYou tell the group '{}|C'|w.", argument);
    auto msg = fmt::format("|C{} tells the group '{}|C'|w.", ch->name, argument);
    for (auto *gch : char_list)
        if (is_same_group(gch, ch) && gch != ch)
            gch->send_line(msg);
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(const Char *ach, const Char *bch) {
    if (ach->leader != nullptr)
        ach = ach->leader;
    if (bch->leader != nullptr)
        bch = bch->leader;
    return ach == bch;
}

/**
 * Trigger the Eliza chat engine. Can cause an NPC to say something back to the player.
 * to_npc: the NPC that received the chat/social message.
 * from_player: the player that sent it.
 */
void chatperform(Char *to_npc, Char *from_player, std::string_view msg) {
    if (to_npc->is_pc() || (from_player != nullptr && from_player->is_npc()))
        return; /* failsafe */
    std::string reply = dochat(from_player ? from_player->name : "you", msg, to_npc->name);
    switch (reply[0]) {
    case '\0': break;
    case '"': /* say message */ to_npc->say(reply.substr(1)); break;
    case ':': /* do emote */ do_emote(to_npc, reply.substr(1).c_str()); break;
    case '!': /* do command */ interpret(to_npc, reply.substr(1).c_str()); break;
    default: /* say or tell */
        if (from_player == nullptr) {
            to_npc->say(reply.c_str());
        } else {
            act("$N tells you '$t'.", from_player, reply, to_npc, To::Char);
            from_player->reply = to_npc;
        }
    }
}

void chatperformtoroom(std::string_view text, Char *ch) {
    if (ch->is_npc())
        return;

    for (auto *vch : ch->in_room->people)
        if (vch->is_npc() && IS_SET(vch->pIndexData->act, ACT_TALKATIVE) && vch->is_pos_awake()) {
            if (number_percent() > 66) /* less spammy - Fara */
                chatperform(vch, ch, text);
        }
}
