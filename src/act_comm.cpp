/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_comm.c: communications facilities within the MUD                 */
/*                                                                       */
/*************************************************************************/

#include "Descriptor.hpp"
#include "info.hpp"
#include "merc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

/* command procedures needed */
void do_quit(CHAR_DATA *ch, const char *arg);

/* Rohan's info stuff - extern to Player list */
extern KNOWN_PLAYERS *player_list;

void do_delet(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("You must type the full command to delete yourself.\n\r", ch);
}

void do_delete(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    char strsave[MAX_INPUT_LENGTH];
    KNOWN_PLAYERS *cursor, *temp;

    if (IS_NPC(ch))
        return;

    if (ch->pcdata->confirm_delete) {
        if (argument[0] != '\0') {
            send_to_char("Delete status removed.\n\r", ch);
            ch->pcdata->confirm_delete = false;
            return;
        } else {
            /* Added by Rohan - to delete the name out of player list if a player
               deletes. Eventually, info will have to be deleted from cached
               info if it is in there */
            cursor = player_list;
            if (cursor != nullptr && !(strcmp(cursor->name, ch->name))) {
                player_list = player_list->next;
                free_string(cursor->name);
                free_mem(cursor, sizeof(KNOWN_PLAYERS));
                /*	     log_string ("Player name removed from player list.");*/
            } else if (cursor != nullptr) {
                while (cursor->next != nullptr && cursor->next->name != ch->name)
                    cursor = cursor->next;
                if (cursor->next != nullptr) {
                    temp = cursor->next;
                    cursor->next = cursor->next->next;
                    free_string(temp->name);
                    free_mem(temp, sizeof(KNOWN_PLAYERS));
                    /*		 log_string ("Player name removed from player list.");*/
                } else
                    bug("Deleted player was not in player list.");
            } else
                bug("Player list was empty. Not good.");
            /* Added by Rohan - to delete the cached info, if it has been
             cached of course! */
            remove_info_for_player(ch->name);

            snprintf(strsave, sizeof(strsave), "%s%s", PLAYER_DIR, capitalize(ch->name));
            do_quit(ch, "");
            unlink(strsave);
            return;
        }
    }

    if (argument[0] != '\0') {
        send_to_char("Just type delete. No argument.\n\r", ch);
        return;
    }

    send_to_char("Type delete again to confirm this command.\n\r", ch);
    send_to_char("WARNING: this command is irreversible.\n\r", ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r", ch);
    ch->pcdata->confirm_delete = true;
}

void announce(const char *buf, CHAR_DATA *ch) {
    Descriptor *d;

    if (descriptor_list == nullptr)
        return;

    if (ch->in_room == nullptr)
        return; /* special case on creation */

    for (d = descriptor_list; d != nullptr; d = d->next) {
        CHAR_DATA *victim;

        victim = d->person();

        if (d->is_playing() && d->character() != ch && victim && can_see(victim, ch)
            && !IS_SET(victim->comm, COMM_NOANNOUNCE) && !IS_SET(victim->comm, COMM_QUIET)) {
            act_new(buf, victim, nullptr, ch, TO_CHAR, POS_DEAD);
        }
    }
}

void do_say(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        send_to_char("|cSay what?\n\r|w", ch);
        return;
    }

    act("$n|w says '$T|w'", ch, nullptr, argument, TO_ROOM);
    act("You say '$T|w'", ch, nullptr, argument, TO_CHAR);
    chatperformtoroom(argument, ch);
    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    mprog_speech_trigger(argument, ch);
}

