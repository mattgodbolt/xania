/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  fight.c:  combat! (yes Xania can be a violent place at times)        */
/*                                                                       */
/*************************************************************************/



#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include "sys/types.h"
#else
#include <sys/types.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "merc.h"

/* For checking whether a duel is active etc. */
#include "challeng.h"

#define MAX_DAMAGE_MESSAGE 32

/* command procedures needed */
DECLARE_DO_FUN(do_emote		);
DECLARE_DO_FUN(do_berserk	);
DECLARE_DO_FUN(do_bash		);
DECLARE_DO_FUN(do_trip		);
DECLARE_DO_FUN(do_dirt		);
DECLARE_DO_FUN(do_flee		);
DECLARE_DO_FUN(do_kick		);
DECLARE_DO_FUN(do_disarm	);
DECLARE_DO_FUN(do_get		);
DECLARE_DO_FUN(do_recall	);
DECLARE_DO_FUN(do_duel          );
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_sacrifice	);
DECLARE_DO_FUN(do_transfer      );
DECLARE_SPELL_FUN(spell_poison     );
DECLARE_SPELL_FUN(spell_plague     );
/*
 * Local functions.
 */
void    do_backstab     args( ( CHAR_DATA *ch, char *argument ) );
void	check_assist	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_dodge	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	check_killer	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_parry	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    check_shield_block args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	dam_message	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
				int dt, int dam_type,  bool immune ) );
void    announce        args( ( char *, CHAR_DATA *ch) );
void    do_immtalk      args( ( CHAR_DATA *ch, char *) );
void	death_cry	args( ( CHAR_DATA *ch ) );
void	group_gain	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int	xp_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim,
int total_levels ) );
bool	is_safe		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	make_corpse	args( ( CHAR_DATA *ch ) );
void	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void    mob_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void	raw_kill	args( ( CHAR_DATA *victim ) );
void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	disarm		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void    lose_level      args( ( CHAR_DATA *ch ) );

/*extern void thrown_off( CHAR_DATA *ch, CHAR_DATA *pet);*/

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
     CHAR_DATA *ch;
     CHAR_DATA *ch_next;
     CHAR_DATA *victim;

     for ( ch = char_list; ch != NULL; ch = ch->next )
     {
	  ch_next	= ch->next;

	  if ( ( victim = ch->fighting ) == NULL || ch->in_room == NULL )
	       continue;

	  if ( IS_AWAKE(ch) && ch->in_room == victim->in_room )
	       multi_hit( ch, victim, TYPE_UNDEFINED );
	  else
	       stop_fighting( ch, FALSE );

	  if ( ( victim = ch->fighting ) == NULL )
	       continue;

          /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
	  mprog_hitprcnt_trigger( ch, victim );
	  mprog_fight_trigger( ch, victim );

	  /*
	   * Fun for the whole family!
	   */
	  check_assist(ch,victim);
     }

     return;
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
   CHAR_DATA *rch, *rch_next;

   for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
   {
      rch_next = rch->next_in_room;

      if (IS_AWAKE(rch) && rch->fighting == NULL)
      {

         /* quick check for ASSIST_PLAYER */
         if (!IS_NPC(ch) && IS_NPC(rch)
         && IS_SET(rch->off_flags,ASSIST_PLAYERS)
         &&  rch->level + 6 > victim->level)
         {
			// moog: copied from do_emote:
		    act( "|W$n screams and attacks!|w", rch, NULL, NULL, TO_ROOM );
		    act( "|W$n screams and attacks!|w", rch, NULL, NULL, TO_CHAR );
            multi_hit(rch,victim,TYPE_UNDEFINED);
            continue;
         }

         /* PCs next */
         if (!IS_NPC(ch) || IS_AFFECTED(ch,AFF_CHARM))
         {
            if ( ( (!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
            ||     IS_AFFECTED(rch,AFF_CHARM))
            &&   is_same_group(ch,rch) )
               multi_hit (rch,victim,TYPE_UNDEFINED);

            continue;
         }

         /* now check the NPC cases */

         if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM))

         {
            if ( (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))

            ||   (IS_NPC(rch) && rch->race == ch->race
            && IS_SET(rch->off_flags,ASSIST_RACE))

            ||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
            &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
            ||  (IS_EVIL(rch)    && IS_EVIL(ch))
            ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))

            ||   (rch->pIndexData == ch->pIndexData
            && IS_SET(rch->off_flags,ASSIST_VNUM)))

            {
               CHAR_DATA *vch;
               CHAR_DATA *target;
               int number;

               if (number_bits(1) == 0)
                  continue;

               target = NULL;
               number = 0;
               for (vch = ch->in_room->people; vch; vch = vch->next)
               {
                  if (can_see(rch,vch)
                  &&  is_same_group(vch,victim)
                  &&  number_range(0,number) == 0)
                  {
                     target = vch;
                     number++;
                  }
               }

               if (target != NULL)
               {
				  // moog: copied from do_emote:
				  act( "|W$n screams and attacks!|w", rch, NULL, NULL, TO_ROOM );
  				  act( "|W$n screams and attacks!|w", rch, NULL, NULL, TO_CHAR );
                  multi_hit(rch,target,TYPE_UNDEFINED);
               }
            }
         }
      }
   }
}


/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
   OBJ_DATA *wield;
   int     chance;

   if (!ch->in_room || !victim->in_room)
      return;
   /* If in combat room */

   if ( ch->in_room->vnum == CHAL_ROOM || victim->in_room->vnum == CHAL_ROOM )
   {
      if (dt == 100)
         dt = TYPE_UNDEFINED;
      else
         if (ch->fighting == NULL)
            return;
   }

   /* if mob has sentient bit set, set victim name in memory */
   if (IS_SET (victim->act, ACT_SENTIENT)
   && !IS_NPC (ch))
   {
      free_string (victim->sentient_victim);
      victim->sentient_victim = str_dup (ch->name);
   };

   /* decrement the wait */
   if (ch->desc == NULL)
      ch->wait = UMAX(0,ch->wait - PULSE_VIOLENCE);

   /* no attacks for stunnies -- just a check */
   if (ch->position < POS_RESTING)
      return;

   if ( IS_NPC(ch) )
   {
      mob_hit(ch,victim,dt);
      return;
   }

   one_hit( ch, victim, dt );

   if (( ch->in_room != NULL && victim->in_room != NULL ) &&
     (ch->in_room->vnum == CHAL_ROOM && victim->in_room->vnum == CHAL_ROOM))
   {
      int percent;
      char wound[100];
      char buf[MAX_STRING_LENGTH];

      if (victim->max_hit > 0)
         percent = victim->hit * 100 / victim->max_hit;
      else
      percent = -1;

      if (percent >= 100)
         sprintf(wound,"is in excellent condition.");
      else if (percent >= 90)
         sprintf(wound,"has a few scratches.");
      else if (percent >= 75)
         sprintf(wound,"has some small wounds and bruises.");
      else if (percent >= 50)
         sprintf(wound,"has quite a few wounds.");
      else if (percent >= 30)
         sprintf(wound,"has some big nasty wounds and scratches.");
      else if (percent >= 15)
         sprintf(wound,"looks pretty hurt.");
      else if (percent >= 0)
         sprintf(wound,"is in awful condition.");
      else
      sprintf(wound,"is bleeding to death.");

      sprintf(buf,"%s %s",
      IS_NPC(victim) ? victim->short_descr : victim->name,wound);
      buf[0] = UPPER(buf[0]);
      act (buf, ch, NULL, victim, TO_NOTVICT);
   }

   if (ch->fighting != victim)
      return;

   wield = get_eq_char( ch, WEAR_WIELD );

   if ((IS_AFFECTED(ch,AFF_HASTE)) && !(IS_AFFECTED(ch,AFF_LETHARGY)))
      one_hit(ch,victim,dt);

   if ( ch->fighting != victim || dt == gsn_backstab )
      return;

   chance = get_skill(ch,gsn_second_attack)/2;
   if ( number_percent( ) < chance )
   {
      one_hit( ch, victim, dt );
      check_improve(ch,gsn_second_attack,TRUE,5);
      if ( ch->fighting != victim )
         return;
   }

   chance = get_skill(ch,gsn_third_attack)/4;
   if ( number_percent( ) < chance )
   {
      one_hit( ch, victim, dt );
      check_improve(ch,gsn_third_attack,TRUE,6);
      if ( ch->fighting != victim )
         return;
   }

   return;
}

/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
   int chance, number;
   CHAR_DATA *vch, *vch_next;

   if ( IS_SET(ch->off_flags, OFF_BACKSTAB)
   && (ch->fighting == NULL)
   && (get_eq_char(ch, WEAR_WIELD) != NULL)
   && (victim->hit == victim->max_hit) )
      one_hit(ch,victim,gsn_backstab);
   else
   one_hit(ch,victim,dt);

   if (ch->fighting != victim)
      return;

   /* Area attack -- BALLS nasty! */

   if (IS_SET(ch->off_flags,OFF_AREA_ATTACK))
   {
      for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
      {
         vch_next = vch->next;
         if ((vch != victim && vch->fighting == ch))
            one_hit(ch,vch,dt);
      }
   }

   if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
      one_hit(ch,victim,dt);

   if (ch->fighting != victim || dt == gsn_backstab)
      return;

   chance = get_skill(ch,gsn_second_attack)/2;
   if (number_percent() < chance)
   {
      one_hit(ch,victim,dt);
      if (ch->fighting != victim)
         return;
   }

   chance = get_skill(ch,gsn_third_attack)/4;
   if (number_percent() < chance)
   {
      one_hit(ch,victim,dt);
      if (ch->fighting != victim)
         return;
   }

   /* oh boy!  Fun stuff! */

   if (ch->wait > 0)
      return;

   number = number_range(0,2);

   if (number == 1 && IS_SET(ch->act,ACT_MAGE))
      /*  { mob_cast_mage(ch,victim); return; } */ ;

   if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
      /* { mob_cast_cleric(ch,victim); return; } */ ;

   /* now for the skills */

   number = number_range(0,7);

   switch(number)
   {
      case (0) :
      if (IS_SET(ch->off_flags,OFF_BASH))
         do_bash(ch,"");
      break;

      case (1) :
      if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK))
         do_berserk(ch,"");
      break;


      case (2) :
      if (IS_SET(ch->off_flags,OFF_DISARM)
      || (get_weapon_sn(ch) != gsn_hand_to_hand
      && (IS_SET(ch->act,ACT_WARRIOR)
      ||  IS_SET(ch->act,ACT_THIEF))))
      {
         if (IS_AFFECTED(victim, AFF_TALON))
         {
            act("$n tries to disarm you, but your talon like grip stops them!",
            ch,NULL,victim,TO_VICT);
            act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
         }
         else
            do_disarm(ch,"");
         break;
      }
      case (3) :
      if (IS_SET(ch->off_flags,OFF_KICK))
         do_kick(ch,"");
      break;

      case (4) :
      if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
         do_dirt(ch,"");
      break;

      case (5) :
      if (IS_SET(ch->off_flags,OFF_TAIL))
         /* do_tail(ch,"") */ ;
      break;

      case (6) :
      if (IS_SET(ch->off_flags,OFF_TRIP))
         do_trip(ch,"");
      break;

      case (7) :
      if (IS_SET(ch->off_flags,OFF_CRUSH))
         /* do_crush(ch,"") */ ;
      break;
   }
}


