/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  magic.h:  wild and wonderful spells                                  */
/*                                                                       */
/*************************************************************************/
#pragma once

struct Char;
struct Object;

int mana_for_spell(const Char *ch, const int sn);
int slot_lookup(int slot);
bool saves_spell(int level, const Char *victim);
void obj_cast_spell(int sn, int level, Char *ch, Char *victim, Object *obj);
bool check_dispel(int dis_level, Char *victim, int spell_num);

void spell_psy_tornado(int spell_num, int level, Char *ch, void *vo);
void spell_insanity(int spell_num, int level, Char *ch, void *vo);
void spell_null(int spell_num, int level, Char *ch, void *vo);
void spell_acid_blast(int spell_num, int level, Char *ch, void *vo);
void spell_acid_wash(int spell_num, int level, Char *ch, void *vo);
void spell_armor(int spell_num, int level, Char *ch, void *vo);
void spell_black_death(int spell_num, int level, Char *ch, void *vo);
void spell_bless(int spell_num, int level, Char *ch, void *vo);
void spell_blindness(int spell_num, int level, Char *ch, void *vo);
void spell_burning_hands(int spell_num, int level, Char *ch, void *vo);
void spell_call_lightning(int spell_num, int level, Char *ch, void *vo);
void spell_calm(int spell_num, int level, Char *ch, void *vo);
void spell_cancellation(int spell_num, int level, Char *ch, void *vo);
void spell_cause_critical(int spell_num, int level, Char *ch, void *vo);
void spell_cause_light(int spell_num, int level, Char *ch, void *vo);
void spell_cause_serious(int spell_num, int level, Char *ch, void *vo);
void spell_change_sex(int spell_num, int level, Char *ch, void *vo);
void spell_chain_lightning(int spell_num, int level, Char *ch, void *vo);
void spell_charm_person(int spell_num, int level, Char *ch, void *vo);
void spell_chill_touch(int spell_num, int level, Char *ch, void *vo);
void spell_colour_spray(int spell_num, int level, Char *ch, void *vo);
void spell_continual_light(int spell_num, int level, Char *ch, void *vo);
void spell_control_weather(int spell_num, int level, Char *ch, void *vo);
void spell_create_food(int spell_num, int level, Char *ch, void *vo);
void spell_create_spring(int spell_num, int level, Char *ch, void *vo);
void spell_create_water(int spell_num, int level, Char *ch, void *vo);
void spell_cure_blindness(int spell_num, int level, Char *ch, void *vo);
void spell_cure_critical(int spell_num, int level, Char *ch, void *vo);
void spell_cure_disease(int spell_num, int level, Char *ch, void *vo);
void spell_cure_light(int spell_num, int level, Char *ch, void *vo);
void spell_cure_poison(int spell_num, int level, Char *ch, void *vo);
void spell_cure_serious(int spell_num, int level, Char *ch, void *vo);
void spell_curse(int spell_num, int level, Char *ch, void *vo);
void spell_damnation(int spell_num, int level, Char *ch, void *vo);
void spell_demonfire(int spell_num, int level, Char *ch, void *vo);
void spell_exorcise(int spell_num, int level, Char *ch, void *vo);
void spell_detect_evil(int spell_num, int level, Char *ch, void *vo);
void spell_detect_hidden(int spell_num, int level, Char *ch, void *vo);
void spell_detect_invis(int spell_num, int level, Char *ch, void *vo);
void spell_detect_magic(int spell_num, int level, Char *ch, void *vo);
void spell_detect_poison(int spell_num, int level, Char *ch, void *vo);
void spell_dispel_evil(int spell_num, int level, Char *ch, void *vo);
void spell_dispel_good(int spell_num, int level, Char *ch, void *vo);
void spell_dispel_magic(int spell_num, int level, Char *ch, void *vo);
void spell_earthquake(int spell_num, int level, Char *ch, void *vo);
void spell_enchant_armor(int spell_num, int level, Char *ch, void *vo);
void spell_enchant_weapon(int spell_num, int level, Char *ch, void *vo);
void spell_energy_drain(int spell_num, int level, Char *ch, void *vo);
void spell_faerie_fire(int spell_num, int level, Char *ch, void *vo);
void spell_faerie_fog(int spell_num, int level, Char *ch, void *vo);
void spell_fireball(int spell_num, int level, Char *ch, void *vo);
void spell_flamestrike(int spell_num, int level, Char *ch, void *vo);
void spell_fly(int spell_num, int level, Char *ch, void *vo);
void spell_frenzy(int spell_num, int level, Char *ch, void *vo);
void spell_gate(int spell_num, int level, Char *ch, void *vo);
void spell_giant_strength(int spell_num, int level, Char *ch, void *vo);
void spell_harm(int spell_num, int level, Char *ch, void *vo);
void spell_haste(int spell_num, int level, Char *ch, void *vo);
void spell_lethargy(int spell_num, int level, Char *ch, void *vo);
void spell_heal(int spell_num, int level, Char *ch, void *vo);
void spell_holy_word(int spell_num, int level, Char *ch, void *vo);
void spell_identify(int spell_num, int level, Char *ch, void *vo);
void spell_infravision(int spell_num, int level, Char *ch, void *vo);
void spell_invis(int spell_num, int level, Char *ch, void *vo);
void spell_know_alignment(int spell_num, int level, Char *ch, void *vo);
void spell_remove_alignment(int spell_num, int level, Char *ch, void *vo);
void spell_remove_invisible(int spell_num, int level, Char *ch, void *vo);
void spell_lightning_bolt(int spell_num, int level, Char *ch, void *vo);
void spell_locate_object(int spell_num, int level, Char *ch, void *vo);
void spell_magic_missile(int spell_num, int level, Char *ch, void *vo);
void spell_mass_healing(int spell_num, int level, Char *ch, void *vo);
void spell_mass_invis(int spell_num, int level, Char *ch, void *vo);
void spell_pass_door(int spell_num, int level, Char *ch, void *vo);
void spell_octarine_fire(int spell_num, int level, Char *ch, void *vo);
void spell_plague(int spell_num, int level, Char *ch, void *vo);
void spell_talon(int spell_num, int level, Char *ch, void *vo);
void spell_poison(int spell_num, int level, Char *ch, void *vo);
void spell_portal(int spell_num, int level, Char *ch, void *vo);
void spell_protect_container(int spell_num, int level, Char *ch, void *vo);
void spell_protection_evil(int spell_num, int level, Char *ch, void *vo);
void spell_protection_good(int spell_num, int level, Char *ch, void *vo);
void spell_refresh(int spell_num, int level, Char *ch, void *vo);
void spell_remove_curse(int spell_num, int level, Char *ch, void *vo);
void spell_regeneration(int spell_num, int level, Char *ch, void *vo);
void spell_sanctuary(int spell_num, int level, Char *ch, void *vo);
void spell_shocking_grasp(int spell_num, int level, Char *ch, void *vo);
void spell_shield(int spell_num, int level, Char *ch, void *vo);
void spell_sleep(int spell_num, int level, Char *ch, void *vo);
void spell_stone_skin(int spell_num, int level, Char *ch, void *vo);
void spell_summon(int spell_num, int level, Char *ch, void *vo);
void spell_tame_lightning(int spell_num, int level, Char *ch, void *vo);
void spell_teleport(int spell_num, int level, Char *ch, void *vo);
void spell_teleport_object(int spell_num, int level, Char *ch, void *vo);
void spell_undo_spell(int spell_num, int level, Char *ch, void *vo);
void spell_vampire(int spell_num, int level, Char *ch, void *vo);
void spell_vorpal(int spell_num, int level, Char *ch, void *vo);
void spell_ventriloquate(int spell_num, int level, Char *ch, void *vo);
void spell_venom(int spell_num, int level, Char *ch, void *vo);
void spell_weaken(int spell_num, int level, Char *ch, void *vo);
void spell_word_of_recall(int spell_num, int level, Char *ch, void *vo);
void spell_acid_breath(int spell_num, int level, Char *ch, void *vo);
void spell_fire_breath(int spell_num, int level, Char *ch, void *vo);
void spell_frost_breath(int spell_num, int level, Char *ch, void *vo);
void spell_gas_breath(int spell_num, int level, Char *ch, void *vo);
void spell_lightning_breath(int spell_num, int level, Char *ch, void *vo);
void spell_general_purpose(int spell_num, int level, Char *ch, void *vo);
void spell_high_explosive(int spell_num, int level, Char *ch, void *vo);
void spell_raise_dead(int spell_num, int level, Char *ch, void *vo);

void explode_bomb(Object *bomb, Char *ch, Char *thrower);