void do_afk(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || IS_SET(ch->comm, COMM_NOCHANNELS))
        return;

    if (IS_SET(ch->act, PLR_AFK) && (argument == nullptr || strlen(argument) == 0)) {
        send_to_char("|cYour keyboard welcomes you back!|w\n\r", ch);
        send_to_char("|cYou are no longer marked as being afk.|w\n\r", ch);
        act_new("|W$n's|w keyboard has welcomed $m back!", ch, nullptr, nullptr, TO_ROOM, POS_DEAD);
        act_new("|W$n|w is no longer afk.", ch, nullptr, nullptr, TO_ROOM, POS_DEAD);
        announce("|W###|w (|cAFK|w) $N has returned to $S keyboard.", ch);
        REMOVE_BIT(ch->act, PLR_AFK);
    } else {
        free_string(ch->pcdata->afk);

        if (argument[0] == '\0')
            ch->pcdata->afk = str_dup("afk");
        else {
            strncpy(buf, argument, 45);
            ch->pcdata->afk = str_dup(buf);
        }

        snprintf(buf, sizeof(buf), "|cYou notify the mud that you are %s|c.|w", ch->pcdata->afk);
        act_new(buf, ch, nullptr, nullptr, TO_CHAR, POS_DEAD);

        snprintf(buf, sizeof(buf), "|W$n|w is %s|w.", ch->pcdata->afk);
        act_new(buf, ch, nullptr, nullptr, TO_ROOM, POS_DEAD);

        snprintf(buf, sizeof(buf), "|W###|w (|cAFK|w) $N is %s|w.", ch->pcdata->afk);
        announce(buf, ch);

        SET_BIT(ch->act, PLR_AFK);
    }
}

static void tell_to(CHAR_DATA *ch, CHAR_DATA *victim, const char *text) {
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->act, PLR_AFK))
        do_afk(ch, nullptr);

    if (victim == nullptr || text == nullptr || text[0] == '\0') {
        send_to_char("|cTell whom what?|w\n\r", ch);

    } else if (IS_SET(ch->comm, COMM_NOTELL)) {
        send_to_char("|cYour message didn't get through.|w\n\r", ch);

    } else if (IS_SET(ch->comm, COMM_QUIET)) {
        send_to_char("|cYou must turn off quiet mode first.|w\n\r", ch);

    } else if (victim->desc == nullptr && !IS_NPC(victim)) {
        act("|W$N|c seems to have misplaced $S link...try again later.|w", ch, nullptr, victim, TO_CHAR);

    } else if (IS_SET(victim->comm, COMM_QUIET) && !IS_IMMORTAL(ch)) {
        act("|W$E|c is not receiving replies.|w", ch, 0, victim, TO_CHAR);

    } else if (IS_SET(victim->act, PLR_AFK) && !IS_NPC(victim)) {
        snprintf(buf, sizeof(buf), "|W$N|c is %s.|w", victim->pcdata->afk);
        act_new(buf, ch, nullptr, victim, TO_CHAR, POS_DEAD);
        if (IS_SET(victim->comm, COMM_SHOWAFK)) {
            char *strtime;
            strtime = ctime(&current_time);
            strtime[strlen(strtime) - 1] = '\0';
            snprintf(buf, sizeof(buf), "|c%cAFK|C: At %s, $n told you '%s|C'.|w", 7, strtime, text);
            act_new(buf, ch, nullptr, victim, TO_VICT, POS_DEAD);
            snprintf(buf, sizeof(buf), "|cYour message was logged onto $S screen.|w");
            act_new(buf, ch, nullptr, victim, TO_CHAR, POS_DEAD);
            victim->reply = ch;
        }

    } else {
        act_new("|CYou tell $N '$t|C'|w", ch, text, victim, TO_CHAR, POS_DEAD);
        act_new("|C$n tells you '$t|C'|w", ch, text, victim, TO_VICT, POS_DEAD);
        victim->reply = ch;
        chatperform(victim, ch, text);
    }
}

void do_tell(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    const char *message = one_argument(argument, arg);

    if (arg[0] == '\0' || message[0] == '\0') {
        send_to_char("|cTell whom what?|w\n\r", ch);
        return;
    }
    victim = get_char_world(ch, arg);
    // TM: victim /may/ be null here, so don't check this if so
    if (victim && // added :)
        IS_NPC(victim) && victim->in_room != ch->in_room)
        victim = nullptr;
    tell_to(ch, victim, message);
}

void do_reply(CHAR_DATA *ch, const char *argument) { tell_to(ch, ch->reply, argument); }