/*
 * Hit one guy once.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
   OBJ_DATA *wield;
   int victim_ac;
   int thac0;
   int thac0_00;
   int thac0_32;
   int dam;
   int diceroll;
   int sn,skill;
   int dam_type;
   bool self_hitting = FALSE;
   AFFECT_DATA *af;

   sn = -1;


   /* just in case */
   if (victim == ch || ch == NULL || victim == NULL)
      return;

   /*
    * Check for insanity
    */
   if ((af = find_affect (ch, gsn_insanity)) != NULL) {
	int chance = URANGE (2, af->level / 5, 5);
	if (number_percent() < chance) {
	     act("In your confused state, you attack yourself!", ch, NULL, victim, TO_CHAR);
	     act("$n stumbles and in a confused state, hits $mself.", ch, NULL, victim, TO_ROOM);
	     victim = ch;
	     self_hitting = TRUE;
	}
   }

   /*
        * Can't beat a dead char!
        * Guard against weird room-leavings.
        */
   if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
      return;

   /*
        * Figure out the type of damage message.
        */
   wield = get_eq_char( ch, WEAR_WIELD );

   if ( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if ( wield != NULL && wield->item_type == ITEM_WEAPON )
         dt += wield->value[3];
      else
      dt += ch->dam_type;
   }

   if (dt < TYPE_HIT)
      if (wield != NULL)
         dam_type = attack_table[wield->value[3]].damage;
   else
   dam_type = attack_table[ch->dam_type].damage;
   else
   dam_type = attack_table[dt - TYPE_HIT].damage;

   if (dam_type == -1)
      dam_type = DAM_BASH;

   /* get the weapon skill */
   sn = get_weapon_sn(ch);
   skill = 20 + get_weapon_skill(ch,sn);

   /*
        * Calculate to-hit-armor-class-0 versus armor.
        */
   if ( IS_NPC(ch) )
   {
      thac0_00 = 20;
      thac0_32 = -4;   /* as good as a thief */

      if (IS_SET(ch->act,ACT_WARRIOR))
         thac0_32 = -10;
      else if (IS_SET(ch->act,ACT_THIEF))
         thac0_32 = -4;
      else if (IS_SET(ch->act,ACT_CLERIC))
         thac0_32 = 2;
      else if (IS_SET(ch->act,ACT_MAGE))
         thac0_32 = 6;
   }
   else
   {
      thac0_00 = class_table[ch->class].thac0_00;
      thac0_32 = class_table[ch->class].thac0_32;
   }

   thac0  = interpolate( ch->level, thac0_00, thac0_32 );

   thac0 -= GET_HITROLL(ch) * skill/100;
   thac0 += 5 * (100 - skill) / 100;

   if (dt == gsn_backstab)
     /* changed from
      * thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));
      * to this....Fara 28/8/96
      */
     thac0 -=  ( 5 * ( 100 - get_skill(ch, gsn_backstab))) ;

   switch(dam_type)
   {
      case(DAM_PIERCE):
      victim_ac = GET_AC(victim,AC_PIERCE)/10;
      break;
      case(DAM_BASH):
      victim_ac = GET_AC(victim,AC_BASH)/10;
      break;
      case(DAM_SLASH):
      victim_ac = GET_AC(victim,AC_SLASH)/10;
      break;
   default:
      victim_ac = GET_AC(victim,AC_EXOTIC)/10;
      break;
   };

   if (victim_ac < -15)
      victim_ac = (victim_ac + 15) / 5 - 15;

   if ( !can_see( ch, victim ) )
      victim_ac -= 4;

   if ( victim->position < POS_FIGHTING)
      victim_ac += 4;

   if (victim->position < POS_RESTING)
      victim_ac += 6;

   /*
        * The moment of excitement!
        */
   while ( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if ( diceroll == 0
   || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
   {
      /* Miss. */
      damage( ch, victim, 0, dt, dam_type );
      tail_chain( );
      return;
   }

   /*
        * Hit.
        * Calc damage.
        */
   if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
      if (!ch->pIndexData->new_format)
      {
         dam = number_range( ch->level / 2, ch->level * 3 / 2 );
         if ( wield != NULL )
            dam += dam / 2;
      }
      else
         dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);

   else
   {
      if (sn != -1)
         check_improve(ch,sn,TRUE,5);
      if ( wield != NULL )
      {
         if (wield->pIndexData->new_format)
            dam = dice(wield->value[1],wield->value[2]) * skill/100;
         else
         dam = number_range( wield->value[1] * skill/100,
         wield->value[2] * skill/100);

         /* Sharp weapon flag implemented by Wandera */
         if ( (wield != NULL) && (wield->item_type == ITEM_WEAPON) && !self_hitting)
            if(IS_SET(wield->value[4], WEAPON_SHARP) &&
            number_percent()>98)
            {
               dam *= 2;
               act ("Sunlight glints off your sharpened blade!",
               ch, NULL, victim, TO_CHAR );
               act ("Sunlight glints off $n's sharpened blade!",
               ch, NULL, victim, TO_NOTVICT );
               act ("Sunlight glints off $n's sharpened blade!",
               ch, NULL, victim, TO_VICT );
            }


         /* Vorpal weapon flag implemented by Wandera and Death*/
         /* If weapon is vorpal the change of damage being *4  */
         if ( (wield != NULL) && (wield->item_type == ITEM_WEAPON) ) {
            if(IS_SET(wield->value[4], WEAPON_VORPAL)) {
               if (wield->pIndexData->new_format) {
                  if (dam == (1 + wield->value[2]) * wield->value[1]/2) {
                     dam *= 4;
                     bug("one_hit:QUAD_DAM with %s [%d] by %s",
                     wield->name, wield->pIndexData->vnum, ch->name);
                     act("With a blood curdling scream you leap forward swinging\n\ryour weapon in a great arc.", ch, NULL, victim, TO_CHAR);
                     act("$n screams and leaps forwards swinging $s weapon in a great arc.", ch, NULL, victim, TO_NOTVICT);
                     act("$n screams and leaps towards you swinging $s weapon in a great arc.", ch, NULL, victim, TO_VICT);
                  }
               } else {
                  if (dam == (wield->value[1] + wield->value[2])/2) {
                     dam *= 4;
                     send_to_char("With a blood curdling scream you leap forward swinging your weapon in a great    arc.", ch);
                     act("$n screams and leaps forwards swinging his weapon in a great arc.", ch, NULL, victim, TO_ROOM);
                     act("$n screams and leaps towards you swinging his weapon in a great arc.", ch, NULL, victim, TO_VICT);
                  }
               }
            }
         }

         if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
            dam = (int)((float)dam * 21.f/20.f);
      } else
         dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
   }

   /*
        * Bonuses.
        */
   if ( get_skill(ch,gsn_enhanced_damage) > 0 )
   {
      diceroll = number_percent();
      if (diceroll <= get_skill(ch,gsn_enhanced_damage))
      {
         check_improve(ch,gsn_enhanced_damage,TRUE,6);
         dam += dam * diceroll/100;
      }
   }

   if ( !IS_AWAKE(victim) )
      dam *= 2;
   else if (victim->position < POS_FIGHTING)
      dam = dam * 3 / 2;

   if ( dt == gsn_backstab && wield != NULL) {
      if ( wield->value[0] != 2 )
         dam *= 2 + ch->level / 10;
      else
         dam *= 2 + ch->level / 8;
   }

   dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

   if ( dam <= 0 )
      dam = 1;
   if ( dam > 1000)
      dam = 1000;

   damage( ch, victim, dam, dt, dam_type );

   if ( (wield != NULL) && (wield->item_type == ITEM_WEAPON) )
      if( (IS_SET(wield->value[4], WEAPON_POISONED)) &&
      (!IS_AFFECTED(victim, AFF_POISON)) )
      {
         if(number_percent() > 75)
         {
            int p_sn = skill_lookup("poison");
            spell_poison ( p_sn, wield->level, ch, victim );
         }
      }

   if ( (wield != NULL) && (wield->item_type == ITEM_WEAPON) )
      if ( ( IS_SET(wield->value[4], WEAPON_PLAGUED)) &&
      (!IS_AFFECTED(victim, AFF_PLAGUE)) )
      {
         if(number_percent() > 75)
         {
            int p_sn = skill_lookup("plague");
            spell_plague ( p_sn, wield->level, ch, victim );
         }
      }

   tail_chain( );
   return;
}


/*
 * Inflict damage from a hit.
 */