void do_yell(CHAR_DATA *ch, const char *argument) {
    Descriptor *d;

    if (IS_SET(ch->comm, COMM_NOSHOUT)) {
        send_to_char("|cYou can't yell.|w\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("|cYell what?|w\n\r", ch);
        return;
    }

    if (IS_SET(ch->act, PLR_AFK))
        do_afk(ch, nullptr);

    act("|WYou yell '$t|W'|w", ch, argument, nullptr, TO_CHAR);
    for (d = descriptor_list; d != nullptr; d = d->next) {
        if (d->is_playing() && d->character() != ch && d->character()->in_room != nullptr
            && d->character()->in_room->area == ch->in_room->area && !IS_SET(d->character()->comm, COMM_QUIET)) {
            act("|W$n yells '$t|W'|w", ch, argument, d->character(), TO_VICT);
        }
    }
}

void do_emote(CHAR_DATA *ch, const char *argument) {
    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE)) {
        send_to_char("|cYou can't show your emotions.|w\n\r", ch);

    } else if (argument[0] == '\0') {
        send_to_char("|cEmote what?|w\n\r", ch);

    } else {
        if (IS_SET(ch->act, PLR_AFK))
            do_afk(ch, nullptr);

        act("$n $T", ch, nullptr, argument, TO_ROOM);
        act("$n $T", ch, nullptr, argument, TO_CHAR);
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

void do_pose(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    int level;
    int pose;

    if (IS_NPC(ch))
        return;

    level = UMIN(ch->level, (int)(sizeof(pose_table) / sizeof(pose_table[0]) - 1));
    pose = number_range(0, level);

    act(pose_table[pose].message[2 * ch->class_num + 0], ch, nullptr, nullptr, TO_CHAR);
    act(pose_table[pose].message[2 * ch->class_num + 1], ch, nullptr, nullptr, TO_ROOM);
}

void do_bug(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        send_to_char("Please provide a brief description of the bug!\n\r", ch);
        return;
    }
    append_file(ch, BUG_FILE, argument);
    send_to_char("|RBug logged! If you're lucky it may even get fixed!|w\n\r", ch);
}

void do_idea(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        send_to_char("Please provide a brief description of your idea!\n\r", ch);
        return;
    }
    append_file(ch, IDEA_FILE, argument);
    send_to_char("|WIdea logged. This is |RNOT|W an identify command.|w\n\r", ch);
}

void do_typo(CHAR_DATA *ch, const char *argument) {
    if (argument[0] == '\0') {
        send_to_char("A typo you say? Tell us where!\n\r", ch);
        return;
    }
    append_file(ch, TYPO_FILE, argument);
    send_to_char("|WTypo logged. One day we'll fix it, or buy a spellchecker.|w\n\r", ch);
}

void do_qui(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    send_to_char("|cIf you want to |RQUIT|c, you have to spell it out.|w\n\r", ch);
}

void do_quit(CHAR_DATA *ch, const char *arg) {
    (void)arg;
    Descriptor *d;

    if (IS_NPC(ch))
        return;

    if ((ch->pnote != nullptr) && (ch->desc != nullptr)) {
        /* Allow linkdeads to be auto-quitted even if they have a note */
        send_to_char("|cDon't you want to post your note first?|w\n\r", ch);
        return;
    }

    if (ch->position == POS_FIGHTING) {
        send_to_char("|RNo way! You are fighting.|w\n\r", ch);
        return;
    }

    if (ch->position < POS_STUNNED) {
        send_to_char("|RYou're not DEAD yet.|w\n\r", ch);
        return;
    }
    do_chal_canc(ch);
    send_to_char("|WYou quit reality for the game.|w\n\r", ch);
    act("|W$n has left reality for the game.|w", ch, nullptr, nullptr, TO_ROOM);
    snprintf(log_buf, LOG_BUF_SIZE, "%s has quit.", ch->name);
    log_string(log_buf);
    snprintf(log_buf, LOG_BUF_SIZE, "|W### |P%s|W departs, seeking another reality.|w", ch->name);
    announce(log_buf, ch);

    /*
     * After extract_char the ch is no longer valid!
     */
    /* Rohan: if char's info is in cache, update login time and host */
    update_info_cache(ch);

    save_char_obj(ch);
    d = ch->desc;
    extract_char(ch, true);
    if (d != nullptr)
        close_socket(d);
}

void do_save(CHAR_DATA *ch, const char *arg) {
    (void)arg;
    if (IS_NPC(ch))
        return;

    save_char_obj(ch);
    send_to_char("|cTo Save is wisdom, but don't forget |WXania|c does it automagically!|w\n\r", ch);
    WAIT_STATE(ch, 5 * PULSE_VIOLENCE);
}

void char_ride(CHAR_DATA *ch, CHAR_DATA *pet);
void unride_char(CHAR_DATA *ch, CHAR_DATA *pet);

void do_ride(CHAR_DATA *ch, const char *argument) {

    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ridee;

    if (IS_NPC(ch))
        return;

    if (get_skill_learned(ch, gsn_ride) == 0) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->riding != nullptr) {
        send_to_char("You can only ride one mount at a time.\n\r", ch);
        return;
    }

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Ride whom or what?\n\r", ch);
        return;
    }

    if ((ridee = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if ((!IS_SET(ridee->act, ACT_CAN_BE_RIDDEN)) || (ridee->master != ch)) {
        act("You can't ride $N!", ch, nullptr, ridee, TO_CHAR);
        return;
    }

    char_ride(ch, ridee);
}

void char_ride(CHAR_DATA *ch, CHAR_DATA *ridee) {
    AFFECT_DATA af;

    act("You leap gracefully onto $N.", ch, nullptr, ridee, TO_CHAR);
    act("$n swings up gracefully onto the back of $N.", ch, nullptr, ridee, TO_ROOM);
    ch->riding = ridee;
    ridee->ridden_by = ch;

    af.type = gsn_ride;
    af.level = ridee->level;
    af.duration = -1;
    af.location = APPLY_DAMROLL;
    af.modifier = (ridee->level / 10) + (get_curr_stat(ridee, STAT_DEX) / 8);
    af.bitvector = 0;
    affect_to_char(ch, &af);
    af.location = APPLY_HITROLL;
    af.modifier = -(((ridee->level / 10) + (get_curr_stat(ridee, STAT_DEX) / 8)) / 4);
    affect_to_char(ch, &af);
}

void do_dismount(CHAR_DATA *ch, const char *argument) {
    (void)argument;
    if (IS_NPC(ch))
        return;

    if (get_skill_learned(ch, gsn_ride) == 0) {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->riding == nullptr) {
        send_to_char("But you aren't riding anything!\n\r", ch);
        return;
    }
    unride_char(ch, ch->riding);
}

void thrown_off(CHAR_DATA *ch, CHAR_DATA *pet) {
    act("|RYou are flung from $N!|w", ch, nullptr, pet, TO_CHAR);
    act("$n is flung from $N!", ch, nullptr, pet, TO_ROOM);
    fallen_off_mount(ch);
}

void fallen_off_mount(CHAR_DATA *ch) {
    CHAR_DATA *pet = ch->riding;
    if (pet == nullptr)
        return;

    pet->ridden_by = nullptr;
    ch->riding = nullptr;

    affect_strip(ch, gsn_ride);
    check_improve(ch, gsn_ride, false, 3);

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    ch->position = POS_RESTING;
    damage(pet, ch, number_range(2, 2 + 2 * ch->size + pet->size), gsn_bash, DAM_BASH);
}

void unride_char(CHAR_DATA *ch, CHAR_DATA *pet) {
    act("You swing your leg over and dismount $N.", ch, nullptr, pet, TO_CHAR);
    act("$n smoothly dismounts $N.", ch, nullptr, pet, TO_ROOM);
    pet->ridden_by = nullptr;
    ch->riding = nullptr;

    affect_strip(ch, gsn_ride);
}

void do_follow(CHAR_DATA *ch, const char *argument) {
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Follow whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != nullptr) {
        act("But you'd rather follow $N!", ch, nullptr, ch->master, TO_CHAR);
        return;
    }

    if (victim == ch) {
        if (ch->master == nullptr) {
            send_to_char("You already follow yourself.\n\r", ch);
            return;
        }
        stop_follower(ch);
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW) && !IS_IMMORTAL(ch)) {
        act("$N doesn't seem to want any followers.\n\r", ch, nullptr, victim, TO_CHAR);
        return;
    }

    REMOVE_BIT(ch->act, PLR_NOFOLLOW);

    if (ch->master != nullptr)
        stop_follower(ch);

    add_follower(ch, victim);
}

void add_follower(CHAR_DATA *ch, CHAR_DATA *master) {
    if (ch->master != nullptr) {
        bug("Add_follower: non-null master.");
        return;
    }

    ch->master = master;
    ch->leader = nullptr;

    if (can_see(master, ch))
        act("$n now follows you.", ch, nullptr, master, TO_VICT);

    act("You now follow $N.", ch, nullptr, master, TO_CHAR);
}