bool damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type )
{
     int temp;
     CHAR_DATA *mob;
     CHAR_DATA *mob_next;
     OBJ_DATA *corpse;
     OBJ_DATA *wield;
     AFFECT_DATA *octarineFire;
     bool immune;
     
     if ( victim->position == POS_DEAD )
	  return FALSE;
     
     /*
      * Stop up any residual loopholes.
      */
     if ( dam > 1000 )
     {
	  bug( "Damage: %d: more than 1000 points!", dam );
	  dam = 1000;
	  if (!IS_IMMORTAL(ch))
	  {
	       OBJ_DATA *obj;
	       obj = get_eq_char( ch, WEAR_WIELD );
	       if (obj != NULL) /*Rohans try at stopping crashes!*/
	       {
		    send_to_char("|WYou really shouldn't cheat.|r\n\r",ch);
		    extract_obj(obj);
	       }
	  }
	  
     }
     
     
     /* damage reduction */
     if ( dam > 30)
	  dam = (dam - 30)/2 + 30;
     if ( dam > 75)
	  dam = (dam - 75)/2 + 75;
     
     /*
      * New code to make octarine fire a little bit more effective
      */
     if ((octarineFire = find_affect (victim, gsn_octarine_fire)) != NULL) {
	  
	  /*
	   * Increase by (level / 8) %
	   */
	  dam += (int)((float)dam * (float)octarineFire->level / 800.f);
     }


     if ( victim != ch )
     {
	  /*
	   * Certain attacks are forbidden.
	   * Most other attacks are returned.
	   */
	  if ( is_safe( ch, victim ) )
	       return FALSE;
	  check_killer( ch, victim );

	  if ( victim->position > POS_STUNNED )
	  {
	       if ( victim->fighting == NULL )
		    set_fighting( victim, ch );
	       if (victim->timer <= 4)
		    victim->position = POS_FIGHTING;
	  }

	  if ( victim->position > POS_STUNNED )
	  {
	       if ( ch->fighting == NULL )
		    set_fighting( ch, victim );

	       /*
		* If victim is charmed, ch might attack victim's master.
		*/
	       if ( IS_NPC(ch)
		    &&   IS_NPC(victim)
		    &&   IS_AFFECTED(victim, AFF_CHARM)
		    &&   victim->master != NULL
		    &&   victim->master->in_room == ch->in_room
		    &&   number_bits( 3 ) == 0 )
	       {
		    stop_fighting( ch, FALSE );
		    multi_hit( ch, victim->master, TYPE_UNDEFINED );
		    return FALSE;
	       }
	  }

	  /*
      	 * More charm stuff.
      	 */
	  if ( victim->master == ch )
	       stop_follower( victim );
     }

     /*
        * Inviso attacks ... not.
        */
     if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
     {
	  affect_strip( ch, gsn_invis );
	  affect_strip( ch, gsn_mass_invis );
	  REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
	  act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
     }

     /*
        * Damage modifiers.
        */
     if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
	  dam /= 2;

     if ( IS_AFFECTED(victim, AFF_PROTECTION_EVIL) && IS_EVIL(ch) )
	  dam -= dam / 4;

     if ( IS_AFFECTED(victim, AFF_PROTECTION_GOOD) && IS_GOOD(ch) )
	  dam -= dam / 4;

     immune = FALSE;


     /*
        * Check for parry, shield block and dodge.
        */
     if ( dt >= TYPE_HIT && ch != victim)
     {
	  if ( check_parry( ch, victim ) )
	       return FALSE;
	  if ( check_dodge( ch, victim ) )
	       return FALSE;
	  if ( check_shield_block( ch, victim ) )
	       return FALSE;
     }

     switch(check_immune(victim,dam_type))
     {
     case(IS_IMMUNE):
	  immune = TRUE;
	  dam = 0;
	  break;
     case(IS_RESISTANT):
	  dam -= dam/3;
	  break;
     case(IS_VULNERABLE):
	  dam += dam/2;
	  break;
     }
     if ( ( (wield = get_eq_char(ch, WEAR_WIELD)) != NULL) &&
	  check_material_vulnerability(victim, wield))
	  dam += dam/2;
   
     dam_message( ch, victim, dam, dt, dam_type, immune  );

     if (dam == 0)
	  return FALSE;

   /*
        * Hurt the victim.
        * Inform the victim of his new state.
        */
     victim->hit -= dam;
     if ( !IS_NPC(victim)
	  &&   victim->level >= LEVEL_IMMORTAL
	  &&   victim->hit < 1 )
	  victim->hit = 1;
     update_pos( victim );

     wield = get_eq_char(ch, WEAR_WIELD);

     if ( (wield != NULL) && (wield->item_type == ITEM_WEAPON) &&
	  ( IS_SET(wield->value[4], WEAPON_VAMPIRIC)) )
     {
	  ch->hit += (dam/100)*10;
	  victim->hit -= (dam/100)*10;
     }

     switch( victim->position )
     {
     case POS_MORTAL:
	  act( "|r$n is mortally wounded, and will die soon, if not aided.|w",
	       victim, NULL, NULL, TO_ROOM );
	  send_to_char(
	       "|rYou are mortally wounded, and will die soon, if not aided.|w\n\r",
	       victim );
	  break;

     case POS_INCAP:
	  act( "|r$n is incapacitated and will slowly die, if not aided.|w",
	       victim, NULL, NULL, TO_ROOM );
	  send_to_char(
	       "|rYou are incapacitated and will slowly die, if not aided.|w\n\r",
	       victim );
	  break;

     case POS_STUNNED:
	  act( "|r$n is stunned, but will probably recover.|w",
	       victim, NULL, NULL, TO_ROOM );
	  send_to_char("|rYou are stunned, but will probably recover.|w\n\r",
		       victim );
	  break;

     case POS_DEAD:
	  act( "|R$n is DEAD!!|w", victim, 0, 0, TO_ROOM );
	  send_to_char( "|RYou have been KILLED!!|w\n\r\n\r", victim );
	  break;

     default:
	  if ( dam > victim->max_hit / 4 )
	       send_to_char( "That really did |RHURT|w!\n\r", victim );
	  if ( victim->hit < victim->max_hit / 4 )
	       send_to_char( "You sure are |RBLEEDING|w!\n\r", victim );
	  break;
     }

     /*
        * Sleep spells and extremely wounded folks.
        */
     if ( !IS_AWAKE(victim) )
	  stop_fighting( victim, FALSE );

   /*
        * Payoff for killing things.
        */
     if ( victim->position == POS_DEAD )
     {
	  group_gain( ch, victim );

	  if ( !IS_NPC(victim) )
	  {
	       unsigned int exp_total=(victim->level * exp_per_level(victim, victim->
								     pcdata->points));
	       unsigned int exp_level=exp_per_level(victim, victim->pcdata->points);
	       CHAR_DATA *squib;

	       temp = do_check_chal (victim);
	       if (temp == 1)
		    return TRUE;

	       sprintf( log_buf, "%s killed by %s at %d",
			victim->name,
			(IS_NPC(ch) ? ch->short_descr : ch->name),
			victim->in_room->vnum );
	       log_string( log_buf );
	       sprintf (log_buf, "|P###|w Sadly, %s was killed by %s.",
			victim->name,
			(IS_NPC(ch) ? ch->short_descr : ch->name));
	       announce (log_buf, victim);

	       for ( squib = victim->in_room->people; squib;
		     squib = squib->next_in_room) {
		    if ((IS_NPC(squib)) &&
			(squib->pIndexData->vnum == LESSER_MINION_VNUM)) {
			 act( "$n swings his scythe and ushers $N's soul into the next world.",
			      squib, NULL, victim, TO_ROOM);
			 break;
		    }
	       }


	       /*
		* Dying penalty:
		* 1/2 way back to previous level.
		*/
	       if ( victim->level >= 26 ) {
		    /* Erm...MATT I think this logic is wrong...sorry... =) */
		    /* You will lose (exp_per_level/2), if that's less than */
		    /* (level*exp_per_level) then you lose a level..EASY */
		    if ( victim->exp-(exp_level/2) < (exp_total) )
		    {
			 send_to_char("You lose a level!!  ", victim);
			 victim->level -= 1;
			 lose_level(victim);
		    }
		    gain_exp( victim, -(exp_level / 2));

	       }
	       else
	       {
		    if ( victim->exp > exp_total )
			 gain_exp( victim,
				   -(( exp_level - (( (victim->level + 1) * exp_per_level(victim,victim->pcdata->points) - victim->exp)) )/2));
	       }
	  }
	  else {

	       if (victim->level >= ( ch->level + 30 ) ) {
		    sprintf( log_buf, "|R### %s just killed %s - %d levels above them!|w", (IS_NPC(ch) ? ch->short_descr : ch->name),
			     victim->short_descr,
			     (victim->level - ch->level));
		    do_immtalk (ch, log_buf );
	       }
	  }
	  raw_kill( victim );

	  for (mob = char_list; mob != NULL; mob = mob_next)
	  {
	       mob_next = mob->next;
	       if (IS_NPC (mob))
	       {
		    if (IS_SET (mob->act, ACT_SENTIENT))
		    {
			 if ((mob->sentient_victim != NULL)
			     && !str_cmp (victim->name, mob->sentient_victim))
			 {
			      free_string (mob->sentient_victim);
			      mob->sentient_victim = str_dup ("");
			 }
		    }
	       }
	  }
	  /* RT new auto commands */

	  if ( !IS_NPC(ch) && IS_NPC(victim) )
	  {
	       corpse = get_obj_list( ch, "corpse", ch->in_room->contents );

	       if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
		    corpse && corpse->contains) /* exists and not empty */
		    do_get( ch, "all corpse" );

	       if (IS_SET(ch->act,PLR_AUTOGOLD) &&
		   corpse && corpse->contains  && /* exists and not empty */
		   !IS_SET(ch->act,PLR_AUTOLOOT))
		    do_get(ch, "gold corpse");

	       if ( IS_SET(ch->act, PLR_AUTOSAC) ) {
		    if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains)
			 return TRUE;  /* leave if corpse has treasure */
		    else
			 do_sacrifice( ch, "corpse" );
	       }
	  }

	  return TRUE;
     }

     if ( victim == ch )
	  return TRUE;

   /*
        * Take care of link dead people.
        */
     if ( !IS_NPC(victim) && victim->desc == NULL )
     {
	  if ( number_range( 0, victim->wait ) == 0 )
	  {
	       do_recall( victim, "" );
	       return TRUE;
	  }
     }

     /*
        * Wimp out?
        */
     if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
     {
	  if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
		 &&   victim->hit < victim->max_hit / 5)
	       ||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
		      &&     victim->master->in_room != victim->in_room ) )
	       do_flee( victim, "" );
     }

     if ( !IS_NPC(victim)
	  &&   victim->hit > 0
	  &&   victim->hit <= victim->wimpy
	  &&   victim->wait < PULSE_VIOLENCE / 2 )
	  do_flee( victim, "" );

   /*
      *  Check for being thrown from your horse/other mount
      */
  
     if ((victim->riding != NULL) && (ch != NULL)) {
	  int fallchance;
	  if (IS_NPC(victim)) {
	       fallchance = URANGE(2,victim->level*2,98);
	  }
	  else {
	       fallchance = get_skill_learned (victim, gsn_ride);
	  }
	  switch(dam_type) {
	  case DAM_BASH:
	       break;
	  case DAM_PIERCE:
	  case DAM_SLASH:
	  case DAM_ENERGY:
	  case DAM_HARM:
	       fallchance+=10;
	       break;
	  default:
	       fallchance+=40;
	  }
	  fallchance += (victim->level - ch->level)/4;
	  fallchance = URANGE(20, fallchance, 100);
	  if (number_percent() > fallchance) {
	       /* Oh dear something nasty happened to you ...
		  You fell off your horse! */
	       thrown_off(victim, victim->riding);
	  }
	  else {
	       check_improve(victim,gsn_ride,TRUE,4);
	  }
     }

     tail_chain( );
     return TRUE;
}


bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim )
{

   char buf[MAX_STRING_LENGTH];

   /* no killing in shops hack */
   if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
   {
      send_to_char("The shopkeeper wouldn't like that.\n\r",ch);
      return TRUE;
   }
   /* no killing healers, adepts, etc */
   if (IS_NPC(victim)
   && (IS_SET(victim->act,ACT_TRAIN)
   ||  IS_SET(victim->act,ACT_PRACTICE)
   ||  IS_SET(victim->act,ACT_IS_HEALER)))
   {
      sprintf( buf, "I don't think %s would approve!\n\r", deity_name);
      send_to_char( buf,ch );
      return TRUE;
   }

   /* no fighting in safe rooms */
   if (IS_SET(ch->in_room->room_flags,ROOM_SAFE))
   {
      send_to_char("Not in this room.\n\r",ch);
      return TRUE;
   }

   if (victim->fighting == ch)
      return FALSE;

   if (IS_NPC(ch))
   {
      /* charmed mobs and pets cannot attack players */
      if (!IS_NPC(victim) && (IS_AFFECTED(ch,AFF_CHARM)
      ||  IS_SET(ch->act,ACT_PET)))
         return TRUE;

      return FALSE;
   }

   else /* Not NPC */
   {
      if (IS_IMMORTAL(ch))
         return FALSE;

      /* no pets */
      if (IS_NPC(victim) && IS_SET(victim->act,ACT_PET))
      {
         act("But $N looks so cute and cuddly...",ch,NULL,victim,TO_CHAR);
         return TRUE;
      }

      /* no charmed mobs unless char is the the owner */
      if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master)
      {
         send_to_char("You don't own that monster.\n\r",ch);
         return TRUE;
      }

      /* no player killing */
      if (!IS_NPC(victim) && !fighting_duel(ch, victim) )
      {
         send_to_char("Sorry, player killing is not permitted.\n\r",ch);
         return TRUE;
      }

      return FALSE;
   }
}

bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area )
{
   /* can't zap self (crash bug) */
   if (ch == victim)
      return TRUE;
   /* immortals not hurt in area attacks */
   if (IS_IMMORTAL(victim) &&  area)
      return TRUE;

   /* no killing in shops hack */
   if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
      return TRUE;

   /* no killing healers, adepts, etc */
   if (IS_NPC(victim)
   && (IS_SET(victim->act,ACT_TRAIN)
   ||  IS_SET(victim->act,ACT_PRACTICE)
   ||  IS_SET(victim->act,ACT_IS_HEALER)))
      return TRUE;

   /* no fighting in safe rooms */
   if (IS_SET(ch->in_room->room_flags,ROOM_SAFE))
      return TRUE;

   if (victim->fighting == ch)
      return FALSE;

   if (IS_NPC(ch))
   {
      /* charmed mobs and pets cannot attack players */
      if (!IS_NPC(victim) && (IS_AFFECTED(ch,AFF_CHARM)
      ||  IS_SET(ch->act,ACT_PET)))
         return TRUE;

      /* area affects don't hit other mobiles */
      if (IS_NPC(victim) && area)
         return TRUE;

      return FALSE;
   }

   else /* Not NPC */
   {
      if (IS_IMMORTAL(ch) && !area)
         return FALSE;

      /* no pets */
      if (IS_NPC(victim) && IS_SET(victim->act,ACT_PET))
         return TRUE;

      /* no charmed mobs unless char is the the owner */
      if (IS_AFFECTED(victim,AFF_CHARM) && ch != victim->master)
         return TRUE;

      /* no player killing */
      if (!IS_NPC(victim))
         return TRUE;

      /* cannot use spells if not in same group */
      if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
         return TRUE;

      return FALSE;
   }
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
   /*
        * Follow charm thread to responsible character.
        * Attacking someone's charmed char is hostile!
        */
   while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
      victim = victim->master;

   /*
        * NPC's are fair game.
        * So are killers and thieves.
        */
   if ( IS_NPC(victim)
   ||   IS_SET(victim->act, PLR_KILLER)
   ||   IS_SET(victim->act, PLR_THIEF) )
      return;

   /*
        * Charm-o-rama.
        */
   if ( IS_SET(ch->affected_by, AFF_CHARM) )
   {
      if ( ch->master == NULL )
      {
         char buf[MAX_STRING_LENGTH];

         sprintf( buf, "Check_killer: %s bad AFF_CHARM",
         IS_NPC(ch) ? ch->short_descr : ch->name );
         bug( buf, 0 );
         affect_strip( ch, gsn_charm_person );
         REMOVE_BIT( ch->affected_by, AFF_CHARM );
         return;
      }
      /*
      	send_to_char( "*** You are now a KILLER!! ***\n\r", ch->master );
        	SET_BIT(ch->master->act, PLR_KILLER);
      */
      stop_follower( ch );
      return;
   }

   /*
        * NPC's are cool of course (as long as not charmed).
        * Hitting yourself is cool too (bleeding).
        * So is being immortal (Alander's idea).
        * And current killers stay as they are.
        */
   if ( IS_NPC(ch)
   ||   ch == victim
   ||   ch->level >= LEVEL_IMMORTAL
   ||   IS_SET(ch->act, PLR_KILLER)
   || fighting_duel(ch, victim) )
      return;

   send_to_char( "*** You are now a KILLER!! ***\n\r", ch );
   SET_BIT(ch->act, PLR_KILLER);
   save_char_obj( ch );
   return;
}



/*
 * Check for parry. Mod by Faramir 10/8/96 so whips can't be blocked
 * or parried. mwahahaha
 */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim )
{
   OBJ_DATA *weapon;
   int chance;

   if ( !IS_AWAKE(victim) )
      return FALSE;

   if ( IS_NPC(victim) ) {
      chance	= UMIN( 30, victim->level );
      if (is_affected (victim, gsn_insanity))
	   chance -= 10;
   } else
     chance	= get_skill_learned (victim, gsn_parry) / 2;

   if ( get_eq_char( victim, WEAR_WIELD ) == NULL )
     return FALSE;
   if ( (weapon = get_eq_char (ch, WEAR_WIELD )) != NULL) {
     if (weapon->value[0] == WEAPON_WHIP )
       return FALSE;
   }
   if ( number_percent( ) >= chance + victim->level - ch->level )
     return FALSE;
   if (IS_SET (victim->comm, COMM_SHOWDEFENCE))
     act( "You parry $n's attack.",  ch, NULL, victim, TO_VICT    );
   if (IS_SET (ch->comm, COMM_SHOWDEFENCE))
      act( "$N parries your attack.", ch, NULL, victim, TO_CHAR    );
   check_improve(victim,gsn_parry,TRUE,6);
   return TRUE;
}



/*
 * Check for shield block.
 */
bool check_shield_block( CHAR_DATA *ch, CHAR_DATA *victim )
{
   OBJ_DATA *weapon;
   int chance;

   if ( !IS_AWAKE(victim) )
      return FALSE;

   if ( IS_NPC(victim) ) {
	chance  = UMIN( 30, victim->level );
	if (is_affected (victim, gsn_insanity))
	     chance -= 10;
   } else
     chance  = get_skill_learned (victim, gsn_shield_block) / 2;

   if ( get_eq_char( victim, WEAR_SHIELD ) == NULL )
     return FALSE;
   if ( (weapon = get_eq_char (ch, WEAR_WIELD)) != NULL) {
     if (weapon->value[0] == WEAPON_WHIP )
       return FALSE;
   }
   if ( number_percent( ) >= chance + victim->level - ch->level )
     return FALSE;

   if (IS_SET (victim->comm, COMM_SHOWDEFENCE))
      act( "You block $n's attack.",  ch, NULL, victim, TO_VICT    );
   if (IS_SET (ch->comm, COMM_SHOWDEFENCE))
      act( "$N blocks your attack.", ch, NULL, victim, TO_CHAR    );
   check_improve(victim,gsn_shield_block,TRUE,6);
   return TRUE;
}



/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chance;
   int ddex = 0;

   if ( !IS_AWAKE(victim) )
      return FALSE;

   if ( IS_NPC(victim) ) {
	chance  = UMIN( 30, victim->level );
	if (is_affected (victim, gsn_insanity))
	     chance -= 10;
   } else
	chance  = get_skill_learned (victim, gsn_dodge) / 2;

   /* added Faramir so dexterity actually influences like it says
    * in the help file! Thanx to Oshea for noticing */
   ddex = ( (get_curr_stat(victim, STAT_DEX) -
	     (get_curr_stat(ch, STAT_DEX))) * 3);

   chance += ddex;
   chance += (get_curr_stat(victim,STAT_DEX)/2);

   if ( number_percent( ) >= chance + victim->level - ch->level )
      return FALSE;

   if (IS_SET (victim->comm, COMM_SHOWDEFENCE))
      act( "You dodge $n's attack.", ch, NULL, victim, TO_VICT    );
   if (IS_SET (ch->comm, COMM_SHOWDEFENCE))
      act( "$N dodges your attack.", ch, NULL, victim, TO_CHAR    );
   check_improve(victim,gsn_dodge,TRUE,6);
   return TRUE;
}



/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
   if ( victim->hit > 0 )
   {
      if ( victim->position <= POS_STUNNED )
         victim->position = POS_STANDING;
      return;
   }

   if ( IS_NPC(victim) && victim->hit < 1 )
   {
      victim->position = POS_DEAD;
      return;
   }

   if ( victim->hit <= -11 )
   {
      victim->position = POS_DEAD;
      return;
   }

   if ( victim->hit <= -6 ) victim->position = POS_MORTAL;
   else if ( victim->hit <= -3 ) victim->position = POS_INCAP;
   else                          victim->position = POS_STUNNED;

   return;
}



/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{

   if ( ch->fighting != NULL )
   {
      bug( "Set_fighting: already fighting", 0 );
      return;
   }

   if ( IS_AFFECTED(ch, AFF_SLEEP) )
      affect_strip( ch, gsn_sleep );

   ch->fighting = victim;
   ch->position = POS_FIGHTING;

   return;
}



/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
   CHAR_DATA *fch;

   for ( fch = char_list; fch != NULL; fch = fch->next )
   {
      if ( fch == ch || ( fBoth && fch->fighting == ch ) )
      {
         fch->fighting	= NULL;
         fch->position	= IS_NPC(fch) ? ch->default_pos : POS_STANDING;
         update_pos( fch );
      }
   }

   return;
}



/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch )
{
   char buf[MAX_STRING_LENGTH];
   OBJ_DATA *corpse;
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   char *name;

   if ( IS_NPC(ch) )
   {
      name		= ch->short_descr;
      corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
      corpse->timer	= number_range( 3, 6 );
      if ( ch->gold > 0 )
      {
         obj_to_obj( create_money( ch->gold ), corpse );
         ch->gold = 0;
      }
      corpse->cost = 0;
   }
   else
   {
      name		= ch->name;
      corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
      corpse->timer	= number_range( 25, 40 );
      REMOVE_BIT(ch->act,PLR_CANLOOT);
      if (!IS_SET(ch->act,PLR_KILLER) && !IS_SET(ch->act,PLR_THIEF))
         corpse->owner = str_dup(ch->name);
      else
      corpse->owner = NULL;
      corpse->cost = 0;
   }

   corpse->level = ch->level;

   sprintf( buf, corpse->short_descr, name );
   free_string( corpse->short_descr );
   corpse->short_descr = str_dup( buf );

   sprintf( buf, corpse->description, name );
   free_string( corpse->description );
   corpse->description = str_dup( buf );

   for ( obj = ch->carrying; obj != NULL; obj = obj_next )
   {
      obj_next = obj->next_content;
      obj_from_char( obj );
      if (obj->item_type == ITEM_POTION)
         obj->timer = number_range(500,1000);
      if (obj->item_type == ITEM_SCROLL)
         obj->timer = number_range(1000,2500);
      if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH))
         obj->timer = number_range(5,10);
      REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);
      REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);

      if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
         extract_obj( obj );
      else
      obj_to_obj( obj, corpse );
   }

   obj_to_room( corpse, ch->in_room );
   return;
}



/*
 * Improved Death_cry contributed by Diavolo.
 * Modified to by Faramir to deal with verbose fight sequences
 */
void death_cry( CHAR_DATA *ch )
{
   ROOM_INDEX_DATA *was_in_room;
   const char *msg;
   int door;
   int vnum;
   int i =0;
   bool found = FALSE;

   vnum = 0;
   msg = "You hear $n's death cry.";

   switch ( number_range( 0,3)) {
	   
	   
   case  1:
	   if (ch->material == 0){
		   msg  = "$n splatters blood on your armor.";
		   break;
	   } /* roll on through to the next case....*/

   case  2:
	   if( ch->hit_location != 0 ) {
		   for( ; i < MAX_BODY_PARTS && !found; i++ ) {
			
			   if( IS_SET( ch->hit_location, 
				       race_body_table[i].part_flag )) {
				   msg = race_body_table[i].spill_msg;
				   vnum = race_body_table[i].obj_vnum;
				   found = TRUE;
			   }
		   }
	   }
	   break;
   case  0:
   default:
	   msg  = "$n hits the ground ... DEAD.";
	   break;
	   
   }
   
   act( msg, ch, NULL, NULL, TO_ROOM );
   
   if ( vnum != 0 )
   {
      char buf[MAX_STRING_LENGTH];
      OBJ_DATA *obj;
      char *name;

      name		= IS_NPC(ch) ? ch->short_descr : ch->name;
      obj		= create_object( get_obj_index( vnum ), 1 );
      obj->timer	= number_range( 4, 7 );

      sprintf( buf, obj->short_descr, name );
      free_string( obj->short_descr );
      obj->short_descr = str_dup( buf );

      sprintf( buf, obj->description, name );
      free_string( obj->description );
      obj->description = str_dup( buf );

      if (obj->item_type == ITEM_FOOD)
      {
         if (IS_SET(ch->form,FORM_POISON))
            obj->value[3] = 1;
         else if (!IS_SET(ch->form,FORM_EDIBLE))
            obj->item_type = ITEM_TRASH;
      }

      obj_to_room( obj, ch->in_room );
   }

   if ( IS_NPC(ch) )
      msg = "You hear something's death cry.";
   else
      msg = "You hear someone's death cry.";

   was_in_room = ch->in_room;
   for ( door = 0; door <= 5; door++ )
   {
      EXIT_DATA *pexit;

      if ( ( pexit = was_in_room->exit[door] ) != NULL
      &&   pexit->u1.to_room != NULL
      &&   pexit->u1.to_room != was_in_room )
      {
         ch->in_room = pexit->u1.to_room;
         act( msg, ch, NULL, NULL, TO_ROOM );
      }
   }
   ch->in_room = was_in_room;

   return;
}