void stop_follower(CHAR_DATA *ch) {
    if (ch->master == nullptr) {
        bug("Stop_follower: null master.");
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        REMOVE_BIT(ch->affected_by, AFF_CHARM);
        affect_strip(ch, gsn_charm_person);
    }

    if (can_see(ch->master, ch) && ch->in_room != nullptr) {
        act("$n stops following you.", ch, nullptr, ch->master, TO_VICT);
        act("You stop following $N.", ch, nullptr, ch->master, TO_CHAR);
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
void nuke_pets(CHAR_DATA *ch) {
    CHAR_DATA *pet;

    if ((pet = ch->pet) != nullptr) {
        stop_follower(pet);
        if (pet->ridden_by != nullptr) {
            unride_char(ch, pet);
        }

        if (pet->in_room != nullptr) {
            act("$N slowly fades away.", ch, nullptr, pet, TO_NOTVICT);
            extract_char(pet, true);
        }
    }
    ch->pet = nullptr;
}

void die_follower(CHAR_DATA *ch) {
    CHAR_DATA *fch;

    if (ch->master != nullptr) {
        if (ch->master->pet == ch)
            ch->master->pet = nullptr;
        stop_follower(ch);
    }

    ch->leader = nullptr;

    for (fch = char_list; fch != nullptr; fch = fch->next) {
        if (fch->master == ch)
            stop_follower(fch);
        if (fch->leader == ch)
            fch->leader = fch;
    }
}

void do_order(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    auto *command_remainder = one_argument(argument, arg);
    one_argument(command_remainder, arg2);

    if (!str_cmp(arg2, "delete")) {
        send_to_char("That will NOT be done.\n\r", ch);
        return;
    }

    if (arg[0] == '\0' || command_remainder[0] == '\0') {
        send_to_char("Order whom to do what?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        send_to_char("You feel like taking, not giving, orders.\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        fAll = true;
        victim = nullptr;
    } else {
        fAll = false;
        if ((victim = get_char_room(ch, arg)) == nullptr) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch) {
            send_to_char("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch) {
            send_to_char("Do it yourself!\n\r", ch);
            return;
        }
    }

    found = false;
    for (och = ch->in_room->people; och != nullptr; och = och_next) {
        och_next = och->next_in_room;

        if (IS_AFFECTED(och, AFF_CHARM) && och->master == ch && (fAll || och == victim)) {
            found = true;
            snprintf(buf, sizeof(buf), "|W$n|w orders you to '%s'.", command_remainder);
            act(buf, ch, nullptr, och, TO_VICT);
            WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
            // We know this points into the remainder of "argument"
            interpret(och, command_remainder);
        }
    }

    if (found)
        send_to_char("Ok.\n\r", ch);
    else
        send_to_char("You have no followers here.\n\r", ch);
}

void do_group(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        CHAR_DATA *gch;
        CHAR_DATA *leader;

        leader = (ch->leader != nullptr) ? ch->leader : ch;
        snprintf(buf, sizeof(buf), "%s's group:\n\r", PERS(leader, ch));
        send_to_char(buf, ch);

        for (gch = char_list; gch != nullptr; gch = gch->next) {
            if (is_same_group(gch, ch)) {
                snprintf(buf, sizeof(buf), "[%2d %s] %-16s %4d/%4d hp %4d/%4d mana %4d/%4d mv %5ld xp\n\r", gch->level,
                         IS_NPC(gch) ? "Mob" : class_table[gch->class_num].who_name, capitalize(PERS(gch, ch)),
                         gch->hit, gch->max_hit, gch->mana, gch->max_mana, gch->move, gch->max_move, gch->exp);
                send_to_char(buf, ch);
            }
        }
        return;
    }

    if ((victim = get_char_room(ch, arg)) == nullptr) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (ch->master != nullptr || (ch->leader != nullptr && ch->leader != ch)) {
        send_to_char("But you are following someone else!\n\r", ch);
        return;
    }

    if (victim->master != ch && ch != victim) {
        act("|W$N|w isn't following you.", ch, nullptr, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM)) {
        send_to_char("You can't remove charmed mobs from your group.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        act("You like your master too much to leave $m!", ch, nullptr, victim, TO_VICT);
        return;
    }

    if (is_same_group(victim, ch) && ch != victim) {
        victim->leader = nullptr;
        act("$n removes $N from $s group.", ch, nullptr, victim, TO_NOTVICT);
        act("$n removes you from $s group.", ch, nullptr, victim, TO_VICT);
        act("You remove $N from your group.", ch, nullptr, victim, TO_CHAR);
        return;
    }
    /*
        if ( ch->level - victim->level < -8
        ||   ch->level - victim->level >  8 )
        {
         act( "$N cannot join $n's group.",     ch, nullptr, victim, TO_NOTVICT );
         act( "You cannot join $n's group.",    ch, nullptr, victim, TO_VICT    );
         act( "$N cannot join your group.",     ch, nullptr, victim, TO_CHAR    );
         return;
        }
    */

    victim->leader = ch;
    act("$N joins $n's group.", ch, nullptr, victim, TO_NOTVICT);
    act("You join $n's group.", ch, nullptr, victim, TO_VICT);
    act("$N joins your group.", ch, nullptr, victim, TO_CHAR);
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount;
    int share;
    int extra;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Split how much?\n\r", ch);
        return;
    }

    amount = atoi(arg);

    if (amount < 0) {
        send_to_char("Your group wouldn't like that.\n\r", ch);
        return;
    }

    if (amount == 0) {
        send_to_char("You hand out zero coins, but no one notices.\n\r", ch);
        return;
    }

    if (ch->gold < amount) {
        send_to_char("You don't have that much gold.\n\r", ch);
        return;
    }

    members = 0;
    for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
        if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
            members++;
    }

    if (members < 2) {
        send_to_char("You'd share the gold if there was someone here to split it with!\n\r", ch);
        return;
    }

    share = amount / members;
    extra = amount % members;

    if (share == 0) {
        send_to_char("Don't even bother, cheapskate.\n\r", ch);
        return;
    }

    ch->gold -= amount;
    ch->gold += share + extra;

    snprintf(buf, sizeof(buf), "You split %d gold coins.  Your share is %d gold coins.\n\r", amount, share + extra);
    send_to_char(buf, ch);

    snprintf(buf, sizeof(buf), "$n splits %d gold coins.  Your share is %d gold coins.", amount, share);

    for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
        if (gch != ch && is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM)) {
            act(buf, ch, nullptr, gch, TO_VICT);
            gch->gold += share;
        }
    }
}

void do_gtell(CHAR_DATA *ch, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;

    if (argument[0] == '\0') {
        send_to_char("|cTell your group what?|w\n\r", ch);
        return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL)) {
        send_to_char("|cYour message didn't get through!|w\n\r", ch);
        return;
    }

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
    snprintf(buf, sizeof(buf), "|CYou tell the group '%s|C'|w.\n\r", argument);
    send_to_char(buf, ch);
    snprintf(buf, sizeof(buf), "|C%s tells the group '%s|C'|w.\n\r", ch->name, argument);
    for (gch = char_list; gch != nullptr; gch = gch->next) {
        if (is_same_group(gch, ch) && (gch != ch))
            send_to_char(buf, gch);
    }
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch) {
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
void chatperform(CHAR_DATA *to_npc, CHAR_DATA *from_player, const char *msg) {
    char response_buf[MaxChatReplyLength];
    if (!IS_NPC(to_npc) || (from_player != nullptr && IS_NPC(from_player)))
        return; /* failsafe */
    const char *reply = dochat(response_buf, from_player ? from_player->name : "you", msg, to_npc->name);
    if (reply) {
        switch (reply[0]) {
        case '\0': break;
        case '"': /* say message */ do_say(to_npc, reply + 1); break;
        case ':': /* do emote */ do_emote(to_npc, reply + 1); break;
        case '!': /* do command */ interpret(to_npc, reply + 1); break;
        default: /* say or tell */
            if (from_player == nullptr) {
                do_say(to_npc, reply);
            } else {
                act("$N tells you '$t'.", from_player, reply, to_npc, TO_CHAR);
                from_player->reply = to_npc;
            }
        }
    }
}

void chatperformtoroom(const char *text, CHAR_DATA *ch) {
    CHAR_DATA *vch;
    if (IS_NPC(ch))
        return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
        if (IS_NPC(vch) && IS_SET(vch->pIndexData->act, ACT_TALKATIVE) && IS_AWAKE(vch)) {
            if (number_percent() > 66) /* less spammy - Fara */
                chatperform(vch, ch, text);
        }
}