void raw_kill( CHAR_DATA *victim )
{
   int i;

   stop_fighting( victim, TRUE );
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
   mprog_death_trigger( victim );
   death_cry( victim );

   if (!in_duel(victim))
      make_corpse( victim );

   if ( IS_NPC(victim) )
   {
      victim->pIndexData->killed++;
      kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
      extract_char( victim, TRUE );
      return;
   }

   if (!in_duel(victim))
      extract_char( victim, FALSE );
   while ( victim->affected )
      affect_remove( victim, victim->affected );
   victim->affected_by	= race_table[victim->race].aff;
   if (!in_duel(victim))
   {
      for (i = 0; i < 4; i++)
         victim->armor[i]= 100;
   }
   victim->position	= POS_RESTING;
   victim->hit		= UMAX( 1, victim->hit  );
   victim->mana	= UMAX( 1, victim->mana );
   victim->move	= UMAX( 1, victim->move );
   /* RT added to prevent infinite deaths */
   if (!in_duel(victim))
   {
      REMOVE_BIT(victim->act, PLR_KILLER);
      REMOVE_BIT(victim->act, PLR_THIEF);
      REMOVE_BIT(victim->act, PLR_BOUGHT_PET);
   }
   /*  save_char_obj( victim ); */
   return;
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *gch;
   CHAR_DATA *lch;
   int xp;
   int members;
   int group_levels;

   /*
        * Monsters don't get kill xp's or alignment changes.
        * P-killing doesn't help either.
        * Dying of mortal wounds or poison doesn't give xp to anyone!
        */
   if ( !IS_NPC(victim) || victim == ch )
      return;

   members = 0;
   group_levels = 0;
   for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
   {
      if ( is_same_group( gch, ch ) )
      {
         members++;
         group_levels += gch->level;
      }
   }

   if ( members == 0 )
   {
      bug( "Group_gain: members.", members );
      members = 1;
      group_levels = ch->level ;
   }

   lch = (ch->leader != NULL) ? ch->leader : ch;

   for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
   {
      OBJ_DATA *obj;
      OBJ_DATA *obj_next;

      if ( !is_same_group( gch, ch ) || IS_NPC(gch))
         continue;

      /*
      	if ( gch->level - lch->level >= 9 )
      	{
      	    send_to_char( "You are too high for this group.\n\r", gch );
      	    continue;
      	}

      	if ( gch->level - lch->level <= -9 )
      	{
      	    send_to_char( "You are too low for this group.\n\r", gch );
      	    continue;
      	}

      */
      /*
         Basic exp should be based on the highest level PC in the Group
      */
      xp = xp_compute( gch, victim, group_levels );
      sprintf( buf, "You receive %d experience points.\n\r", xp );
      send_to_char( buf, gch );
      gain_exp( gch, xp );

      for ( obj = ch->carrying; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if ( obj->wear_loc == WEAR_NONE )
            continue;

         if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
         ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
         ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
         {
            act( "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
            act( "$n is zapped by $p.",   ch, obj, NULL, TO_ROOM );
            obj_from_char( obj );
            obj_to_room( obj, ch->in_room );
         }
         if ( ( check_material_vulnerability(ch,obj) == 1) ) {
            /* Bastard :> */
            act( "You shriek in pain and drop $p.", ch, obj, NULL, TO_CHAR );
            act( "$n shrieks in pain and drops $p.", ch, obj, NULL, TO_ROOM );
            obj_from_char( obj );
            obj_to_room( obj, ch->in_room );
            return;
         }

      }
   }

   return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels )
{
   CHAR_DATA *tmpch;
   int xp,base_exp;
   int align,level_range;
   int change;
   int base_level = gch->level;
   int highest_level = 0;
   int time_per_level = 0;

   if (total_levels > gch->level)  /* Must be in a group */
   {
      /* Find level of highest PC in group */
      for ( tmpch = gch->in_room->people; tmpch != NULL;
	      tmpch = tmpch->next_in_room )
         if ( is_same_group( tmpch, gch ) )
            if ( tmpch->level > highest_level )
               highest_level = tmpch->level;

      base_level = highest_level;
   }

   level_range = victim->level - base_level;

   /* compute the base exp */
   switch (level_range)
   {
   default :
      base_exp =   0;
      break;
   case -9 :
      base_exp =   1;
      break;
   case -8 :
      base_exp =   2;
      break;
   case -7 :
      base_exp =   5;
      break;
   case -6 :
      base_exp =   9;
      break;
   case -5 :
      base_exp =  11;
      break;
   case -4 :
      base_exp =  22;
      break;
   case -3 :
      base_exp =  33;
      break;
   case -2 :
      base_exp =  55;
      break;
   case -1 :
      base_exp =  72;
      break;
   case  0 :
      base_exp =  85;
      break;
   case  1 :
      base_exp =  100;
      break;
   case  2 :
      base_exp = 120;
      break;
   case  3 :
      base_exp = 144;
      break;
   case  4 :
      base_exp = 160;
      break;
   }


   if (level_range > 4)
      base_exp = 150 + 20 * (level_range - 4);

   /* do alignment computations */

   align = victim->alignment - gch->alignment;

   if (IS_SET(victim->act,ACT_NOALIGN))
   {
      /* no change */
   }

   else if (align > 500) /* monster is more good than slayer */
   {
      change = (align - 500) * base_exp / 500 * base_level/total_levels;
      change = UMAX(1,change);
      gch->alignment = UMAX(-1000,gch->alignment - change);
   }

   else if (align < -500) /* monster is more evil than slayer */
   {
      change =  ( -1 * align - 500) * base_exp/500 * base_level/total_levels;
      change = UMAX(1,change);
      gch->alignment = UMIN(1000,gch->alignment + change);
   }

   else /* improve this someday */
   {
      change =  gch->alignment * base_exp/500 * base_level/total_levels;
      gch->alignment -= change;
   }

   /* calculate exp multiplier */
   if (IS_SET(victim->act,ACT_NOALIGN))
      xp = base_exp;

   else if (gch->alignment > 500)  /* for goodie two shoes */
   {
      if (victim->alignment < -750)
         xp = base_exp * 4/3;

      else if (victim->alignment < -500)
         xp = base_exp * 5/4;

      else if (victim->alignment > 750)
         xp = base_exp / 4;

      else if (victim->alignment > 500)
         xp = base_exp / 2;

      else if ((victim->alignment > 250))
         xp = base_exp * 3/4;

      else
         xp = base_exp;
   }

   else if (gch->alignment < -500) /* for baddies */
   {
      if (victim->alignment > 750)
         xp = base_exp * 5/4;

      else if (victim->alignment > 500)
         xp = base_exp * 11/10;

      else if (victim->alignment < -750)
         xp = base_exp * 1/2;

      else if (victim->alignment < -500)
         xp = base_exp * 3/4;

      else if (victim->alignment < -250)
         xp = base_exp * 9/10;

      else
         xp = base_exp;
   }

   else if (gch->alignment > 200)  /* a little good */
   {

      if (victim->alignment < -500)
         xp = base_exp * 6/5;

      else if (victim->alignment > 750)
         xp = base_exp * 1/2;

      else if (victim->alignment > 0)
         xp = base_exp * 3/4;

      else
         xp = base_exp;
   }

   else if (gch->alignment < -200) /* a little bad */
   {
      if (victim->alignment > 500)
         xp = base_exp * 6/5;

      else if (victim->alignment < -750)
         xp = base_exp * 1/2;

      else if (victim->alignment < 0)
         xp = base_exp * 3/4;

      else
         xp = base_exp;
   }

   else /* neutral */
   {

      if (victim->alignment > 500 || victim->alignment < -500)
         xp = base_exp * 4/3;

      else if (victim->alignment < 200 || victim->alignment > -200)
         xp = base_exp * 1/2;

      else
         xp = base_exp;
   }

   /* more exp at the low levels */
   if (base_level < 6)
      xp = 10 * xp / (base_level + 4);

   /* we have to scale down XP at higher levels as players progressively
      fight against tougher opponents and the original 60 level mud 
      calculations don't scale up right */
   /* less at high  -- modified 23/01/2000 --Fara */
   /* level 35, shave off 11%, level 69, shave off 23%, level 91 36% */

   if (base_level > 35 && base_level < 70 )
     xp = ( xp * 100 ) /  ( base_level / 3  + 100 );

   if( base_level >= 70 )
     xp = ( xp * 100 ) /  ( base_level / 2.5  + 100 );

   /* compute quarter-hours per level */

   time_per_level = 4 *
     (gch->played + (int) (current_time - gch->logon))/3600
     / base_level;

   /* ensure minimum of 6 quarts (1.5 hours) per level */
   time_per_level = URANGE(2, time_per_level, 6 );

   if (base_level < 15)  
     time_per_level = UMAX(time_per_level,(8 - base_level));
   xp = xp * time_per_level / 6;  

   
   /* randomize the rewards */
   xp = number_range (xp * 3/4, xp * 5/4);

   /* adjust for grouping */
   xp = (xp * gch->level/total_levels) * 1.5;

   return xp;
}

void dam_message( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,
		  int dam_type, bool immune  )
{
   char buf1[256], buf2[256], buf3[256];
   char vs[80];
   char vp[80];
   const char *attack;
   char body_part[32]; /* should be ample */
   const char * d_pref=NULL;
   const char * d_suff=NULL;
   char * ptr = body_part;
   const char * damstr_ptr=NULL;
   char  damltr_ptr = '\0';
   char * plural;
   sh_int b, c, d, prob, size_diff;
   long part_flag=0;
   char punct;
   signed int dam_prop;
   bool found = FALSE, back1 = FALSE;
   
   memset( body_part, 0, 32 );
   dam_prop = 0;

   victim->hit_location = 0;
   if( dam != 0 ) {  /* don't do any funny business if we have missed them! */
	   if( victim->max_hit != 0 )
		   dam_prop = ( dam * 100 ) / victim->max_hit; /* proportionate dam */
	   else
		   dam_prop = (dam * 100) / 1;
	   dam_prop += number_range( 2, ch->level / 6 ); /* modifier */
   }
   punct   = (dam_prop <= 20) ? '.' : '!';
   
   if( dam_prop >= 0 && dam_prop <= 12 ){ 
	   d_pref = "|w";
	   d_suff = "|w";
   }
   else if( dam_prop >= 16 && dam_prop <= 30 ){ 
	   d_pref = "|G";
	   d_suff = "|w";
   }
   else if( dam_prop >= 31 && dam_prop <= 61 ) {
	   d_pref = "|Y";
	   d_suff = "|w";
   }
   else if( dam_prop >= 62 && dam_prop <= 80 ) {
	   d_pref = "|W***  ";
	   d_suff = "  ***|w";
   }
   else if( dam_prop >=81 && dam_prop <= 120 ) {
	   d_pref = "|R<<<  ";
	   d_suff = "  >>>|w";
   }
   else {
	   d_pref ="|w";
	   d_suff ="|w";
   }
   
   strcpy( vs, d_pref );

   for( b = 0; dam_string_table[b].dam_types[DAM_NONE] != NULL
		&& !found  ; b++ ) {
	   if( dam_prop <= dam_string_table[b].amount ) {
		   if( dam_type > 0 && 
		       dam_string_table[b].dam_types[dam_type] != '\0' )
			   damstr_ptr = 
				   dam_string_table[b].dam_types[dam_type];
		   else    /*default damage string */
			   damstr_ptr = dam_string_table[b].dam_types[0];
		   strcat( vs, damstr_ptr ); 
		   damltr_ptr = damstr_ptr[strlen( damstr_ptr)-1];
/* FIX ME add multi dam types*/
		   found = TRUE;
	   }
   }
   if( !found ) {
	   damstr_ptr = "does |RUNSPEAKABLE|w things to";
	   strcat( vs, damstr_ptr );
	   damltr_ptr = '\0';
	   found = TRUE;
   }	   
   strcat( vs, d_suff ); /* that's the singular description made */
/* some cunning pluralisation...*/
   
   switch( damltr_ptr ) {

   case '\0':
	   plural = "";
	   break;

   case 'y':
	   plural = "ies";
	   back1 = TRUE;
	   break;
   case 'Y':
	   plural = "IES";
	   back1 = TRUE;
	   break;
   case 'h':
   case 's':
   case 'x':
	   plural = "es";
	   break;
   case 'H':
   case 'S':
   case 'X':
	   plural = "ES";
	   break;
   default: /* hopefully nobody puts numbers or punct and end of descrip */
	   if( isupper( damltr_ptr ))
		   plural = "S";
	   else
		   plural = "s";
	   break;
   }
   strcpy( vp, d_pref );
   strcat( vp, damstr_ptr );
   if( back1 == TRUE ) /* do we need to erase the last char?*/
	   vp[strlen(vp)-1] = '\0';  /* nuke the last char, e.g. 'y' */
   strcat( vp, plural );
   strcat( vp, d_suff );
   /* done */

   /* HACK to cope with headbutting - you only ever headbutt a head..
      never anywhere else! even if you are shorty fighting big mama */

   if( dt == gsn_headbutt ) {
	   SET_BIT( part_flag, PART_HEAD );
	   goto got_location;      /* GOTO */
   }

   d = 0;
   size_diff = 0;
   prob = 0;
   
   /* this section allows us to adjust the probability of where the char hits
      the victim. This is dependent on size, unless ch is affected by fly or
      haste in which case you are pretty likely to be able to hit them anywhere
   */
   size_diff = ch->size - victim->size;

   if( is_affected( ch, AFF_FLYING ) || is_affected( ch, AFF_HASTE )) 
	   size_diff = 0;

   if( size_diff == 0 )  
	   prob = 100;
   else prob = 75;

   while( ch != victim && body_part[0] == '\0' && d < 32 ) {  /* d to avoid unecessary loops */
	   ptr = &body_part[0];
	   c = number_range( 0, MAX_BODY_PARTS -1 );
	   part_flag = race_body_table[c].part_flag;
	   
	   if( race_table[victim->race].parts  & part_flag ) {
		   
/* this bit of code adds an element of realism for bipedal creatures at least:
   small ones are more likely to hit the lower parts of their opponent,
   less likely to hit the higher parts e.g. head, and vice versa */
		   if( size_diff >= 2 ) {  
/* i.e. if we are quite a bit bigger than them */
			   switch( race_body_table[c].pos ) {
			   case 3: /* we're hitting them high up */
				   prob = 100;
				   break;
			   case 2:
				   prob = 66;
				   break;
			   case 1:
			   default:
				   prob = 33;
			   }
		   }
		   if( size_diff <= -2 ) {
/* i.e. the other way around this time */
			   switch( race_body_table[c].pos ) {
			   case 3: /* we're hitting them high up */
				   prob = 33;
				   break;
			   case 2:
				   prob = 66;
				   break;
			   case 1:
			   default:
				   prob = 100;
			   }
		   }
		   if( number_percent() < prob ) {   
			   
/* using hit_location field in char_data - quite important in case the victim
   dies - he might lose a body part, it needs to be appropriate to the
   final wound inflicted */
			   SET_BIT( victim->hit_location, 
				    part_flag );
			   if( race_body_table[c].pair == TRUE ) { /* a paired part */
				   
				   switch( number_range( 0, 4 ) %2 ) {
				   case 0:
					   strcpy( ptr, "left " );
					   ptr += 5; /* change if necessary! */
					   break;
				   case 1:
				   default:
					   strcpy( ptr, "right " );
					   ptr += 6;
					   break;
				   }
			   }			   
			   if( part_flag & PART_ARMS ) {
				   switch( number_range( 0, 6 ) % 3 ) {
				   case 0:
					   strcpy( ptr, "bicep" );
					   break;
				   case 1:
					   strcpy( ptr, "shoulder" );
					   break;
				   default:
					   strcpy( ptr, "forearm" );
					   break;
				   }
			   } 
			   else if( part_flag & PART_LEGS ) {
				   
				   switch( number_range( 0, 6 ) % 3 ) {
				   case 0:
					   strcpy( ptr, "thigh" );
					   break;
				   case 1:
					   strcpy( ptr, "knee" );
					   break;
				   default:
					   strcpy( ptr, "calf" );
					   break;
				   }
			   }
			   else {
				   strcpy( ptr, race_body_table[c].name);
			   }
		   }
		   
	   }
	   d++;
   }
   if( strlen( ptr ) <= 2 ) { /* i.e if previous loop was not fruitful */

     got_location: /* GOTO - we only come here indirectly if headbutt */
	   strcpy( ptr, "head" );
	   victim->hit_location = race_body_table[0].part_flag;
   }
   

   if ( dt == TYPE_HIT )
	   {
      if (ch  == victim)
      {
	 sprintf( buf1, "$n %s $m %s%c|w",vp, body_part, punct);
         sprintf( buf2, "You %s your own %s%c|w",
		  vs, body_part, punct);
      }
      else {
	      sprintf( buf1, "$n %s $N's %s%c|w",  
		       vp, body_part, punct );  /* NOTVICT */
	      sprintf( buf2, "You %s $N's %s%c|w", 
		       vs, body_part, punct ); /* CHAR */
	      sprintf( buf3, "$n %s your %s%c|w", 
		       vp, body_part, punct ); /* VICT */
      }
   }
   else
   {
      if ( dt >= 0 && dt < MAX_SKILL )
         attack	= skill_table[dt].noun_damage;
      else if ( dt >= TYPE_HIT
		&& dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE) /* this might be broken*/
         attack	= attack_table[dt - TYPE_HIT].noun;
      else
      {
         bug( "Dam_message: bad dt %d.", dt );
         dt  = TYPE_HIT;
         attack  = attack_table[0].name;
      }

      if (immune)
      {
         if (ch == victim)
         {
            sprintf(buf1,"$n is |Wunaffected|w by $s own %s.|w",attack);
            sprintf(buf2,"Luckily, you are immune to that.|w");
         }
         else
         {
            sprintf(buf1,"$N is |Wunaffected|w by $n's %s!|w",attack);
            sprintf(buf2,"$N is |Wunaffected|w by your %s!|w",attack);
            sprintf(buf3,"$n's %s is powerless against you.|w",attack);
         }
      }
      else
      {
         if (ch == victim)
		 {
		 sprintf( buf1, "$n's %s %s $m%c|w",
			  attack, vp, punct);
		 sprintf( buf2, "Your %s %s you%c|w",
			  attack, vp, punct);
         }
         else
         {
		 if( dt == gsn_bash && dam_prop == 0 ) {
			 sprintf( buf1, "$n's %s %s $N%c|w",  
				  attack, vp, punct );
			 sprintf( buf2, "Your %s %s $N%c|w",  
				  attack, vp, punct );
			 sprintf( buf3, "$n's %s %s you%c|w", 
				  attack, vp, punct );
		 }
		 else {
			 sprintf( buf1, "$n's %s %s $N's %s%c|w",  
				  attack, vp, body_part, punct );
			 sprintf( buf2, "Your %s %s $N's %s%c|w",  
				  attack, vp, body_part, punct );
			 sprintf( buf3, "$n's %s %s your %s%c|w", 
				  attack, vp, body_part, punct );
		 }
         }
      }
   }
   
   if (ch == victim)
	   {
		   act(buf1,ch,NULL,NULL,TO_ROOM);
		   act(buf2,ch,NULL,NULL,TO_CHAR);
	   }
   else
	   {
		   act( buf1, ch, NULL, victim, TO_NOTVICT );
		   act( buf2, ch, NULL, victim, TO_CHAR );
		   act( buf3, ch, NULL, victim, TO_VICT );
	   }

   return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
   OBJ_DATA *obj;

   if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
      return;


   if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
   {
      act("$S weapon won't budge!",ch,NULL,victim,TO_CHAR);
      act("$n tries to disarm you, but your weapon won't budge!",
      ch,NULL,victim,TO_VICT);
      act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
      return;
   }

   act( "|W$n disarms you and sends your weapon flying!|w",
   ch, NULL, victim, TO_VICT    );
   act( "|WYou disarm $N!|w",  ch, NULL, victim, TO_CHAR    );
   act( "|W$n disarms $N!|w",  ch, NULL, victim, TO_NOTVICT );

   obj_from_char( obj );
   if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
      obj_to_char( obj, victim );
   else
   {
      obj_to_room( obj, victim->in_room );
      if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
         get_obj(victim,obj,NULL);
   }

   return;
}

void do_berserk( CHAR_DATA *ch, char *argument)
{
   int chance, hp_percent;
   /*    OBJ_DATA *wield = get_eq_char( ch, WEAR_WIELD );*/

   if ((chance = get_skill(ch,gsn_berserk)) == 0
   ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK))
   ||  (!IS_NPC(ch)
   &&   ch->level < get_skill_level(ch,gsn_berserk)))
   {
      send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
   ||  is_affected(ch,skill_lookup("frenzy")))

   {
      send_to_char("|rYou get a little madder|r.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(ch,AFF_CALM))
   {
      send_to_char("You're feeling too mellow to berserk.\n\r",ch);
      return;
   }

   if (ch->mana < 50)
   {
      send_to_char("You can't get up enough energy.\n\r",ch);
      return;
   }

   /* modifiers */

   /* fighting */
   if (ch->position == POS_FIGHTING)
      chance += 10;

   /* damage -- below 50% of hp helps, above hurts */
   hp_percent = 100 * ch->hit/ch->max_hit;
   chance += 25 - hp_percent/2;

   if (number_percent() < chance)
   {
      AFFECT_DATA af;

      WAIT_STATE(ch,PULSE_VIOLENCE);
      ch->mana -= 50;
      ch->move /= 2;

      /* heal a little damage */
      ch->hit += ch->level * 2;
      ch->hit = UMIN(ch->hit,ch->max_hit);

      send_to_char("|RYour pulse races as you are consumed by rage!|w\n\r",ch);
      act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM);
      check_improve(ch,gsn_berserk,TRUE,2);

      af.type		= gsn_berserk;
      af.level	= ch->level;
      af.duration	= number_fuzzy(ch->level / 8);
      af.modifier	= UMAX(1,ch->level/5);
      af.bitvector 	= AFF_BERSERK;

      af.location	= APPLY_HITROLL;
      affect_to_char(ch,&af);

      af.location	= APPLY_DAMROLL;
      affect_to_char(ch,&af);

      af.modifier	= UMAX(10,10 * (ch->level/5));
      af.location	= APPLY_AC;
      affect_to_char(ch,&af);

      /*	if ( (wield !=NULL) && (wield->item_type == ITEM_WEAPON) &&
      	    (IS_SET(wield->value[4], WEAPON_FLAMING)))
      	  {
      	    send_to_char("Your great energy causes your weapon to burst into
flame.\n\r",ch);
      	  wield->value[3] = 29;
      	  }*/

   }
   else
   {
      WAIT_STATE(ch,3 * PULSE_VIOLENCE);
      ch->mana -= 25;
      ch->move /= 2;

      send_to_char("Your pulse speeds up, but nothing happens.\n\r",ch);
      check_improve(ch,gsn_berserk,FALSE,2);
   }
}

void do_bash( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int chance;

   one_argument(argument,arg);

   if ( (chance = get_skill(ch,gsn_bash)) == 0
   ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
   ||	 (!IS_NPC(ch)
   &&	  ch->level < get_skill_level(ch,gsn_bash)))
   {
      send_to_char("Bashing? What's that?\n\r",ch);
      return;
   }

   if (ch->riding != NULL) {
      send_to_char("You can't bash while mounted.\n\r",ch);
      return;
   }

   if (arg[0] == '\0')
   {
      victim = ch->fighting;
      if (victim == NULL)
      {
         send_to_char("But you aren't fighting anyone!\n\r",ch);
         return;
      }
   }

   else if ((victim = get_char_room(ch,arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r",ch);
      return;
   }

   if (victim->position < POS_FIGHTING)
   {
      act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (victim == ch)
   {
      send_to_char("You try to bash your brains out, but fail.\n\r",ch);
      return;
   }

   if (is_safe(ch,victim))
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
   {
      act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
      return;
   }

   /* modifiers */

   /* size  and weight */
   chance += ch->carry_weight / 25;
   chance -= victim->carry_weight / 20;

   if (ch->size < victim->size)
      chance += (ch->size - victim->size) * 25;
   else
      chance += (ch->size - victim->size) * 10;


   /* stats */
   chance += get_curr_stat(ch,STAT_STR);
   chance -= get_curr_stat(victim,STAT_DEX) * 4/3;

   /* speed */
   if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
      chance += 10;
   if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
      chance -= 20;

   /* level */
   chance += (ch->level - victim->level) * 2;

   /* now the attack */
   if (number_percent() < chance)
   {

      act("$n sends you sprawling with a powerful bash!",
      ch,NULL,victim,TO_VICT);
      act("You slam into $N, and send $M flying!",ch,NULL,victim,TO_CHAR);
      act("$n sends $N sprawling with a powerful bash.",
      ch,NULL,victim,TO_NOTVICT);
      check_improve(ch,gsn_bash,TRUE,1);

      if (fighting_duel(ch,victim))
         WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
      else
         WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
      WAIT_STATE(ch,skill_table[gsn_bash].beats);
      victim->position = POS_RESTING;
      damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash,
      DAM_BASH);
      if (victim->ridden_by != NULL) {
         thrown_off(victim->ridden_by,victim);
      }
   }
   else
   {
      damage(ch,victim,0,gsn_bash,DAM_BASH);
      act("You fall flat on your face!",
      ch,NULL,victim,TO_CHAR);
      act("$n falls flat on $s face.",
      ch,NULL,victim,TO_NOTVICT);
      act("You evade $n's bash, causing $m to fall flat on $s face.",
      ch,NULL,victim,TO_VICT);
      check_improve(ch,gsn_bash,FALSE,1);
      ch->position = POS_RESTING;
      WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2);
      if (ch->ridden_by != NULL) {
         thrown_off(ch->ridden_by,ch);
      }
   }
}

void do_dirt( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int chance;

   one_argument(argument,arg);

   if ( (chance = get_skill(ch,gsn_dirt)) == 0
   ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT))
   ||   (!IS_NPC(ch)
   &&    ch->level < get_skill_level(ch,gsn_dirt)))
   {
      send_to_char("You get your feet dirty.\n\r",ch);
      return;
   }

   if (ch->riding != NULL) {
      send_to_char( "It's hard to dirt kick when your feet are off the ground!"
      ,ch);
      return;
   }

   if (arg[0] == '\0')
   {
      victim = ch->fighting;
      if (victim == NULL)
      {
         send_to_char("But you aren't in combat!\n\r",ch);
         return;
      }
   }

   else if ((victim = get_char_room(ch,arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(victim,AFF_BLIND))
   {
      act("$e's already been blinded.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (victim == ch)
   {
      send_to_char("Very funny.\n\r",ch);
      return;
   }

   if (is_safe(ch,victim))
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
   {
      act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
      return;
   }

   /* modifiers */

   /* dexterity */
   chance += get_curr_stat(ch,STAT_DEX);
   chance -= 2 * get_curr_stat(victim,STAT_DEX);

   /* speed  */
   if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
      chance += 10;
   if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
      chance -= 25;

   /* level */
   chance += (ch->level - victim->level) * 2;

   /* sloppy hack to prevent false zeroes */
   if (chance % 5 == 0)
      chance += 1;

   /* terrain */

   switch(ch->in_room->sector_type)
   {
      case(SECT_INSIDE):
      chance -= 20;
      break;
      case(SECT_CITY):
      chance -= 10;
      break;
      case(SECT_FIELD):
      chance +=  5;
      break;
      case(SECT_FOREST):
      break;
      case(SECT_HILLS):
      break;
      case(SECT_MOUNTAIN):
      chance -= 10;
      break;
      case(SECT_WATER_SWIM):
      chance  =  0;
      break;
      case(SECT_WATER_NOSWIM):
      chance  =  0;
      break;
      case(SECT_AIR):
      chance  =  0;
      break;
      case(SECT_DESERT):
      chance += 10;
      break;
   }

   if (chance == 0)
   {
      send_to_char("There isn't any dirt to kick.\n\r",ch);
      return;
   }

   /* now the attack */
   if (number_percent() < chance)
   {
      AFFECT_DATA af;
      act("$n is blinded by the dirt in $s eyes!",victim,NULL,NULL,TO_ROOM);
      damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE);
      send_to_char("You can't see a thing!\n\r",victim);
      check_improve(ch,gsn_dirt,TRUE,2);
      WAIT_STATE(ch,skill_table[gsn_dirt].beats);

      af.type 	= gsn_dirt;
      af.level 	= ch->level;
      af.duration	= 0;
      af.location	= APPLY_HITROLL;
      af.modifier	= -4;
      af.bitvector 	= AFF_BLIND;

      affect_to_char(victim,&af);
   }
   else
   {
      damage(ch,victim,0,gsn_dirt,DAM_NONE);
      check_improve(ch,gsn_dirt,FALSE,2);
      WAIT_STATE(ch,skill_table[gsn_dirt].beats);
   }
}

void do_trip( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int chance;

   one_argument(argument,arg);

   if ( (chance = get_skill(ch,gsn_trip)) == 0
   ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TRIP))
   ||   (!IS_NPC(ch)
   && ch->level < get_skill_level(ch,gsn_trip)))
   {
      send_to_char("Tripping?  What's that?\n\r",ch);
      return;
   }


   if (ch->riding != NULL) {
      send_to_char("You can't trip while mounted.\n\r",ch);
      return;
   }

   if (arg[0] == '\0')
   {
      victim = ch->fighting;
      if (victim == NULL)
      {
         send_to_char("But you aren't fighting anyone!\n\r",ch);
         return;
      }
   }

   else if ((victim = get_char_room(ch,arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r",ch);
      return;
   }

   if (is_safe(ch,victim))
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if (IS_AFFECTED(victim,AFF_FLYING) || (victim->riding != NULL))
   {
      act("$S feet aren't on the ground.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (victim->position < POS_FIGHTING)
   {
      act("$N is already down.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (victim == ch)
   {
      send_to_char("You fall flat on your face!\n\r",ch);
      WAIT_STATE(ch,2 * skill_table[gsn_trip].beats);
      act("$n trips over $s own feet!",ch,NULL,NULL,TO_ROOM);
      return;
   }

   if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
   {
      act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
      return;
   }

   /* modifiers */

   /* size */
   if (ch->size < victim->size)
      chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

   /* dex */
   chance += get_curr_stat(ch,STAT_DEX);
   chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

   /* speed */
   if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
      chance += 10;
   if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
      chance -= 20;

   /* level */
   chance += (ch->level - victim->level) * 2;


   /* now the attack */
   if (number_percent() < chance)
   {
      act("$n trips you and you go down!",ch,NULL,victim,TO_VICT);
      act("You trip $N and $N goes down!",ch,NULL,victim,TO_CHAR);
      act("$n trips $N, sending $M to the ground.",ch,NULL,victim,TO_NOTVICT);
      check_improve(ch,gsn_trip,TRUE,1);

      WAIT_STATE(victim,2 * PULSE_VIOLENCE);
      WAIT_STATE(ch,skill_table[gsn_trip].beats);
      victim->position = POS_RESTING;
      damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip,
      DAM_BASH);
      if (victim->ridden_by != NULL) {
         thrown_off(victim->ridden_by, victim);
      }
   }
   else
   {
      damage(ch,victim,0,gsn_trip,DAM_BASH);
      WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
      check_improve(ch,gsn_trip,FALSE,1);
   }
}



void do_kill( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Kill whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( !IS_NPC(victim) )
   {
      if ( !IS_SET(victim->act, PLR_KILLER)
      &&   !IS_SET(victim->act, PLR_THIEF) )
      {
         send_to_char( "You must MURDER a player.\n\r", ch );
         return;
      }
   }

   if ( victim == ch )
   {
      send_to_char( "You hit yourself.  Ouch!\n\r", ch );
      multi_hit( ch, ch, TYPE_UNDEFINED );
      return;
   }

   if ( is_safe( ch, victim ) )
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
   {
      act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if ( ch->position == POS_FIGHTING )
   {
      send_to_char( "You do the best you can!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
   check_killer( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}



void do_murde( CHAR_DATA *ch, char *argument )
{
   send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
   return;
}



void do_murder( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Murder whom?\n\r", ch );
      return;
   }

   if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
      return;

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim == ch )
   {
      send_to_char( "Suicide is a mortal sin.\n\r", ch );
      return;
   }

   if ( is_safe( ch, victim ) )
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
   {
      act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if ( ch->position == POS_FIGHTING )
   {
      send_to_char( "You do the best you can!\n\r", ch );
      return;
   }

   WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
   if (IS_NPC(ch))
      sprintf(buf, "Help! I am being attacked by %s!",ch->short_descr);
   else
   sprintf( buf, "Help!  I am being attacked by %s!", ch->name );
   do_yell( victim, buf );
   check_killer( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
   return;
}



void do_backstab( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   one_argument( argument, arg );

   /* Death : removed NPC check, so mobs can backstab */
   if (!IS_NPC(ch))
      if ( (ch->level < get_skill_level(ch,gsn_backstab))
      || (get_skill_learned (ch, gsn_backstab) == 0) )
      {
         send_to_char(
         "That might not be a good idea. You might hurt yourself.\n\r", ch );
         return ;
      }

   if ( arg[0] == '\0' )
   {
      send_to_char( "Backstab whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\n\r", ch );
      return;
   }

   if ( is_safe( ch, victim ) )
      return;

   if ( victim->fighting != NULL && !is_same_group(ch,victim->fighting))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
   {
      send_to_char( "You need to wield a weapon to backstab.\n\r", ch );
      return;
   }

   if ( victim->fighting != NULL )
   {
      send_to_char( "You can't backstab someone who's fighting.\n\r", ch );
      return;
   }

   if ( (victim->hit < victim->max_hit) )
      if (can_see(victim, ch))
      {
         act( "$N is hurt and suspicious ... you can't sneak up.",
         ch, NULL, victim, TO_CHAR );
         return;
      }

   check_killer( ch, victim );
   WAIT_STATE( ch, skill_table[gsn_backstab].beats );
   if (!IS_NPC(ch) && (!IS_AWAKE(victim)
   ||   number_percent( ) < get_skill_learned (ch, gsn_backstab)) )
   {
      check_improve(ch,gsn_backstab,TRUE,1);
      multi_hit( ch, victim, gsn_backstab );
   }
   else
   {
      check_improve(ch,gsn_backstab,FALSE,1);
      damage( ch, victim, 0, gsn_backstab,DAM_NONE );
   }
   return;
}



void do_flee( CHAR_DATA *ch, char *argument )
{
   ROOM_INDEX_DATA *was_in;
   ROOM_INDEX_DATA *now_in;
   CHAR_DATA *victim;
   int attempt;

   if ( ( victim = ch->fighting ) == NULL )
   {
      if ( ch->position == POS_FIGHTING )
         ch->position = POS_STANDING;
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }


   if (IS_AFFECTED(ch,AFF_LETHARGY))
   {
      act("You are too lethargic to flee.",ch, NULL, victim, TO_CHAR );
      return;
   }

   was_in = ch->in_room;
   for ( attempt = 0; attempt < 6; attempt++ )
   {
      EXIT_DATA *pexit;
      int door;

      door = number_door( );
      if ( ( pexit = was_in->exit[door] ) == 0
      ||   pexit->u1.to_room == NULL
      ||   IS_SET(pexit->exit_info, EX_CLOSED)
      || ( IS_NPC(ch)
      &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
         continue;

      move_char( ch, door, FALSE );
      if ( ( now_in = ch->in_room ) == was_in )
         continue;

      ch->in_room = was_in;
      if (ch->ridden_by != NULL)
         thrown_off(ch->ridden_by, ch);
      act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
      ch->in_room = now_in;

      if ( !IS_NPC(ch) )
      {
         send_to_char( "You flee from combat!  You lose 10 exps.\n\r", ch );
         gain_exp( ch, -10 );
      }

      stop_fighting( ch, TRUE );
      do_flee_check (ch);
      return;
   }

   send_to_char( "PANIC! You couldn't escape!\n\r", ch );
   return;
}



void do_rescue( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CHAR_DATA *fch;

   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   {
      send_to_char( "Rescue whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim == ch )
   {
      send_to_char( "What about fleeing instead?\n\r", ch );
      return;
   }

   if ( !is_same_group(ch,victim))
   {
      send_to_char("Kill stealing is not permitted.\n\r",ch);
      return;
   }

   if ( !IS_NPC(ch) && IS_NPC(victim) )
   {
      send_to_char( "Doesn't need your help!\n\r", ch );
      return;
   }

   if ( ch->fighting == victim )
   {
      send_to_char( "Too late.\n\r", ch );
      return;
   }

   if ( ( fch = victim->fighting ) == NULL )
   {
      send_to_char( "That person is not fighting right now.\n\r", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_rescue].beats );
   if ( !IS_NPC(ch) && number_percent( ) > get_skill_learned (ch, gsn_rescue) )
   {
      send_to_char( "You fail the rescue.\n\r", ch );
      check_improve(ch,gsn_rescue,FALSE,1);
      return;
   }

   act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    );
   act( "$n rescues you!", ch, NULL, victim, TO_VICT    );
   act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT );
   check_improve(ch,gsn_rescue,TRUE,1);

   stop_fighting( fch, FALSE );
   stop_fighting( victim, FALSE );

   check_killer( ch, fch );
   set_fighting( ch, fch );
   set_fighting( fch, ch );
   return;
}

/* TheMoog woz 'ere */
/* er...and me!...OFTEN!     */
/* ER..AND WANDERA...*/
/* A.N.D. BLOODY ME AS WELL (Death) */
/* blimey o'reilly, so was Faramir */
/* <RoFL!> ---- ok I'd accept this...were headbutt tough to code! <boggle> */

/* and what a surprise, Oshea's had some mods too. */

/* !!!! */
/* ok now the code....yes you did get to it eventually..*/

void do_headbutt( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int chance;


   if ( !IS_NPC(ch)
   && ch->level < get_skill_level(ch,gsn_headbutt))
   {
      send_to_char(
      "That might not be a good idea. You might hurt yourself.\n\r", ch );
      return ;
   }

   if (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_HEADBUTT))
      return;

   one_argument( argument, arg );

   if (arg[0] == '\0')
   {
      if (ch->fighting == NULL)
      {
         send_to_char("You headbutt the air furiously!\n\r", ch);
         return;
      }
      else
      victim = ch->fighting;
   }
   else
   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char("They aren't here.\n\r",ch );
      return;
   }
   else
   if (victim != ch->fighting && (ch->fighting != NULL) )
   {
      send_to_char("No way! You are still fighting!\n\r", ch);
      return;
   }

   if (ch->riding != NULL) {
      send_to_char ("You cannot headbutt whilst mounted.\n\r", ch);
      return;
   }

   /* Changed to next condition instead.  Oshea
   if (ch->fighting == NULL && victim->fighting == NULL)
   {
      if (ch == challenger || ch == challengee)
         return;
   }

   if (!IS_NPC (victim) && (ch->in_room->vnum != CHAL_ROOM)) {
      send_to_char ("You can only legally headbutt a player if you are
duelling with them.\n\r", ch);
      return;
   }
*/

   if (!IS_NPC(victim) && !fighting_duel(ch,victim)) {
     send_to_char ("You can only legally headbutt a player if you are duelling with them.\n\r", ch);
      return;
   }


   if ( victim->fighting != NULL && ((ch->fighting != victim) &&
				     (victim->fighting != ch)) ) {
	   send_to_char("Hey, they're already fighting someone else!\n\r", ch);
	   return;
   }

   WAIT_STATE( ch, skill_table[gsn_headbutt].beats);

   if ( ( chance = number_percent( ) ) < get_skill_learned (ch, gsn_headbutt))
   {
      damage( ch, victim, number_range( ch->level /3, ch->level ),
      gsn_headbutt, DAM_BASH );
      check_improve(ch,gsn_headbutt, TRUE, 2);

      if (ch->size >= (victim->size-1) && ch->size <= (victim->size+1) ) {

         chance = chance - ch->level + victim->level ;
         if ( (chance < 5 ) || !IS_AFFECTED(victim,AFF_BLIND))
         {
            act("$n is blinded by the blood running into $s eyes!",
            victim, NULL, NULL, TO_ROOM);
            send_to_char("Blood runs into your eyes - you can't see!\n\r",
            victim);

            af.type           = gsn_headbutt;
            af.level          = ch->level;
            af.duration       = 0;
            af.location       = APPLY_HITROLL;
            af.modifier       = -5;
            af.bitvector      = AFF_BLIND;

            affect_to_char(victim, &af);
         }
      }
   }
   else
   {
      act("$N dodges your headbutt. You feel disoriented.",ch, NULL, victim,
TO_CHAR);
      act("$N dodges $n's headbutt.",ch, NULL, victim, TO_ROOM);
      act("You dodge $n's headbutt.",ch, NULL, victim, TO_VICT);
      WAIT_STATE(ch, skill_table[gsn_headbutt].beats/2 );

   }
}


/* Wandera's little baby is just slipping in here */
/**/
void do_sharpen( CHAR_DATA *ch, char *argument )
{
   OBJ_DATA *weapon;
   int chance;

   if ( (weapon = get_eq_char(ch, WEAR_WIELD)) == NULL)
   {
      send_to_char("You must be wielding a weapon to sharpen it.\n\r", ch);
      return;
   }

   if (weapon->item_type != ITEM_WEAPON)
   {
      send_to_char("You can't sharpen it. It is not a weapon.\n\r",ch);
      return;
   }

   if ( !IS_NPC(ch)
   &&  ch->level < get_skill_level(ch,gsn_sharpen))
   {
      send_to_char("You can't do that or you may cut yourself.\n\r",ch);
      return;
   }

   if (IS_SET (weapon->value[4], WEAPON_SHARP))
   {
      send_to_char ("It can't get any sharper.\n\r", ch);
      return;
   }

   chance = get_skill(ch,gsn_sharpen);
   if ( number_percent() <= chance )
   {
      SET_BIT(weapon->value[4], WEAPON_SHARP);
      send_to_char("You sharpen the weapon to a fine, deadly point.\n\r",ch);
   }
   else {
      send_to_char("Your lack of skill removes all bonuses on this weapon.\n\r", ch);
      weapon->pIndexData->condition -= 10;  /* reduce condition of weapon*/
      weapon->value[4] = 0;  /* Wipe all bonuses */
   }
   check_improve(ch,gsn_sharpen,TRUE,5);
}

void do_kick( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   if ( !IS_NPC(ch)
   &&   ch->level < get_skill_level(ch,gsn_kick))
   {
      send_to_char(
      "You better leave the martial arts to fighters.\n\r", ch );
      return;
   }

   if (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK))
      return;

   one_argument( argument, arg );

   if (ch->riding != NULL) {
      send_to_char( "You can't kick - your feet are still in the stirrups!\n\r"
      ,ch);
      return;
   }

   if (arg[0] == '\0')
   {
      if (ch->fighting == NULL)
      {
         send_to_char("Kick who?\n\r", ch);
         return;
      }
      else
      victim = ch->fighting;
   }
   else
   if ( ( victim = get_char_room( ch, arg) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   else
   if (victim != ch->fighting && (ch->fighting != NULL) )
   {
      send_to_char( "No way! You are still fighting!\n\r", ch);
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_kick].beats );
   if ( number_percent( ) < get_skill_learned (ch, gsn_kick) )
   {
      damage( ch, victim, number_range( 1, ch->level ), gsn_kick,DAM_BASH );
      check_improve(ch,gsn_kick,TRUE,1);
   }
   else
   {
      damage( ch, victim, 0, gsn_kick,DAM_BASH );
      check_improve(ch,gsn_kick,FALSE,1);
   }

   return;
}




void do_disarm( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int chance,hth,ch_weapon,vict_weapon,ch_vict_weapon;

   hth = 0;

   if ((chance = get_skill(ch,gsn_disarm)) == 0)
   {
      send_to_char( "You don't know how to disarm opponents.\n\r", ch );
      return;
   }

   if ( get_eq_char( ch, WEAR_WIELD ) == NULL
   &&   ((hth = get_skill(ch,gsn_hand_to_hand)) == 0
   ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
   {
      send_to_char( "You must wield a weapon to disarm.\n\r", ch );
      return;
   }

   if ( ( victim = ch->fighting ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\n\r", ch );
      return;
   }

   if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
      return;
   }

   if ( IS_AFFECTED(victim, AFF_TALON))
   {
      act("$N's talon-like grip stops you from disarming $M!",ch,NULL,victim,
      TO_CHAR);
      act("$n tries to disarm you, but your talon like grip stops them!",
      ch,NULL,victim,TO_VICT);
      act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
      return;
   }

   /* find weapon skills */
   ch_weapon = get_weapon_skill(ch,get_weapon_sn(ch));
   vict_weapon = get_weapon_skill(victim,get_weapon_sn(victim));
   ch_vict_weapon = get_weapon_skill(ch,get_weapon_sn(victim));

   /* modifiers */

   /* skill */
   if ( get_eq_char(ch,WEAR_WIELD) == NULL)
      chance = chance * hth/150;
   else
   chance = chance * ch_weapon/100;

   chance += (ch_vict_weapon/2 - vict_weapon) / 2;

   /* dex vs. strength */
   chance += get_curr_stat(ch,STAT_DEX);
   chance -= 2 * get_curr_stat(victim,STAT_STR);

   /* level */
   chance += (ch->level - victim->level) * 2;

   /* and now the attack */
   if (number_percent() < chance)
   {
      WAIT_STATE( ch, skill_table[gsn_disarm].beats );
      disarm( ch, victim );
      check_improve(ch,gsn_disarm,TRUE,1);
   }
   else
   {
      WAIT_STATE(ch,skill_table[gsn_disarm].beats);
      act("You fail to disarm $N.",ch,NULL,victim,TO_CHAR);
      act("$n tries to disarm you, but fails.",ch,NULL,victim,TO_VICT);
      act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
      check_improve(ch,gsn_disarm,FALSE,1);
   }
   return;
}



void do_sla( CHAR_DATA *ch, char *argument )
{
   send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
   return;
}



void do_slay( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];

   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   {
      send_to_char( "Slay whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( ch == victim )
   {
      send_to_char( "Suicide is a mortal sin.\n\r", ch );
      return;
   }

   if ( !IS_NPC(victim) && victim->level >= get_trust(ch) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   act( "You slay $M in cold blood!",  ch, NULL, victim, TO_CHAR    );
   act( "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
   act( "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
   raw_kill( victim );
   return;
}


