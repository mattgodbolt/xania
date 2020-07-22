/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  magic.c:  wild and wonderful spells                                  */
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "challeng.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look     );
DECLARE_DO_FUN(do_give          );

/*
 * Local functions.
 */
void  say_spell   args( ( CHAR_DATA *ch, int sn )       );
void    explode_bomb    args( ( OBJ_DATA *bomb, CHAR_DATA *ch,
CHAR_DATA *thrower ) );
void    do_recall       args( ( CHAR_DATA *, char *));



/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
   char buf  [MAX_STRING_LENGTH];
   char buf2 [MAX_STRING_LENGTH * 2];
   char buf3 [MAX_STRING_LENGTH];
   CHAR_DATA *rch;
   char *pName;
   int iSyl;
   int length;

   struct syl_type
   {
      char *   old;
      char *   new;
   };

   static const struct syl_type syl_table[] =
   {
      {
         " ",      " "            }
      ,
      {
         "ar",     "abra"            }
      ,
      {
         "au",     "kada"            }
      ,
      {
         "bless",  "fido"            }
      ,
      {
         "blind",  "nose"            }
      ,
      {
         "bur", "mosa"            }
      ,
      {
         "cu",     "judi"            }
      ,
      {
         "de",     "oculo"           }
      ,
      {
         "en",     "unso"            }
      ,
      {
         "light",  "dies"            }
      ,
      {
         "lo",     "hi"           }
      ,
      {
         "mor", "zak"          }
      ,
      {
         "move",   "sido"            }
      ,
      {
         "ness",   "lacri"           }
      ,
      {
         "ning",   "illa"            }
      ,
      {
         "per", "duda"            }
      ,
      {
         "ra",     "gru"          }
      ,
      {
         "fresh",  "ima"          }
      ,
      {
         "gate",       "imoff"               }
      ,
      {
         "re",     "candus"       }
      ,
      {
         "son", "sabru"           }
      ,
      {
         "tect",   "infra"           }
      ,
      {
         "tri", "cula"            }
      ,
      {
         "ven", "nofo"            }
      ,
      {
         "oct",        "bogusi"              }
      ,
      {
         "rine",       "dude"                }
      ,
      {
         "a", "a"       }
      , {
         "b", "b"       }
      , {
         "c", "q"       }
      , {
         "d", "e"       }
      ,
      {
         "e", "z"       }
      , {
         "f", "y"       }
      , {
         "g", "o"       }
      , {
         "h", "p"       }
      ,
      {
         "i", "u"       }
      , {
         "j", "y"       }
      , {
         "k", "t"       }
      , {
         "l", "r"       }
      ,
      {
         "m", "w"       }
      , {
         "n", "i"       }
      , {
         "o", "a"       }
      , {
         "p", "s"       }
      ,
      {
         "q", "d"       }
      , {
         "r", "f"       }
      , {
         "s", "g"       }
      , {
         "t", "h"       }
      ,
      {
         "u", "j"       }
      , {
         "v", "z"       }
      , {
         "w", "x"       }
      , {
         "x", "n"       }
      ,
      {
         "y", "l"       }
      , {
         "z", "k"       }
      ,
      {
         "", ""       }
   };

   buf[0]  = '\0';
   for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
   {
      for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
      {
         if ( !str_prefix( syl_table[iSyl].old, pName ) )
         {
            strcat( buf, syl_table[iSyl].new );
            break;
         }
      }

      if ( length == 0 )
         length = 1;
   }

   snprintf( buf2, sizeof(buf2), "$n utters the words, '%s'.", buf );
   snprintf( buf, sizeof(buf), "$n utters the words, '%s'.", skill_table[sn].name );

   for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
   {
      if (!IS_NPC(rch) && rch->pcdata->colour)
      {
         snprintf(buf3, sizeof(buf3),"%c[0;33m", 27);
         send_to_char(buf3, rch);
      }
      if ( rch != ch )
         act( ch->class==rch->class ? buf : buf2, ch, NULL, rch, TO_VICT );
      if (!IS_NPC(rch) && rch->pcdata->colour)
      {
         snprintf(buf3, sizeof(buf3), "%c[0;37m", 27);
         send_to_char(buf3, rch);
      }
   }

   return;
}



/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_spell( int level, CHAR_DATA *victim )
{
   int save;

   save = 50 + ( victim->level - level - victim->saving_throw ) * 5;
   if (IS_AFFECTED(victim,AFF_BERSERK))
      save += victim->level/2;
   save = URANGE( 5, save, 95 );
   return number_percent( ) < save;
}

/* RT save for dispels */

bool saves_dispel( int dis_level, int spell_level, int duration)
{
   int save;

   if (duration == -1)
      spell_level += 5;
   /* very hard to dispel permanent effects */

   save = 50 + (spell_level - dis_level) * 5;
   save = URANGE( 5, save, 95 );
   return number_percent( ) < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn)
{
   AFFECT_DATA *af;

   if (is_affected(victim, sn))
   {
      for ( af = victim->affected; af != NULL; af = af->next )
      {
         if ( af->type == sn )
         {
            if (!saves_dispel(dis_level,af->level,af->duration))
            {
               affect_strip(victim,sn);
               if ( skill_table[sn].msg_off )
               {
                  send_to_char( skill_table[sn].msg_off, victim );
                  send_to_char( "\n\r", victim );
               }
               return TRUE;
            }
            else
            af->level--;
         }
      }
   }
   return FALSE;
}

/* for finding mana costs -- temporary version */
int mana_cost (CHAR_DATA *ch, int min_mana, int level)
{
   if (ch->level + 2 == level)
      return 1000;
   return UMAX(min_mana,(100/(2 + ch->level - level)));
}



/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

void do_cast( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   OBJ_DATA *potion;
   OBJ_INDEX_DATA *i_Potion;
   OBJ_DATA *scroll;
   OBJ_INDEX_DATA *i_Scroll;
   OBJ_DATA *bomb;
   char buf[MAX_STRING_LENGTH];
   void *vo;
   int mana;
   int sn;
   int pos;
   /* Switched NPC's can cast spells, but others can't. */
   if ( IS_NPC(ch) && (ch->desc == NULL))
      return;

   target_name = one_argument( argument, arg1 );
   one_argument( target_name, arg2 );

   if ( arg1[0] == '\0' )
   {
      send_to_char( "Cast which what where?\n\r", ch );
      return;
   }
   /* Changed this bit, to '<=' 0 : spells you didn't know, were starting
          fights =) */
   if ( ( sn = skill_lookup( arg1 ) ) <= 0
   || ( !IS_NPC(ch) && ch->level < get_skill_level(ch,sn)))
   {
      send_to_char( "You don't know any spells of that name.\n\r", ch );
      return;
   }

   if ( ch->position < skill_table[sn].minimum_position )
   {
      send_to_char( "You can't concentrate enough.\n\r", ch );
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   /* MG's rather more dubious bomb-making routine */

   if (!str_cmp(arg2,"explosive") ) {

      if (ch->class != 0) {
         send_to_char( "You're more than likely gonna kill yourself!\n\r", ch);
         return;
      }

      if ( ch->position < POS_STANDING ) {
         send_to_char( "You can't concentrate enough.\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->gold < (mana * 100) ) {
         send_to_char( "You can't afford to!\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->mana < (mana * 2) )
      {
         send_to_char( "You don't have enough mana.\n\r", ch );
         return;
      }

      WAIT_STATE( ch, skill_table[sn].beats*2 );

      if ( !IS_NPC(ch) && number_percent( ) > get_skill_learned (ch, sn) )
      {
         send_to_char( "You lost your concentration.\n\r", ch );
         check_improve(ch,sn,FALSE,8);
         ch->mana -= mana / 2;
      }
      else
      {
		  // Array holding the innate chance of explosion per
		  // additional spell for bombs
		  static const int bomb_chance[5] = 
		  { 0, 0, 20, 35, 70 };
		  ch->mana -= (mana * 2);
		  ch->gold -= (mana * 100);
		  
		  if ( ( ( bomb = get_eq_char(ch, WEAR_HOLD) ) !=NULL ) ) {
			  if (bomb->item_type != ITEM_BOMB ) {
				  send_to_char( "You must be holding a bomb to add to it.\n\r",ch);
				  send_to_char(
					  "Or, to create a new bomb you must have free hands.\n\r",ch);
				  ch->mana += (mana * 2);
				  ch->gold += (mana * 100);
				  return;
			  }
			  // God alone knows what I was on when I originally wrote this....
			  /* do detonation if things go wrong */
			  if ( (bomb->value[0] > (ch->level+1)) &&
				  ( number_percent() < 90 ) ) {
				  act( "Your naive fumblings on a higher-level bomb cause it to go off!",
					  ch, NULL, NULL, TO_CHAR);
				  act( "$n is delving with things $s doesn't understand - BANG!!",
					  ch, NULL, NULL, TO_CHAR);
				  explode_bomb( bomb, ch, ch );
				  return;
			  }

			  // Find first free slot
			  for (pos = 2; pos < 5; ++pos) {
				  if (bomb->value[pos] <= 0)
					  break;
			  }
			  if ((pos == 5) || // If we've run out of spaces in the bomb
				  (number_percent() > (get_curr_stat(ch,STAT_INT)*4)) || // test against int
				  (number_percent() < bomb_chance[pos])) { // test against the number of spells in the bomb
				  act( "You try to add another spell to your bomb but it can't take anymore!!!",
					  ch,NULL,NULL, TO_CHAR );
				  act( "$n tries to add more power to $s bomb - but has pushed it too far!!!",
					  ch,NULL,NULL, TO_ROOM );
				  explode_bomb( bomb,ch,ch );
				  return;
			  }
			  bomb->value[pos]=sn;
			  act( "$n adds more potency to $s bomb.",
				  ch, NULL, NULL, TO_ROOM );
			  act( "You carefully add another spell to your bomb.",
				  ch,NULL,NULL,TO_CHAR );
			  
		  }
		  else {
			  bomb = create_object(get_obj_index(OBJ_VNUM_BOMB), 0 );
			  bomb->level = ch->level;
			  bomb->value[0] = ch->level;
			  bomb->value[1] = sn;
			  
			  snprintf( buf, sizeof(buf), bomb->description, ch->name );
			  free_string( bomb->description );
			  bomb->description = str_dup( buf );
			  
			  snprintf( buf, sizeof(buf), bomb->name, ch->name );
			  free_string( bomb->name );
			  bomb->name = str_dup( buf );
			  
			  obj_to_char( bomb,ch );
			  equip_char( ch, bomb, WEAR_HOLD );
			  act( "$n creates $p.", ch, bomb, NULL, TO_ROOM );
			  act( "You have created $p.", ch, bomb, NULL, TO_CHAR );
			  
			  check_improve(ch,sn,TRUE,8);
		  }
      }
      return;
   }

   /* MG's scribing command ... */
   if (!str_cmp(arg2,"scribe") && (skill_table[sn].spell_fun != spell_null)) {

      if ( (ch->class != 0) && (ch->class != 1) ) {
         send_to_char( "You can't scribe! You can't read or write!\n\r", ch);
         return;
      }

      if ( ch->position < POS_STANDING ) {
         send_to_char( "You can't concentrate enough.\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->gold < (mana * 100) ) {
         send_to_char( "You can't afford to!\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->mana < (mana * 2) )
      {
         send_to_char( "You don't have enough mana.\n\r", ch );
         return;
      }
      i_Scroll = get_obj_index ( OBJ_VNUM_SCROLL );
      if ( ch->carry_weight + i_Scroll->weight > can_carry_w( ch ) ) {
	      act( "You cannot carry that much weight.",
		   ch, NULL, NULL, TO_CHAR );
	      return;
      }
      if ( ch->carry_number + 1 > can_carry_n( ch ) ) {
	      act ( "You can't carry any more items.",
		    ch, NULL, NULL, TO_CHAR );
	      return;
      }


      WAIT_STATE( ch, skill_table[sn].beats*2 );

      if ( !IS_NPC(ch) && number_percent( ) > get_skill_learned (ch, sn) )
      {
         send_to_char( "You lost your concentration.\n\r", ch );
         check_improve(ch,sn,FALSE,8);
         ch->mana -= mana / 2;
      }
      else
      {
         ch->mana -= (mana * 2);
         ch->gold -= (mana * 100);

         scroll = create_object(get_obj_index(OBJ_VNUM_SCROLL), 0 );
         scroll->level = ch->level;
         scroll->value[0] = ch->level;
         scroll->value[1] = sn;
         scroll->value[2] = -1;
         scroll->value[3] = -1;
         scroll->value[4] = -1;

         snprintf( buf, sizeof(buf), scroll->short_descr, skill_table[sn].name );
         free_string( scroll->short_descr );
         scroll->short_descr = str_dup( buf );

         snprintf( buf, sizeof(buf), scroll->description, skill_table[sn].name );
         free_string( scroll->description );
         scroll->description = str_dup( buf );

         snprintf( buf, sizeof(buf), scroll->name, skill_table[sn].name );
         free_string( scroll->name );
         scroll->name = str_dup( buf );

         obj_to_char( scroll,ch );
         act( "$n creates $p.", ch, scroll, NULL, TO_ROOM );
         act( "You have created $p.", ch, scroll, NULL, TO_CHAR );

         check_improve(ch,sn,TRUE,8);
      }
      return;
   }

   /* MG's brewing command ... */
   if (!str_cmp(arg2,"brew") && (skill_table[sn].spell_fun != spell_null)) {

      if ( (ch->class != 0) && (ch->class != 1) ) {
         send_to_char( "You can't make potions! You don't know how!\n\r", ch);
         return;
      }

      if ( ch->position < POS_STANDING ) {
         send_to_char( "You can't concentrate enough.\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->gold < (mana * 100) ) {
         send_to_char( "You can't afford to!\n\r", ch );
         return;
      }

      if ( !IS_NPC(ch) && ch->mana < (mana * 2) )
      {
         send_to_char( "You don't have enough mana.\n\r", ch );
         return;
      }
      i_Potion = get_obj_index ( OBJ_VNUM_POTION );
      if ( ch->carry_weight + i_Potion->weight > can_carry_w( ch ) ) {
	      act( "You cannot carry that much weight.",
		   ch, NULL, NULL, TO_CHAR );
	      return;
      }
      if ( ch->carry_number + 1 > can_carry_n( ch ) ) {
	      act ( "You can't carry any more items.",
		    ch, NULL, NULL, TO_CHAR );
	      return;
      }

      WAIT_STATE( ch, skill_table[sn].beats*2 );

      if ( !IS_NPC(ch) && number_percent( ) > get_skill_learned (ch, sn) )
      {
         send_to_char( "You lost your concentration.\n\r", ch );
         check_improve(ch,sn,FALSE,8);
         ch->mana -= mana / 2;
      }
      else
      {
         ch->mana -= (mana * 2);
         ch->gold -= (mana * 100);

         potion = create_object(get_obj_index(OBJ_VNUM_POTION), 0 );
         potion->level = ch->level;
         potion->value[0] = ch->level;
         potion->value[1] = sn;
         potion->value[2] = -1;
         potion->value[3] = -1;
         potion->value[4] = -1;

         snprintf( buf, sizeof(buf), potion->short_descr, skill_table[sn].name );
         free_string( potion->short_descr );
         potion->short_descr = str_dup( buf );

         snprintf( buf, sizeof(buf), potion->description, skill_table[sn].name );
         free_string( potion->description );
         potion->description = str_dup( buf );

         snprintf( buf, sizeof(buf), potion->name, skill_table[sn].name );
         free_string( potion->name );
         potion->name = str_dup( buf );

         obj_to_char( potion,ch );
         act( "$n creates $p.", ch, potion, NULL, TO_ROOM );
         act( "You have created $p.", ch, potion, NULL, TO_CHAR );

         check_improve(ch,sn,TRUE,8);
      }
      return;
   }

   /*
        * Locate targets.
        */
   victim  = NULL;
   obj     = NULL;
   vo      = NULL;

   switch ( skill_table[sn].target )
   {
   default:
      bug( "Do_cast: bad target for sn %d.", sn );
      return;

   case TAR_IGNORE:
      break;

   case TAR_CHAR_OFFENSIVE:
      if ( arg2[0] == '\0' )
      {
         if ( ( victim = ch->fighting ) == NULL )
         {
            send_to_char( "Cast the spell on whom?\n\r", ch );
            return;
         }
      }
      else
      {
         if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
         {
            send_to_char( "They aren't here.\n\r", ch );
            return;
         }
      }
      /*
              if ( ch == victim )
              {
                  send_to_char( "You can't do that to yourself.\n\r", ch );
                  return;
              }
      */


      if ( !IS_NPC(ch) )
      {

         if (is_safe_spell(ch,victim,FALSE) && victim != ch)
         {
            send_to_char("Not on that target.\n\r",ch);
            return;
         }
      }

      if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
      {
         send_to_char( "You can't do that on your own follower.\n\r",
         ch );
         return;
      }

      vo = (void *) victim;
      break;

   case TAR_CHAR_DEFENSIVE:
      if ( arg2[0] == '\0' )
      {
         victim = ch;
      }
      else
      {
         if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
         {
            send_to_char( "They aren't here.\n\r", ch );
            return;
         }
      }

      vo = (void *) victim;
      break;

   case TAR_CHAR_OBJECT:
   case TAR_CHAR_OTHER:
      vo = (void *) arg2;
      break;

   case TAR_CHAR_SELF:
      if ( arg2[0] != '\0' && !is_name( arg2, ch->name ) )
      {
         send_to_char( "You cannot cast this spell on another.\n\r", ch );
         return;
      }

      vo = (void *) ch;
      break;

   case TAR_OBJ_INV:
      if ( arg2[0] == '\0' )
      {
         send_to_char( "What should the spell be cast upon?\n\r", ch );
         return;
      }

      if ( ( obj = get_obj_carry( ch, arg2 ) ) == NULL )
      {
         send_to_char( "You are not carrying that.\n\r", ch );
         return;
      }

      vo = (void *) obj;
      break;
   }

   if ( !IS_NPC(ch) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\n\r", ch );
      return;
   }

   if ( str_cmp( skill_table[sn].name, "ventriloquate" ) )
      say_spell( ch, sn );

   WAIT_STATE( ch, skill_table[sn].beats );

   if ( !IS_NPC(ch) && number_percent( ) > get_skill_learned (ch, sn) )
   {
      send_to_char( "You lost your concentration.\n\r", ch );
      check_improve(ch,sn,FALSE,1);
      ch->mana -= mana / 2;
   }
   else
   {
      ch->mana -= mana;
      (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vo );
      check_improve(ch,sn,TRUE,1);
   }

   if ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
   &&   victim != ch
   &&   victim->master != ch)
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      if (ch && ch->in_room && ch->in_room->people != NULL) {
         for ( vch = ch->in_room->people; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL )
            {
               multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
   }

   return;
}



/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
   void *vo;

   if ( sn <= 0 )
      return;

   if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
   {
      bug( "Obj_cast_spell: bad sn %d.", sn );
      return;
   }

   switch ( skill_table[sn].target )
   {
   default:
      bug( "Obj_cast_spell: bad target for sn %d.", sn );
      return;

   case TAR_IGNORE:
      vo = NULL;
      break;

   case TAR_CHAR_OFFENSIVE:
      if ( victim == NULL )
         victim = ch->fighting;
      if ( victim == NULL )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      if (is_safe_spell(ch,victim,FALSE) && ch != victim)
      {
         send_to_char("Something isn't right...\n\r",ch);
         return;
      }
      vo = (void *) victim;
      break;

   case TAR_CHAR_DEFENSIVE:
      if ( victim == NULL )
         victim = ch;
      vo = (void *) victim;
      break;

   case TAR_CHAR_SELF:
      vo = (void *) ch;
      break;

   case TAR_OBJ_INV:
      if ( obj == NULL )
      {
         send_to_char( "You can't do that.\n\r", ch );
         return;
      }
      vo = (void *) obj;
      break;
   }

   /*   target_name = ""; - no longer needed */
   (*skill_table[sn].spell_fun) ( sn, level, ch, vo );

   if ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
   &&   victim != ch
   &&   victim->master != ch )
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      for ( vch = ch->in_room->people; vch; vch = vch_next )
      {
         vch_next = vch->next_in_room;
         if ( victim == vch && victim->fighting == NULL )
         {
            multi_hit( victim, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }

   return;
}



/*
 * Spell functions.
 */
void spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   dam = dice( level, 12 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn,DAM_ACID );
   return;
}

void spell_acid_wash( int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   /*AFFECT_DATA *paf;
       int result, fail;*/

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("You do not have that item in your inventory.\n\r",ch);
      return;
   }

   if ( IS_SET(obj->value[4], WEAPON_LIGHTNING) )
   {
      send_to_char("Acid and lightning don't mix.\n\r", ch);
      return;
   }
   obj->value[3] = ATTACK_TABLE_INDEX_ACID_WASH;
   SET_BIT(obj->value[4], WEAPON_ACID);
   send_to_char("With a mighty scream you draw acid from the earth.\n\rYou wash your weapon in the acid pool.\n\r", ch);
   return;
}

void spell_armor( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) )
   {
      if (victim == ch)
         send_to_char("You are already protected.\n\r",ch);
      else
         act("$N is already armored.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = 24;
   af.modifier  = -20;
   af.location  = APPLY_AC;
   af.bitvector = 0;
   affect_to_char( victim, &af );
   send_to_char( "You feel someone protecting you.\n\r", victim );
   if ( ch != victim )
      act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR);
   return;
}



void spell_bless( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) )
   {
      if (victim == ch)
         send_to_char("You are already blessed.\n\r",ch);
      else
         act("$N already has divine favor.",ch,NULL,victim,TO_CHAR);

      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = 6+level;
   af.location  = APPLY_HITROLL;
   af.modifier  = level / 8;
   af.bitvector = 0;
   affect_to_char( victim, &af );

   af.location  = APPLY_SAVING_SPELL;
   af.modifier  = 0 - level / 8;
   affect_to_char( victim, &af );
   send_to_char( "You feel righteous.\n\r", victim );
   if ( ch != victim )
      act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR);
   return;
}



void spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_BLIND) || saves_spell( level, victim ) )
      return;

   af.type      = sn;
   af.level     = level;
   af.location  = APPLY_HITROLL;
   af.modifier  = -4;
   af.duration  = 1;
   af.bitvector = AFF_BLIND;
   affect_to_char( victim, &af );
   send_to_char( "You are blinded!\n\r", victim );
   act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
   return;
}



void spell_burning_hands( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      0,  0,  0,  0,   14,   17, 20, 23, 26, 29,
      29, 29, 30, 30,   31,   31, 32, 32, 33, 33,
      34, 34, 35, 35,   36,   36, 37, 37, 38, 38,
      39, 39, 40, 40,   41,   41, 42, 42, 43, 43,
      44, 44, 45, 45,   46,   46, 47, 47, 48, 48
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_FIRE );
   return;
}



void spell_call_lightning( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)vo;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   int dam;
   char buf [MAX_STRING_LENGTH];

   if ( !IS_OUTSIDE(ch) )
   {
      send_to_char( "You must be out of doors.\n\r", ch );
      return;
   }

   if ( weather_info.sky < SKY_RAINING )
   {
      send_to_char( "You need bad weather.\n\r", ch );
      return;
   }

   dam = dice(level/2, 8);

   snprintf( buf, sizeof(buf), "%s's lightning strikes your foes!\n\r", deity_name);
   send_to_char( buf, ch );

   snprintf( buf, sizeof(buf), "$n calls %s's lightning to strike $s foes!", deity_name);
   act( buf,
   ch, NULL, NULL, TO_ROOM );

   for ( vch = char_list; vch != NULL; vch = vch_next )
   {
      vch_next = vch->next;
      if ( vch->in_room == NULL )
         continue;
      if ( vch->in_room == ch->in_room )
      {
         if ( vch != ch && ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) ) )
            damage( ch, vch, saves_spell( level,vch ) ? dam / 2 : dam, sn,
            DAM_LIGHTNING );
         continue;
      }

      if ( vch->in_room->area == ch->in_room->area
      &&   IS_OUTSIDE(vch)
      &&   IS_AWAKE(vch) )
         send_to_char( "Lightning flashes in the sky.\n\r", vch );
   }

   return;
}

/* RT calm spell stops all fighting in the room */

void spell_calm( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)vo;
   CHAR_DATA *vch;
   int mlevel = 0;
   int count = 0;
   int high_level = 0;
   int chance;
   AFFECT_DATA af;

   /* get sum of all mobile levels in the room */
   for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
   {
      if (vch->position == POS_FIGHTING)
      {
         count++;
         if (IS_NPC(vch))
            mlevel += vch->level;
         else
         mlevel += vch->level/2;
         high_level = UMAX(high_level,vch->level);
      }
   }

   /* compute chance of stopping combat */
   chance = 4 * level - high_level + 2 * count;

   if (IS_IMMORTAL(ch)) /* always works */
      mlevel = 0;

   if (number_range(0, chance) >= mlevel)  /* hard to stop large fights */
   {
      for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
      {
         if (IS_NPC(vch) && (IS_SET(vch->imm_flags,IMM_MAGIC) ||
         IS_SET(vch->act,ACT_UNDEAD)))
            return;

         if (IS_AFFECTED(vch,AFF_CALM) || IS_AFFECTED(vch,AFF_BERSERK)
         ||  is_affected(vch,skill_lookup("frenzy")))
            return;

         send_to_char("A wave of calm passes over you.\n\r",vch);

         if (vch->fighting || vch->position == POS_FIGHTING)
            stop_fighting(vch,FALSE);

         if (IS_NPC(vch) && vch->sentient_victim) {
            free_string(vch->sentient_victim);
            vch->sentient_victim=NULL;
         }

         af.type = sn;
         af.level = level;
         af.duration = level/4;
         af.location = APPLY_HITROLL;
         if (!IS_NPC(vch))
            af.modifier = -5;
         else
         af.modifier = -2;
         af.bitvector = AFF_CALM;
         affect_to_char(vch,&af);

         af.location = APPLY_DAMROLL;
         affect_to_char(vch,&af);
      }
   }
}

void spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   bool found = FALSE;

   level += 2;

   if ((!IS_NPC(ch) && IS_NPC(victim) &&
   !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) ) ||
   (IS_NPC(ch) && !IS_NPC(victim)) )
   {
      send_to_char("You failed, try dispel magic.\n\r",ch);
      return;
   }

   /* unlike dispel magic, the victim gets NO save */

   /* begin running through the spells */

   if (check_dispel(level,victim,skill_lookup("armor")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("bless")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("blindness")))
   {
      found = TRUE;
      act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("calm")))
   {
      found = TRUE;
      act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("change sex")))
   {
      found = TRUE;
      act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
      send_to_char("You feel more like yourself again.\n\r",victim);
   }

   if (check_dispel(level,victim,skill_lookup("charm person")))
   {
      found = TRUE;
      act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("chill touch")))
   {
      found = TRUE;
      act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("curse")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect evil")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect invis")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect hidden")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("protection evil")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("protection good")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("lethargy")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("regeneration")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("talon")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("octarine fire")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect magic")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("faerie fire")))
   {
      act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("fly")))
   {
      act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("frenzy")))
   {
      act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);
      ;
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("giant strength")))
   {
      act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("haste")))
   {
      act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("talon")))
   {
      act("$n's hands return to normal.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("lethargy")))
   {
      act("$n's heart-beat quickens.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("insanity")))
   {
      act("$n looks less confused.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("infravision")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("invis")))
   {
      act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("mass invis")))
   {
      act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("octarine fire")))
   {
      act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }
   if (check_dispel(level,victim,skill_lookup("pass door")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("sanctuary")))
   {
      act("The white aura around $n's body vanishes.",
      victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("shield")))
   {
      act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("sleep")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("stone skin")))
   {
      act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("weaken")))
   {
      act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (found)
      send_to_char("Ok.\n\r",ch);
   else
      send_to_char("Spell failed.\n\r",ch);
}

void spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
   damage( ch, (CHAR_DATA *) vo, dice(1, 8) + level / 3, sn, DAM_HARM );
   return;
}



void spell_cause_critical( int sn, int level, CHAR_DATA *ch, void *vo )
{
   damage( ch, (CHAR_DATA *) vo, dice(3, 8) + level - 6, sn, DAM_HARM );
   return;
}



void spell_cause_serious( int sn, int level, CHAR_DATA *ch, void *vo )
{
   damage( ch, (CHAR_DATA *) vo, dice(2, 8) + level / 2, sn, DAM_HARM );
   return;
}

void spell_chain_lightning(int sn, int level, CHAR_DATA *ch, void *vo)
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   CHAR_DATA *tmp_vict,*last_vict,*next_vict;
   bool found;
   int dam;

   /* first strike */

   act("A lightning bolt leaps from $n's hand and arcs to $N.",
   ch,NULL,victim,TO_ROOM);
   act("A lightning bolt leaps from your hand and arcs to $N.",
   ch,NULL,victim,TO_CHAR);
   act("A lightning bolt leaps from $n's hand and hits you!",
   ch,NULL,victim,TO_VICT);

   dam = dice(level,6);
   if (saves_spell(level,victim))
      dam /= 3;
   damage(ch,victim,dam,sn,DAM_LIGHTNING);
   last_vict = victim;
   level -= 4;   /* decrement damage */

   /* new targets */
   while (level > 0)
   {
      found = FALSE;
      for (tmp_vict = ch->in_room->people;
        tmp_vict != NULL;
        tmp_vict = next_vict)
      {
         next_vict = tmp_vict->next_in_room;
         if (!is_safe_spell(ch,tmp_vict,TRUE) && tmp_vict != last_vict)
         {
            found = TRUE;
            last_vict = tmp_vict;
            act("The bolt arcs to $n!",tmp_vict,NULL,NULL,TO_ROOM);
            act("The bolt hits you!",tmp_vict,NULL,NULL,TO_CHAR);
            dam = dice(level,6);
            if (saves_spell(level,tmp_vict))
               dam /= 3;
            damage(ch,tmp_vict,dam,sn,DAM_LIGHTNING);
            level -= 4;  /* decrement damage */
         }
      }   /* end target searching loop */

      if (!found) /* no target found, hit the caster */
      {
         if (ch == NULL || ch->in_room == NULL)
            return;

         if (last_vict == ch) /* no double hits */
         {
            act("The bolt seems to have fizzled out.",ch,NULL,NULL,TO_ROOM);
            act("The bolt grounds out through your body.",
            ch,NULL,NULL,TO_CHAR);
            return;
         }

         last_vict = ch;
         act("The bolt arcs to $n...whoops!",ch,NULL,NULL,TO_ROOM);
         send_to_char("You are struck by your own lightning!\n\r",ch);
         dam = dice(level,6);
         if (saves_spell(level,ch))
            dam /= 3;
         damage(ch,ch,dam,sn,DAM_LIGHTNING);
         level -= 4;  /* decrement damage */
         if ((ch == NULL) || (ch->in_room == NULL))
            return;
      }
      /* now go back and find more targets */
   }
}


void spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ))
   {
      if (victim == ch)
         send_to_char("You've already been changed.\n\r",ch);
      else
         act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR);
      return;
   }
   if (saves_spell(level , victim))
      return;
   af.type      = sn;
   af.level     = level;
   af.duration  = 2 * level;
   af.location  = APPLY_SEX;
   do
   {
      af.modifier  = number_range( 0, 2 ) - victim->sex;
   }
   while ( af.modifier == 0 );
   af.bitvector = 0;
   affect_to_char( victim, &af );
   send_to_char( "You feel different.\n\r", victim );
   act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM);
   return;
}



void spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( victim == ch )
   {
      send_to_char( "You like yourself even better!\n\r", ch );
      return;
   }

   if ( IS_AFFECTED(victim, AFF_CHARM)
   ||   IS_AFFECTED(ch, AFF_CHARM)
   ||   get_trust(ch) < get_trust(victim)
   ||   IS_SET(victim->imm_flags,IMM_CHARM)
   ||   saves_spell( level, victim ) )
      return;


   if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
   {
      send_to_char(
      "Charming is not permitted here.\n\r",ch);
      return;
   }

   if ( victim->master )
      stop_follower( victim );
   add_follower( victim, ch );
   victim->leader = ch;
   af.type      = sn;
   af.level    = level;
   af.duration  = number_fuzzy( level / 4 );
   af.location  = 0;
   af.modifier  = 0;
   af.bitvector = AFF_CHARM;
   affect_to_char( victim, &af );
   act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
   if ( ch != victim )
      act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR);
   return;
}



void spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      0,  0,  6,  7,  8,   9, 12, 13, 13, 13,
      14, 14, 14, 15, 15,  15, 16, 16, 16, 17,
      17, 17, 18, 18, 18,  19, 19, 19, 20, 20,
      20, 21, 21, 21, 22,  22, 22, 23, 23, 23,
      24, 24, 24, 25, 25,  25, 26, 26, 26, 27
   };
   AFFECT_DATA af;
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( !saves_spell( level, victim ) )
   {
      act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM);
      af.type      = sn;
      af.level     = level;
      af.duration  = 6;
      af.location  = APPLY_STR;
      af.modifier  = -1;
      af.bitvector = 0;
      affect_join( victim, &af );
   }
   else
   {
      dam /= 2;
   }

   damage( ch, victim, dam, sn, DAM_COLD );
   return;
}



void spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      30, 35, 40, 45, 50,  55, 55, 55, 56, 57,
      58, 58, 59, 60, 61,  61, 62, 63, 64, 64,
      65, 66, 67, 67, 68,  69, 70, 70, 71, 72,
      73, 73, 74, 75, 76,  76, 77, 78, 79, 79
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2,  dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   else
      spell_blindness(skill_lookup("blindness"),level/2,ch,(void *) victim);

   damage( ch, victim, dam, sn, DAM_LIGHT );
   return;
}



void spell_continual_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;(void)vo;
   OBJ_DATA *light;

   light = create_object( get_obj_index( OBJ_VNUM_LIGHT_BALL ), 0 );
   obj_to_room( light, ch->in_room );
   act( "$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM );
   act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR );
   return;
}



void spell_control_weather( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   if ( !str_cmp( target_name, "better" ) )
      weather_info.change += dice( level / 3, 4 );
   else if ( !str_cmp( target_name, "worse" ) )
      weather_info.change -= dice( level / 3, 4 );
   else
   send_to_char ("Do you want it to get better or worse?\n\r", ch );

   send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   OBJ_DATA *mushroom;

   mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0 );
   mushroom->value[0] = 5 + level;
   obj_to_room( mushroom, ch->in_room );
   act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM );
   act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR );
   return;
}



void spell_create_spring( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   OBJ_DATA *spring;

   spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0 );
   spring->timer = level;
   obj_to_room( spring, ch->in_room );
   act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM );
   act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR );
   return;
}



void spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int water;

   if ( obj->item_type != ITEM_DRINK_CON )
   {
      send_to_char( "It is unable to hold water.\n\r", ch );
      return;
   }

   if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
   {
      send_to_char( "It contains some other liquid.\n\r", ch );
      return;
   }

   water = UMIN(
   level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
   obj->value[0] - obj->value[1]
   );

   if ( water > 0 )
   {
      obj->value[2] = LIQ_WATER;
      obj->value[1] += water;
      if ( !is_name( "water", obj->name ) )
      {
         char buf[MAX_STRING_LENGTH];

         snprintf( buf, sizeof(buf), "%s water", obj->name );
         free_string( obj->name );
         obj->name = str_dup( buf );
      }
      act( "$p is filled.", ch, obj, NULL, TO_CHAR );
   }

   return;
}



void spell_cure_blindness( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;

   if ( !is_affected( victim, gsn_blindness ) )
   {
      if (victim == ch)
         send_to_char("You aren't blind.\n\r",ch);
      else
         act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (check_dispel((level+5),victim,gsn_blindness))
   {
      send_to_char( "Your vision returns!\n\r", victim );
      act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
   }
   else
      send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int heal;

   heal = dice(3, 8) + level - 6;
   victim->hit = UMIN( victim->hit + heal, victim->max_hit );
   update_pos( victim );
   send_to_char( "You feel better!\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}

/* RT added to cure plague */
void spell_cure_disease( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;

   if ( !is_affected( victim, gsn_plague ) )
   {
      if (victim == ch)
         send_to_char("You aren't ill.\n\r",ch);
      else
         act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (check_dispel((level+5),victim,gsn_plague))
   {
      send_to_char("Your sores vanish.\n\r",victim);
      act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM);
   }
   else
      send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int heal;

   heal = dice(1, 8) + level / 3;
   victim->hit = UMIN( victim->hit + heal, victim->max_hit );
   update_pos( victim );
   send_to_char( "You feel better!\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;

   if ( !is_affected( victim, gsn_poison ) )
   {
      if (victim == ch)
         send_to_char("You aren't poisoned.\n\r",ch);
      else
         act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (check_dispel((level+5),victim,gsn_poison))
   {
      send_to_char("A warm feeling runs through your body.\n\r",victim);
      act("$n looks much better.",victim,NULL,NULL,TO_ROOM);
   }
   else
      send_to_char("Spell failed.\n\r",ch);
}

void spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int heal;

   heal = dice(2, 8) + level /2 ;
   victim->hit = UMIN( victim->hit + heal, victim->max_hit );
   update_pos( victim );
   send_to_char( "You feel better!\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_curse( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_CURSE) || saves_spell( level, victim ) )
      return;

   af.type      = sn;
   af.level     = level;
   af.duration  = 2*level;
   af.location  = APPLY_HITROLL;
   af.modifier  = -1 * (level / 8);
   af.bitvector = AFF_CURSE;
   affect_to_char( victim, &af );

   af.location  = APPLY_SAVING_SPELL;
   af.modifier  = level / 8;
   affect_to_char( victim, &af );

   send_to_char( "You feel unclean.\n\r", victim );
   if ( ch != victim )
      act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR);
   return;
}

/* pgWandera's spell */

/* PGW hard hitting spell in the attack group, the big boy of the bunch */

void spell_exorcise( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   if ( !IS_NPC(ch) && (victim->alignment > (ch->alignment+100)) )
   {
      victim = ch;
      send_to_char("Your exorcism turns upon you!\n\r",ch);
   }

   ch->alignment = UMIN(1000, ch->alignment + 50);

   if (victim != ch)
   {
      act("$n calls forth the wrath of the Gods upon $N!",
      ch, NULL, victim, TO_NOTVICT );
      act("$n has assailed you with the wrath of the Gods!",
      ch,NULL,victim,TO_VICT);
      send_to_char("You conjure forth the wrath of the Gods!\n\r",ch);
   }

   dam = dice( level, 12 );
   if ( (ch->alignment <= victim->alignment + 100) && (ch->alignment >=
   victim->alignment - 100) )
      dam /= 2;
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_HOLY );
   return;
}

/* RT replacement demonfire spell */

void spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo)
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   if ( !IS_NPC(ch) && (victim->alignment < (ch->alignment-100)) )
   {
      victim = ch;
      send_to_char("The demons turn upon you!\n\r",ch);
   }

   ch->alignment = UMAX(-1000, ch->alignment - 50);

   if (victim != ch)
   {
      act("$n calls forth the demons of Hell upon $N!",
      ch,NULL,victim,TO_ROOM);
      act("$n has assailed you with the demons of Hell!",
      ch,NULL,victim,TO_VICT);
      send_to_char("You conjure forth the demons of hell!\n\r",ch);
   }
   dam = dice( level, 12 );
   if (ch->alignment <= victim->alignment + 100 && ch->alignment >=
   victim->alignment - 100)
      dam /= 2;
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_NEGATIVE );
}

void spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_DETECT_EVIL) )
   {
      if (victim == ch)
         send_to_char("You can already sense evil.\n\r",ch);
      else
         act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = level;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = AFF_DETECT_EVIL;
   affect_to_char( victim, &af );
   send_to_char( "Your eyes tingle.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_detect_hidden( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_DETECT_HIDDEN) )
   {
      if (victim == ch)
         send_to_char("You are already as alert as you can be. \n\r",ch);
      else
         act("$N can already sense hidden lifeforms.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = level;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_DETECT_HIDDEN;
   affect_to_char( victim, &af );
   send_to_char( "Your awareness improves.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_detect_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
   {
      if (victim == ch)
         send_to_char("You can already see invisible.\n\r",ch);
      else
         act("$N can already see invisible things.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = level;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = AFF_DETECT_INVIS;
   affect_to_char( victim, &af );
   send_to_char( "Your eyes tingle.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_DETECT_MAGIC) )
   {
      if (victim == ch)
         send_to_char("You can already sense magical auras.\n\r",ch);
      else
         act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = level;
   af.modifier  = 0;
   af.location  = APPLY_NONE;
   af.bitvector = AFF_DETECT_MAGIC;
   affect_to_char( victim, &af );
   send_to_char( "Your eyes tingle.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn; (void) level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;

   if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
   {
      if ( obj->value[3] != 0 )
         send_to_char( "You smell poisonous fumes.\n\r", ch );
      else
         send_to_char( "It looks delicious.\n\r", ch );
   }
   else
   {
      send_to_char( "It doesn't look poisoned.\n\r", ch );
   }

   return;
}



void spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf [MAX_STRING_LENGTH];
   int dam;

   if ( !IS_NPC(ch) && IS_EVIL(ch) )
      victim = ch;

   if ( IS_GOOD(victim) )
   {
      snprintf( buf, sizeof(buf), "%s protects $N.", deity_name);
      act( buf, ch, NULL, victim, TO_ROOM );
      return;
   }

   if ( IS_NEUTRAL(victim) )
   {
      act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
   else
      dam = UMAX(victim->hit, dice(level,4));
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_HOLY );
   return;
}



void spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf [MAX_STRING_LENGTH];
   int dam;

   if ( !IS_NPC(ch) && IS_GOOD(ch) )
      victim = ch;

   if ( IS_EVIL(victim) )
   {
      snprintf( buf, sizeof(buf), "%s protects $N.", deity_name);
      act( buf, ch, NULL, victim, TO_ROOM );
      return;
   }

   if ( IS_NEUTRAL(victim) )
   {
      act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
   else
      dam = UMAX(victim->hit, dice(level,4));
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_HOLY );
   return;
}


/* modified for enhanced use */

void spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   bool found = FALSE;

   if (saves_spell(level, victim))
   {
      send_to_char( "You feel a brief tingling sensation.\n\r",victim);
      send_to_char( "You failed.\n\r", ch);
      return;
   }

   /* begin running through the spells */

   if (check_dispel(level,victim,skill_lookup("armor")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("bless")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("blindness")))
   {
      found = TRUE;
      act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("calm")))
   {
      found = TRUE;
      act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("change sex")))
   {
      found = TRUE;
      act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("charm person")))
   {
      found = TRUE;
      act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("chill touch")))
   {
      found = TRUE;
      act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
   }

   if (check_dispel(level,victim,skill_lookup("curse")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect evil")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect hidden")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect invis")))
      found = TRUE;

   found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect hidden")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("detect magic")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("faerie fire")))
   {
      act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("fly")))
   {
      act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("frenzy")))
   {
      act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);
      ;
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("giant strength")))
   {
      act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("haste")))
   {
      act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("talon")))
   {
      act("$n hand's return to normal.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("lethargy")))
   {
      act("$n is looking more lively.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("insanity")))
   {
      act("$n looks less confused.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("infravision")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("invis")))
   {
      act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("mass invis")))
   {
      act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("pass door")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("protection")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("sanctuary")))
   {
      act("The white aura around $n's body vanishes.",
      victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (IS_AFFECTED(victim,AFF_SANCTUARY)
   && !saves_dispel(level, victim->level,-1)
   && !is_affected(victim,skill_lookup("sanctuary")))
   {
      REMOVE_BIT(victim->affected_by,AFF_SANCTUARY);
      act("The white aura around $n's body vanishes.",
      victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("shield")))
   {
      act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("sleep")))
      found = TRUE;

   if (check_dispel(level,victim,skill_lookup("stone skin")))
   {
      act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (check_dispel(level,victim,skill_lookup("weaken")))
   {
      act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
      found = TRUE;
   }

   if (found)
      send_to_char("Ok.\n\r",ch);
   else
      send_to_char("Spell failed.\n\r",ch);
}

void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)vo;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;

   send_to_char( "The earth trembles beneath your feet!\n\r", ch );
   act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM );

   for ( vch = char_list; vch != NULL; vch = vch_next ) {
      vch_next = vch->next;
      if ( vch->in_room == NULL )
         continue;
      if ( vch->in_room == ch->in_room ) {
         if ( vch != ch && !is_safe_spell(ch,vch,TRUE)) {
            if (IS_AFFECTED(vch,AFF_FLYING))
               damage(ch,vch,0,sn,DAM_BASH);
            else
               damage( ch, vch, level + dice(2, 8), sn, DAM_BASH );
         }
         continue;
      }

      if ( vch->in_room->area == ch->in_room->area )
         send_to_char( "The earth trembles and shivers.\n\r", vch );
   }

   return;
}

void spell_remove_invisible( int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int chance;

   if (obj->wear_loc != -1)
   {
      send_to_char("You have to be carrying it to remove invisible on it!\n\r"
      ,ch);
      return;
   }

   if (!IS_SET(obj->extra_flags,ITEM_INVIS)) {
      send_to_char("That object is not invisible!\n\r",ch);
      return;
   }

   chance = URANGE(5,(30 + URANGE( -20, (ch->level - obj->level), 20)
   + (material_table[obj->material].magical_resilience / 2)),100) -
   number_percent();

   if (chance >=0) {
      act( "$p appears from nowhere!",ch, obj, NULL, TO_ROOM);
      act( "$p appears from nowhere!",ch, obj, NULL, TO_CHAR);
      REMOVE_BIT(obj->extra_flags,ITEM_INVIS);
   }

   if ((chance >=-20) && (chance<0) ) {
      act( "$p becomes semi-solid for a moment - then disappears.",
      ch, obj, NULL, TO_ROOM);
      act( "$p becomes semi-solid for a moment - then disappears.",
      ch, obj, NULL, TO_CHAR);
   }

   if ((chance <-20)) {
      act( "$p evaporates under the magical strain",
      ch, obj, NULL, TO_ROOM);
      act( "$p evaporates under the magical strain",
      ch, obj, NULL, TO_CHAR);
      extract_obj( obj );
   }
}

void spell_remove_alignment( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int chance;
   int levdif;
   int score;

   if (obj->wear_loc != -1)
   {
      send_to_char("You have to be carrying it to remove alignment on it!\n\r"
      ,ch);
      return;
   }

   if ( (!IS_SET(obj->extra_flags,ITEM_ANTI_EVIL)) &&
   (!IS_SET(obj->extra_flags,ITEM_ANTI_GOOD)) &&
   (!IS_SET(obj->extra_flags,ITEM_ANTI_NEUTRAL)) ) {
      send_to_char("That object has no alignment!\n\r",ch);
      return;
   }

   levdif = URANGE( -20, (ch->level - obj->level), 20);
   chance = levdif / 2;

   chance = chance + material_table[obj->material].magical_resilience;

   chance = URANGE( 5, chance, 100 );

   score = chance - number_percent() ;

   if ( (IS_SET(obj->extra_flags,ITEM_ANTI_EVIL) && IS_GOOD(ch) )
   || (IS_SET(obj->extra_flags,ITEM_ANTI_GOOD) && IS_EVIL(ch) ) )
      chance = chance - 10;

   if ( (score <=20) ) {
      act( "The powerful nature of $n's spell removes some of $m alignment!",
      ch, obj, NULL, TO_ROOM);
      act( "Your spell removes some of your alignment!",
      ch, obj, NULL, TO_CHAR);
      if IS_GOOD(ch)
         ch->alignment-=(30-score);
      if IS_EVIL(ch)
         ch->alignment+=(30-score);
      ch->alignment = URANGE(-1000,ch->alignment,1000);
   }

   if (score >= 0) {
      act( "$p glows grey.", ch, obj, NULL, TO_ROOM );
      act( "$p glows grey.", ch, obj, NULL, TO_CHAR );
      REMOVE_BIT(obj->extra_flags,ITEM_ANTI_GOOD);
      REMOVE_BIT(obj->extra_flags,ITEM_ANTI_EVIL);
      REMOVE_BIT(obj->extra_flags,ITEM_ANTI_NEUTRAL);
   }

   if ((score >=-20) && (score < 0)) {
      act( "$p appears unaffected.", ch, obj, NULL, TO_ROOM );
      act( "$p appears unaffected.", ch, obj, NULL, TO_CHAR );
   }

   if (score < -20) {
      act( "$p shivers violently and explodes!", ch, obj, NULL,TO_ROOM );
      act( "$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR );
      extract_obj(obj);
   }
}

void spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   AFFECT_DATA *paf;
   int result, fail;
   int ac_bonus, added;
   bool ac_found = FALSE;

   if (obj->item_type != ITEM_ARMOR)
   {
      send_to_char("That isn't an armor.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be enchanted.\n\r",ch);
      return;
   }

   /* this means they have no bonus */
   ac_bonus = 0;
   fail = 15; /* base 15% chance of failure */
   /* TheMoog added material fiddling */
   fail += ( (100 - material_table[obj->material].magical_resilience)
   / 3);

   /* find the bonuses */

   if (!obj->enchanted)
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
         if ( paf->location == APPLY_AC )
         {
            ac_bonus = paf->modifier;
            ac_found = TRUE;
            fail += 5 * (ac_bonus * ac_bonus);
         }

         else  /* things get a little harder */
         fail += 20;
      }

   for ( paf = obj->affected; paf != NULL; paf = paf->next )
   {
      if ( paf->location == APPLY_AC )
      {
         ac_bonus = paf->modifier;
         ac_found = TRUE;
         fail += 5 * (ac_bonus * ac_bonus);
      }

      else /* things get a little harder */
      fail += 20;
   }

   /* apply other modifiers */
   fail -= level;

   if (IS_OBJ_STAT(obj,ITEM_BLESS))
      fail -= 15;
   if (IS_OBJ_STAT(obj,ITEM_GLOW))
      fail -= 5;

   fail = URANGE(5,fail,95);

   result = number_percent();

   /* the moment of truth */
   if (result < (fail / 5))  /* item destroyed */
   {
      act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
      act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
      extract_obj(obj);
      return;
   }

   if (result < (fail / 2)) /* item disenchanted */
   {
      AFFECT_DATA *paf_next;

      act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
      act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
      obj->enchanted = TRUE;

      /* remove all affects */
      for (paf = obj->affected; paf != NULL; paf = paf_next)
      {
         paf_next = paf->next;
         paf->next = affect_free;
         affect_free = paf;
      }
      obj->affected = NULL;

      /* clear all flags */
      obj->extra_flags = 0;
      return;
   }

   if ( result <= fail )  /* failed, no bad result */
   {
      send_to_char("Nothing seemed to happen.\n\r",ch);
      return;
   }

   /* okay, move all the old flags into new vectors if we have to */
   if (!obj->enchanted)
   {
      AFFECT_DATA *af_new;
      obj->enchanted = TRUE;

      for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
      {
         if (affect_free == NULL)
            af_new = alloc_perm(sizeof(*af_new));
         else
         {
            af_new = affect_free;
            affect_free = affect_free->next;
         }

         af_new->next = obj->affected;
         obj->affected = af_new;

         af_new->type  = UMAX(0,paf->type);
         af_new->level = paf->level;
         af_new->duration = paf->duration;
         af_new->location = paf->location;
         af_new->modifier = paf->modifier;
         af_new->bitvector   = paf->bitvector;
      }
   }

   if (result <= (100 - level/5))  /* success! */
   {
      act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
      act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
      SET_BIT(obj->extra_flags, ITEM_MAGIC);
      added = -1;
   }

   else  /* exceptional enchant */
   {
      act("$p glows a brillant gold!",ch,obj,NULL,TO_CHAR);
      act("$p glows a brillant gold!",ch,obj,NULL,TO_ROOM);
      SET_BIT(obj->extra_flags,ITEM_MAGIC);
      SET_BIT(obj->extra_flags,ITEM_GLOW);
      added = -2;
   }

   /* now add the enchantments */

   if (obj->level < LEVEL_HERO)
      obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

   if (ac_found)
   {
      for ( paf = obj->affected; paf != NULL; paf = paf->next)
      {
         if ( paf->location == APPLY_AC)
         {
            paf->type = sn;
            paf->modifier += added;
            paf->level = UMAX(paf->level,level);
         }
      }
   }
   else /* add a new affect */
   {
      if (affect_free == NULL)
         paf = alloc_perm(sizeof(*paf));
      else
      {
         paf = affect_free;
         affect_free = affect_free->next;
      }

      paf->type   = sn;
      paf->level  = level;
      paf->duration  = -1;
      paf->location  = APPLY_AC;
      paf->modifier  =  added;
      paf->bitvector  = 0;
      paf->next   = obj->affected;
      obj->affected  = paf;
   }

}




void spell_enchant_weapon( int sn, int level, CHAR_DATA *ch, void *vo )
{
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   AFFECT_DATA *paf;
   int result, fail, no_ench_num = 0;
   int hit_bonus, dam_bonus, added, modifier;
   bool hit_found = FALSE, dam_found = FALSE;

   if ((obj->item_type != ITEM_WEAPON) && (obj->item_type != ITEM_ARMOR))
   {
      send_to_char("That isn't a weapon or armour.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be enchanted.\n\r",ch);
      return;
   }

   /* this means they have no bonus */
   hit_bonus = 0;
   dam_bonus = 0;
   modifier = 1;
   fail = 15; /* base 15% chance of failure */

   /* Nice little touch for IMPs */
   if (get_trust(ch) == MAX_LEVEL) fail = -16535;

   /* TheMoog added material fiddling */
   fail += ( (100 - material_table[obj->material].magical_resilience)
   / 3);
   if (obj->item_type == ITEM_ARMOR)
   {
      fail += 25; /* harder to enchant armor with weapon */
      modifier = 2;
   }
   /* find the bonuses */

   if (!obj->enchanted)
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
         if ( paf->location == APPLY_HITROLL )
         {
            hit_bonus = paf->modifier;
            hit_found = TRUE;
            fail += 2 * (hit_bonus * hit_bonus) * modifier;
         }

         else if (paf->location == APPLY_DAMROLL )
         {
            dam_bonus = paf->modifier;
            dam_found = TRUE;
            fail += 2 * (dam_bonus * dam_bonus) * modifier;
         }

         else  /* things get a little harder */
         fail += 25 * modifier;
      }

   for ( paf = obj->affected; paf != NULL; paf = paf->next )
   {
      if ( paf->location == APPLY_HITROLL )
      {
         hit_bonus = paf->modifier;
         hit_found = TRUE;
         fail += 2 * (hit_bonus * hit_bonus) * modifier;
      }

      else if (paf->location == APPLY_DAMROLL )
      {
         dam_bonus = paf->modifier;
         dam_found = TRUE;
         fail += 2 * (dam_bonus * dam_bonus) * modifier;
      }

      else /* things get a little harder */
         fail += 25* modifier;
   }

   /* apply other modifiers */
   fail -= 3 * level/2;

   if (IS_OBJ_STAT(obj,ITEM_BLESS))
      fail -= 15;
   if (IS_OBJ_STAT(obj,ITEM_GLOW))
      fail -= 5;

   fail = URANGE(5,fail,95);

   result = number_percent();

   /* We don't want armor, with more than 2 ench. hit&dam */
   if (obj->item_type == ITEM_ARMOR && (get_trust(ch) < MAX_LEVEL))
   {
      for ( paf = obj->affected; paf != NULL; paf = paf->next)
      {
         if ( (paf->type == sn) && (paf->location == APPLY_DAMROLL) )
            break;
      }
      if (paf != NULL)
         /* Check for 2+ dam */
         if (paf->modifier >= 2)
         {
            no_ench_num = number_range(1, 3);
         }

   }

   /* The moment of truth */
   if (result < (fail / 5) || (no_ench_num==1) )  /* item destroyed */
   {
      act("$p shivers violently and explodes!",ch,obj,NULL,TO_CHAR);
      act("$p shivers violently and explodeds!",ch,obj,NULL,TO_ROOM);
      extract_obj(obj);
      return;
   }

   if (result < (fail / 2) || (no_ench_num==2) ) /* item disenchanted */
   {
      AFFECT_DATA *paf_next;

      act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
      act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
      obj->enchanted = TRUE;

      /* remove all affects */
      for (paf = obj->affected; paf != NULL; paf = paf_next)
      {
         paf_next = paf->next;
         paf->next = affect_free;
         affect_free = paf;
      }
      obj->affected = NULL;

      /* clear all flags */
      obj->extra_flags = 0;
      return;
   }

   if ( result <= fail || (no_ench_num==3) )  /* failed, no bad result */
   {
      send_to_char("Nothing seemed to happen.\n\r",ch);
      return;
   }

   /* okay, move all the old flags into new vectors if we have to */
   if (!obj->enchanted)
   {
      AFFECT_DATA *af_new;
      obj->enchanted = TRUE;

      for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
      {
         if (affect_free == NULL)
            af_new = alloc_perm(sizeof(*af_new));
         else
         {
            af_new = affect_free;
            affect_free = affect_free->next;
         }

         af_new->next = obj->affected;
         obj->affected = af_new;

         af_new->type  = UMAX(0,paf->type);
         af_new->level = paf->level;
         af_new->duration = paf->duration;
         af_new->location = paf->location;
         af_new->modifier = paf->modifier;
         af_new->bitvector   = paf->bitvector;
      }
   }


   if (result <= (100 - level/5))  /* success! */
   {
      act("$p glows blue.",ch,obj,NULL,TO_CHAR);
      act("$p glows blue.",ch,obj,NULL,TO_ROOM);
      SET_BIT(obj->extra_flags, ITEM_MAGIC);
      added = 1;
   }

   else  /* exceptional enchant */
   {
      act("$p glows a brillant blue!",ch,obj,NULL,TO_CHAR);
      act("$p glows a brillant blue!",ch,obj,NULL,TO_ROOM);
      SET_BIT(obj->extra_flags,ITEM_MAGIC);
      SET_BIT(obj->extra_flags,ITEM_GLOW);
      added = 2;
   }

   /* now add the enchantments */

   if (obj->level < LEVEL_HERO - 1)
      obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

   if (dam_found)
   {
      for ( paf = obj->affected; paf != NULL; paf = paf->next)
      {
         if ( paf->location == APPLY_DAMROLL)
         {
            paf->type = sn;
            paf->modifier += added;
            paf->level = UMAX(paf->level,level);
            if (paf->modifier > 4)
               SET_BIT(obj->extra_flags,ITEM_HUM);
         }
      }
   }
   else /* add a new affect */
   {
      if (affect_free == NULL)
         paf = alloc_perm(sizeof(*paf));
      else
      {
         paf = affect_free;
         affect_free = affect_free->next;
      }

      paf->type   = sn;
      paf->level  = level;
      paf->duration  = -1;
      paf->location  = APPLY_DAMROLL;
      paf->modifier  =  added;
      paf->bitvector  = 0;
      paf->next   = obj->affected;
      obj->affected  = paf;
   }

   if (hit_found)
   {
      for ( paf = obj->affected; paf != NULL; paf = paf->next)
      {
         if ( paf->location == APPLY_HITROLL)
         {
            paf->type = sn;
            paf->modifier += added;
            paf->level = UMAX(paf->level,level);
            if (paf->modifier > 4)
               SET_BIT(obj->extra_flags,ITEM_HUM);
         }
      }
   }
   else /* add a new affect */
   {
      if (affect_free == NULL)
         paf = alloc_perm(sizeof(*paf));
      else
      {
         paf = affect_free;
         affect_free = affect_free->next;
      }

      paf->type       = sn;
      paf->level      = level;
      paf->duration   = -1;
      paf->location   = APPLY_HITROLL;
      paf->modifier   =  added;
      paf->bitvector  = 0;
      paf->next       = obj->affected;
      obj->affected   = paf;
   }
   /* Make armour become level 50 */
   if ((obj->item_type == ITEM_ARMOR) && (obj->level < 50)) {
      act( "$p looks way better than it did before!", ch, obj, NULL,
      TO_CHAR);
      obj->level = 50;
   }
}

void spell_protect_container (int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;

   if (obj->item_type != ITEM_CONTAINER)
   {
      send_to_char ("That isn't a container.\n\r", ch);
      return;
   }

   if (IS_OBJ_STAT (obj, ITEM_PROTECT_CONTAINER))
   {
      act ("$p is already protected!", ch, obj, NULL, TO_CHAR);
      return;
   }

   if (ch->gold < 50000)
   {
      send_to_char ("You need 50,000 gold coins to protect a container\n\r", ch);
      return;
   }

   ch->gold -= 50000;
   SET_BIT (obj->extra_flags, ITEM_PROTECT_CONTAINER);
   act ("$p is now protected!", ch, obj, NULL, TO_CHAR);
}

/*PGW A new group to give Barbarians a helping hand*/
void spell_vorpal( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int mana;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("This isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be made vorpal.\n\r",ch);
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   if ( IS_NPC(ch) && ch->gold < (mana*100))
   {
      send_to_char("You can't afford to!\n\r", ch);
      return;
   }

   ch->gold -= (mana*100);
   SET_BIT(obj->value[4], WEAPON_VORPAL);
   send_to_char("You create a flaw in the universe and place it on your blade!\n\r", ch);
   return;
}

void spell_venom( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int mana;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be venomed.\n\r",ch);
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   if ( IS_NPC(ch) && ch->gold < (mana*100))
   {
      send_to_char("You can't afford to!\n\r", ch);
      return;
   }

   ch->gold -= (mana*100);
   SET_BIT(obj->value[4], WEAPON_POISONED);
   send_to_char("You coat the blade in poison!\n\r", ch);
   return;
}

void spell_black_death( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int mana;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be plagued.\n\r",ch);
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   if ( IS_NPC(ch) && ch->gold < (mana*100))
   {
      send_to_char("You can't afford to!\n\r", ch);
      return;
   }

   ch->gold -= (mana*100);
   SET_BIT(obj->value[4], WEAPON_PLAGUED);
   send_to_char("Your use your cunning and skill to plague the weapon!\n\r", ch);
   return;
}

void spell_damnation( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int mana;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("You do not have that item in your inventory.\n\r",ch);
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   if ( IS_NPC(ch) && ch->gold < (mana*100))
   {
      send_to_char("You can't afford to!\n\r", ch);
      return;
   }

   ch->gold -= (mana*100);
   SET_BIT(obj->extra_flags, ITEM_NODROP);
   SET_BIT(obj->extra_flags, ITEM_NOREMOVE);
   send_to_char("You turn red in the face and curse your weapon into the pits of hell!\n\r", ch);
   return;
}

void spell_vampire( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("The item must be carried to be vampiric.\n\r",ch);
      return;
   }

   if ( IS_IMMORTAL(ch) )
   {
      SET_BIT(obj->value[4], WEAPON_VAMPIRIC);
      send_to_char("You suck the life force from the weapon leaving it hungry for blood.\n\r", ch);
   }
   return;
}

void spell_tame_lightning( int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   int mana;

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That isn't a weapon.\n\r",ch);
      return;
   }

   if (obj->wear_loc != -1)
   {
      send_to_char("You do not have that item in your inventory.\n\r",ch);
      return;
   }

   if (IS_SET(obj->value[4], WEAPON_ACID))
   {
      send_to_char("Acid and lightning do not mix.\n\r",ch);
      return;
   }

   if (ch->level + 2 == get_skill_level(ch,sn))
      mana = 50;
   else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - get_skill_level(ch,sn)));

   if ( IS_NPC(ch) && ch->gold < (mana*100))
   {
      send_to_char("You can't afford to!\n\r", ch);
      return;
   }

   ch->gold -= (mana*100);
   obj->value[3] = ATTACK_TABLE_INDEX_TAME_LIGHTNING;
   SET_BIT(obj->value[4], WEAPON_LIGHTNING);
   send_to_char( "You summon a MASSIVE storm.\n\rHolding your weapon aloft you call lightning down from the sky. \n\rThe lightning swirls around it - you have |YTAMED|w the |YLIGHTNING|w.\n\r", ch);

   return;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   if ( saves_spell( level, victim ) )
   {
      send_to_char("You feel a momentary chill.\n\r",victim);
      return;
   }

   ch->alignment = UMAX(-1000, ch->alignment - 50);
   if ( victim->level <= 2 )
   {
      dam       = victim->hit + 1;
   }
   else
   {
      if (victim->in_room->vnum != CHAL_ROOM )
         gain_exp( victim, 0 -  5 * number_range( level/2, 3 * level / 2 ) );
      victim->mana   /= 2;
      victim->move   /= 2;
      dam       = dice(1, level);
      ch->hit     += dam;
   }

   send_to_char("You feel your life slipping away!\n\r",victim);
   send_to_char("Wow....what a rush!\n\r",ch);
   damage( ch, victim, dam, sn, DAM_NEGATIVE );

   return;
}



void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      0,   0,   0,   0,   0,     0,   0,   0,   0,   0,
      0,   0,   0,   0,  30,    35,  40,  45,  50,  55,
      60,  65,  70,  75,  80,    82,  84,  86,  88,  90,
      92,  94,  96,  98, 100,   102, 104, 106, 108, 110,
      112, 114, 116, 118, 120,   122, 124, 126, 128, 130
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_FIRE );
   return;
}



void spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   dam = dice(6, 8);
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_FIRE );
   return;
}



void spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) )
      return;
   af.type      = sn;
   af.level    = level;
   af.duration  = level;
   af.location  = APPLY_AC;
   af.modifier  = 2 * level;
   af.bitvector = AFF_FAERIE_FIRE;
   affect_to_char( victim, &af );
   send_to_char( "You are surrounded by a pink outline.\n\r", victim );
   act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM );
   return;
}


void spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   CHAR_DATA *ich;

   act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
   send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

   for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
   {
      if ( !IS_NPC(ich) && IS_SET(ich->act, PLR_WIZINVIS) )
         continue;
      if ( !IS_NPC(ich) && IS_SET(ich->act, PLR_PROWL) )
         continue;

      if ( ich == ch || saves_spell( level, ich ) )
         continue;

      affect_strip ( ich, gsn_invis       );
      affect_strip ( ich, gsn_mass_invis     );
      affect_strip ( ich, gsn_sneak       );
      REMOVE_BIT   ( ich->affected_by, AFF_HIDE );
      REMOVE_BIT   ( ich->affected_by, AFF_INVISIBLE  );
      REMOVE_BIT   ( ich->affected_by, AFF_SNEAK   );
      act( "$n is revealed!", ich, NULL, NULL, TO_ROOM );
      send_to_char( "You are revealed!\n\r", ich );
   }

   return;
}



void spell_fly( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_FLYING) )
   {
      if (victim == ch)
         send_to_char("You are already airborne.\n\r",ch);
      else
         act("$N doesn't need your help to fly.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = level + 3;
   af.location  = 0;
   af.modifier  = 0;
   af.bitvector = AFF_FLYING;
   affect_to_char( victim, &af );
   send_to_char( "Your feet rise off the ground.\n\r", victim );
   act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM );
   return;
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo)
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   /*  OBJ_DATA *wield = get_eq_char( ch, WEAR_WIELD );*/

   AFFECT_DATA af;

   if (is_affected(victim,sn) || IS_AFFECTED(victim,AFF_BERSERK))
   {
      if (victim == ch)
         send_to_char("You are already in a frenzy.\n\r",ch);
      else
         act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR);
      return;
   }

   if (is_affected(victim,skill_lookup("calm")))
   {
      if (victim == ch)
         send_to_char("Why don't you just relax for a while?\n\r",ch);
      else
         act("$N doesn't look like $e wants to fight anymore.",
         ch,NULL,victim,TO_CHAR);
      return;
   }

   if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
   (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
   (IS_EVIL(ch) && !IS_EVIL(victim))
   )
   {
      act("Your god doesn't seem to like $N",ch,NULL,victim,TO_CHAR);
      return;
   }

   af.type    = sn;
   af.level   = level;
   af.duration   = level / 3;
   af.modifier  = level / 6;
   af.bitvector = 0;

   af.location  = APPLY_HITROLL;
   affect_to_char(victim,&af);

   af.location  = APPLY_DAMROLL;
   affect_to_char(victim,&af);

   af.modifier  = 10 * (level / 6);
   af.location  = APPLY_AC;
   affect_to_char(victim,&af);

   send_to_char("You are filled with holy wrath!\n\r",victim);
   act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM);

   /*  if ( (wield !=NULL) && (wield->item_type == ITEM_WEAPON) &&
         (IS_SET(wield->value[4], WEAPON_FLAMING)))
       send_to_char("Your great energy causes your weapon to burst into flame.\n\r",ch);
     wield->value[3] = 29;*/
}


/* RT ROM-style gate */

void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   CHAR_DATA *victim;
   bool gate_pet;

   if ( ( victim = get_char_world( ch, target_name ) ) == NULL
   ||   victim == ch
   ||   victim->in_room == NULL
   ||   !can_see_room(ch,victim->in_room)
   ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
   ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
   ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
   ||   victim->level >= level + 3
   ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
   ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
   ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON))
   ||   (IS_NPC(victim) && saves_spell( level, victim ) ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
      gate_pet = TRUE;
   else
      gate_pet = FALSE;

   act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM);
   send_to_char("You step through a gate and vanish.\n\r",ch);
   char_from_room(ch);
   char_to_room(ch,victim->in_room);

   act("$n has arrived through a gate.",ch,NULL,NULL,TO_ROOM);
   do_look(ch,"auto");

   if (gate_pet && (ch->pet->in_room != ch->in_room))
   {
      act("$n steps through a gate and vanishes.",ch->pet,NULL,NULL,TO_ROOM);
      send_to_char("You step through a gate and vanish.\n\r",ch->pet);
      char_from_room(ch->pet);
      char_to_room(ch->pet,victim->in_room);
      act("$n has arrived through a gate.",ch->pet,NULL,NULL,TO_ROOM);
      do_look(ch->pet,"auto");
   }
}



void spell_giant_strength( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) )
   {
      if (victim == ch)
         send_to_char("You are already as strong as you can get!\n\r",ch);
      else
         act("$N can't get any stronger.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level    = level;
   af.duration  = level;
   af.location  = APPLY_STR;
   af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
   af.bitvector = 0;
   affect_to_char( victim, &af );
   send_to_char( "Your muscles surge with heightened power!\n\r", victim );
   act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM);
   return;
}



void spell_harm( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   dam = dice( level, 12);
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_HARM );
   return;
}

void spell_regeneration(int sn, int level, CHAR_DATA *ch, void *vo)
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if (is_affected( victim, sn) || IS_AFFECTED(victim, AFF_REGENERATION))
   {
      if (victim == ch)
         send_to_char("You are already vibrant!\n\r", ch);
      else
         act("$N is already as vibrant as $e can be.",
         ch, NULL, victim, TO_CHAR);
      return;
   }
   af.type = sn;
   af.level = level;
   if (victim == ch)
      af.duration = level/2;
   else
   af.duration = level/4;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = AFF_REGENERATION;
   affect_to_char(victim, &af);
   send_to_char("You feel vibrant!\n\r", victim);
   act("$n is feeling more vibrant.", victim, NULL, NULL, TO_ROOM);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return;
}

/* RT haste spell */

void spell_haste( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_HASTE)
   ||   IS_SET(victim->off_flags,OFF_FAST))
   {
      if (victim == ch)
         send_to_char("You can't move any faster!\n\r",ch);
      else
         act("$N is already moving as fast as $e can.",
         ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   if (victim == ch)
      af.duration  = level/2;
   else
   af.duration  = level/4;
   af.location  = APPLY_DEX;
   af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
   af.bitvector = AFF_HASTE;
   affect_to_char( victim, &af );
   send_to_char( "You feel yourself moving more quickly.\n\r", victim );
   act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}

/* Moog's insanity spell */
void spell_insanity( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected(victim,sn) ) {
      if (victim == ch)
         send_to_char( "You're mad enough!\n\r",ch);
      else
         send_to_char( "They're as insane as they can be!\n\r",ch);
      return;
   }

   if (saves_spell( level, victim)) {
      act( "$N is unaffected.", ch, NULL, victim, TO_CHAR);
      return;
   }

   af.type      = sn;
   af.level     = level;
   af.duration  = 5;
   af.location  = APPLY_WIS;
   af.modifier  = -(1 + (level >=20) + (level >= 30) + (level>=50) +
   (level >=75) + (level >= 91));
   af.bitvector = 0;

   affect_to_char( victim, &af );
   af.location  = APPLY_INT;
   affect_to_char( victim, &af );
   if ( ch != victim)
      act( "$N suddenly appears very confused.", ch, NULL, victim, TO_CHAR );
   send_to_char( "You feel a cloud of confusion and madness descend upon you.\n\r", victim );
}

/* PGW  lethargy spell */

void spell_lethargy( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_LETHARGY)
   ||   IS_SET(victim->off_flags,OFF_SLOW))
   {
      if (victim == ch)
         send_to_char("Your heart beat is as low as it can go!\n\r",ch);
      else
         act("$N has a slow enough heart-beat already.",
         ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   if (victim == ch)
      af.duration  = level/2;
   else
   af.duration  = level/4;
   af.location  = APPLY_DEX;
   af.modifier  = -(1 + (level >= 18) + (level >= 25) + (level >= 32));
   af.bitvector = AFF_LETHARGY;
   affect_to_char( victim, &af );
   send_to_char( "You feel your heart-beat slowing.\n\r", victim );
   act("$n slows nearly to a stand-still.",victim,NULL,NULL,TO_ROOM);
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}


void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int heal;

   heal = 2*(dice(3, 8) + level - 6);
   victim->hit = UMIN( victim->hit + heal, victim->max_hit );
   update_pos( victim );
   send_to_char( "A warm feeling fills your body.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}

/* RT really nasty high-level attack spell */
/* PGW it was crap so i changed it*/
void spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)vo;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   int dam;
   int bless_num, curse_num, frenzy_num;

   bless_num = skill_lookup("bless");
   curse_num = skill_lookup("curse");
   frenzy_num = skill_lookup("frenzy");

   act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM);
   send_to_char("You utter a word of divine power.\n\r",ch);

   for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
   {
      vch_next = vch->next_in_room;

      if ((IS_GOOD(ch) && IS_GOOD(vch)) ||
      (IS_EVIL(ch) && IS_EVIL(vch)) ||
      (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)) )
      {
         send_to_char("You feel full more powerful.\n\r",vch);
         spell_frenzy(frenzy_num,level,ch,(void *) vch);
         spell_bless(bless_num,level,ch,(void *) vch);
      }

      else if ((IS_GOOD(ch) && IS_EVIL(vch)) ||
      (IS_EVIL(ch) && IS_GOOD(vch)) )
      {
         if (!is_safe_spell(ch,vch,TRUE))
         {
            spell_curse(curse_num,level,ch,(void *) vch);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,14);
            damage(ch,vch,dam,sn,DAM_ENERGY);
         }
      }

      else if (IS_NEUTRAL(ch))
      {
         if (!is_safe_spell(ch,vch,TRUE))
         {
            spell_curse(curse_num,level/2,ch,(void *) vch);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,10);
            damage(ch,vch,dam,sn,DAM_ENERGY);
         }
      }
   }

   send_to_char("You feel drained.\n\r",ch);
   ch->move = 10;
   ch->hit = (ch->hit *3) /4;

   return;
}

void spell_identify( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;
   OBJ_DATA *obj = (OBJ_DATA *) vo;
   char buf[MAX_STRING_LENGTH];
   AFFECT_DATA *paf;

   snprintf( buf, sizeof(buf),
   "Object '%s' is type %s, extra flags %s.\n\rWeight is %d, value is %d, level is %d.\n\r",

   obj->name,
   item_type_name( obj ),
   extra_bit_name( obj->extra_flags ),
   obj->weight,
   obj->cost,
   obj->level
   );
   send_to_char( buf, ch );

   if ((obj->material != MATERIAL_NONE) &&
   (obj->material != MATERIAL_DEFAULT) ) {
      snprintf( buf, sizeof(buf), "Made of %s.\n\r",
      material_table[obj->material].material_name);
      send_to_char( buf, ch );
   }

   switch ( obj->item_type )
   {
   case ITEM_SCROLL:
   case ITEM_POTION:
   case ITEM_PILL:
   case ITEM_BOMB:
      snprintf( buf, sizeof(buf), "Level %d spells of:", obj->value[0] );
      send_to_char( buf, ch );

      if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[1]].name, ch );
         send_to_char( "'", ch );
      }

      if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[2]].name, ch );
         send_to_char( "'", ch );
      }

      if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
      }

      if ( obj->value[4] >= 0 && obj->value[4] < MAX_SKILL
      && obj->item_type == ITEM_BOMB )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[4]].name, ch );
         send_to_char( "'", ch );
      }

      send_to_char( ".\n\r", ch );
      break;

   case ITEM_WAND:
   case ITEM_STAFF:
      snprintf( buf, sizeof(buf), "Has %d(%d) charges of level %d",
      obj->value[1], obj->value[2], obj->value[0] );
      send_to_char( buf, ch );

      if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
      }

      send_to_char( ".\n\r", ch );
      break;

   case ITEM_WEAPON:
      send_to_char("Weapon type is ",ch);
      switch (obj->value[0])
      {
         case(WEAPON_EXOTIC) :
         send_to_char("exotic.\n\r",ch);
         break;
         case(WEAPON_SWORD)  :
         send_to_char("sword.\n\r",ch);
         break;
         case(WEAPON_DAGGER) :
         send_to_char("dagger.\n\r",ch);
         break;
         case(WEAPON_SPEAR)  :
         send_to_char("spear/staff.\n\r",ch);
         break;
         case(WEAPON_MACE)   :
         send_to_char("mace/club.\n\r",ch);
         break;
         case(WEAPON_AXE) :
         send_to_char("axe.\n\r",ch);
         break;
         case(WEAPON_FLAIL)  :
         send_to_char("flail.\n\r",ch);
         break;
         case(WEAPON_WHIP)   :
         send_to_char("whip.\n\r",ch);
         break;
         case(WEAPON_POLEARM):
         send_to_char("polearm.\n\r",ch);
         break;
      default    :
         send_to_char("unknown.\n\r",ch);
         break;
      }
      if ( (obj->value[4] != 0) && (obj->item_type == ITEM_WEAPON) )
      {
         send_to_char("Weapon flags:", ch);
         if ( IS_SET(obj->value[4], WEAPON_FLAMING) )
            send_to_char(" flaming", ch);
         if ( IS_SET(obj->value[4], WEAPON_FROST) )
            send_to_char(" frost", ch);
         if ( IS_SET(obj->value[4], WEAPON_VAMPIRIC) )
            send_to_char(" vampiric", ch);
         if ( IS_SET(obj->value[4], WEAPON_SHARP) )
            send_to_char(" sharp", ch);
         if ( IS_SET(obj->value[4], WEAPON_VORPAL) )
            send_to_char(" vorpal", ch);
         if ( IS_SET(obj->value[4], WEAPON_TWO_HANDS) )
            send_to_char(" two-handed", ch);
         if ( IS_SET(obj->value[4], WEAPON_POISONED) )
            send_to_char(" poisoned", ch);
         if ( IS_SET(obj->value[4], WEAPON_PLAGUED) )
            send_to_char(" death", ch);
         if ( IS_SET(obj->value[4], WEAPON_ACID) )
            send_to_char(" acid", ch);
         if ( IS_SET(obj->value[4], WEAPON_LIGHTNING) )
            send_to_char(" lightning", ch);
         send_to_char(".\n\r", ch);
      }
      if (obj->pIndexData->new_format)
         snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d).\n\r",
         obj->value[1],obj->value[2],
         (1 + obj->value[2]) * obj->value[1] / 2);
      else
      snprintf( buf, sizeof(buf), "Damage is %d to %d (average %d).\n\r",
      obj->value[1], obj->value[2],
      ( obj->value[1] + obj->value[2] ) / 2 );
      send_to_char( buf, ch );
      break;

   case ITEM_ARMOR:
      snprintf( buf, sizeof(buf),
      "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
      send_to_char( buf, ch );
      break;
   }

   if (!obj->enchanted)
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
         if ( paf->location != APPLY_NONE && paf->modifier != 0 )
         {
            snprintf( buf, sizeof(buf), "Affects %s by %d.\n\r",
            affect_loc_name( paf->location ), paf->modifier );
            send_to_char( buf, ch );
         }
      }

   for ( paf = obj->affected; paf != NULL; paf = paf->next )
   {
      if ( paf->location != APPLY_NONE && paf->modifier != 0 )
      {
         snprintf( buf, sizeof(buf), "Affects %s by %d.\n\r",
         affect_loc_name( paf->location ), paf->modifier );
         send_to_char( buf, ch );
      }
   }

   return;
}



void spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_INFRARED) )
   {
      if (victim == ch)
         send_to_char("You can already see in the dark.\n\r",ch);
      else
         act("$N already has infravision.\n\r",ch,NULL,victim,TO_CHAR);
      return;
   }
   act( "$n's eyes glow red.\n\r", ch, NULL, NULL, TO_ROOM );
   af.type      = sn;
   af.level    = level;
   af.duration  = 2 * level;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_INFRARED;
   affect_to_char( victim, &af );
   send_to_char( "Your eyes glow red.\n\r", victim );
   return;
}



void spell_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
      return;

   act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
   af.type      = sn;
   af.level     = level;
   af.duration  = 24;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_INVISIBLE;
   affect_to_char( victim, &af );
   send_to_char( "You fade out of existence.\n\r", victim );
   return;
}



void spell_know_alignment( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   char *msg;
   int ap;

   ap = victim->alignment;

   if ( ap >  700 ) msg = "$N has a pure and good aura.";
   else if ( ap >  350 ) msg = "$N is of excellent moral character.";
   else if ( ap >  100 ) msg = "$N is often kind and thoughtful.";
   else if ( ap > -100 ) msg = "$N doesn't have a firm moral commitment.";
   else if ( ap > -350 ) msg = "$N lies to $S friends.";
   else if ( ap > -700 ) msg = "$N is a black-hearted murderer.";
   else msg = "$N is the embodiment of pure evil!.";

   act( msg, ch, NULL, victim, TO_CHAR );
   return;
}



void spell_lightning_bolt( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      0,  0,  0,  0,  0,   0,  0,  0, 25, 28,
      31, 34, 37, 40, 40,  41, 42, 42, 43, 44,
      44, 45, 46, 46, 47,  48, 48, 49, 50, 50,
      51, 52, 52, 53, 54,  54, 55, 56, 56, 57,
      58, 58, 59, 60, 60,  61, 62, 62, 63, 64
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_LIGHTNING );
   return;
}



void spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   char buf[MAX_INPUT_LENGTH];
   char buffer[200*90]; /* extra paranoia factor */
   OBJ_DATA *obj;
   OBJ_DATA *in_obj;
   bool found;
   int number = 0, max_found;

   found = FALSE;
   number = 0;
   buffer[0] = '\0';
   max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

   for ( obj = object_list; obj != NULL; obj = obj->next )
   {
      if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name )
      ||   (!IS_IMMORTAL(ch) && number_percent() > 2 * level)
      ||   ch->level < obj->level
      ||   IS_SET(obj->extra_flags, ITEM_NO_LOCATE) )
         continue;

      found = TRUE;
      number++;

      for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
         ;

      if ( in_obj->carried_by != NULL /*&& can_see TODO wth was this supposed to be?*/ )
      {
         if (IS_IMMORTAL(ch))
         {
            snprintf( buf, sizeof(buf), "%s carried by %s in %s [Room %d]\n\r",
            obj->short_descr, PERS(in_obj->carried_by, ch),
            in_obj->carried_by->in_room->name,
            in_obj->carried_by->in_room->vnum);
         }
         else
         snprintf( buf, sizeof(buf), "%s carried by %s\n\r",
         obj->short_descr, PERS(in_obj->carried_by, ch) );
      }
      else
      {
         if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
            snprintf( buf, sizeof(buf), "%s in %s [Room %d]\n\r",
            obj->short_descr,
            in_obj->in_room->name, in_obj->in_room->vnum);
         else
         snprintf( buf, sizeof(buf), "%s in %s\n\r",
         obj->short_descr, in_obj->in_room == NULL
         ? "somewhere" : in_obj->in_room->name );
      }

      buf[0] = UPPER(buf[0]);
      strcat(buffer,buf);

      if (number >= max_found)
         break;
   }

   if ( !found )
      send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
   else if (ch->lines)
      page_to_char(buffer,ch);
   else
      send_to_char(buffer,ch);

   return;
}



void spell_magic_missile( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const sh_int dam_each[] =
   {
      0,
      3,  3,  4,  4,  5,   6,  6,  6,  6,  6,
      7,  7,  7,  7,  7,   8,  8,  8,  8,  8,
      9,  9,  9,  9,  9,  10, 10, 10, 10, 10,
      11, 11, 11, 11, 11,  12, 12, 12, 12, 12,
      13, 13, 13, 13, 13,  14, 14, 14, 14, 14
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_ENERGY );
   return;
}

void spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   CHAR_DATA *gch;
   int heal_num, refresh_num;

   heal_num = skill_lookup("heal");
   refresh_num = skill_lookup("refresh");

   for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
   {
      if ((IS_NPC(ch) && IS_NPC(gch)) ||
      (!IS_NPC(ch) && !IS_NPC(gch)))
      {
         spell_heal(heal_num,level,ch,(void *) gch);
         spell_refresh(refresh_num,level,ch,(void *) gch);
      }
   }
}


void spell_mass_invis( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)vo;
   AFFECT_DATA af;
   CHAR_DATA *gch;

   for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
   {
      if ( !is_same_group( gch, ch ) || IS_AFFECTED(gch, AFF_INVISIBLE) )
         continue;
      act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM );
      send_to_char( "You slowly fade out of existence.\n\r", gch );
      af.type      = sn;
      af.level     = level/2;
      af.duration  = 24;
      af.location  = APPLY_NONE;
      af.modifier  = 0;
      af.bitvector = AFF_INVISIBLE;
      affect_to_char( gch, &af );
   }
   send_to_char( "Ok.\n\r", ch );

   return;
}



void spell_null( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;(void)vo;
   send_to_char( "That's not a spell!\n\r", ch );
   return;
}

void spell_octarine_fire( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_OCTARINE_FIRE) )
      return;
   af.type      = sn;
   af.level    = level;
   af.duration  = 2;
   af.location  = APPLY_AC;
   af.modifier  = 10 * level;
   af.bitvector = AFF_OCTARINE_FIRE;
   affect_to_char( victim, &af );
   send_to_char( "You are surrounded by an octarine outline.\n\r", victim );
   act( "$n is surrounded by a octarine outline.", victim, NULL, NULL, TO_ROOM );
   return;
}

void spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
   {
      if (victim == ch)
         send_to_char("You are already out of phase.\n\r",ch);
      else
         act("$N is already shifted out of phase.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = number_fuzzy( level / 4 );
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_PASS_DOOR;
   affect_to_char( victim, &af );
   act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM );
   send_to_char( "You turn translucent.\n\r", victim );
   return;
}

/* RT plague spell, very nasty */

void spell_plague( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if (saves_spell(level,victim) ||
   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
   {
      if (ch == victim)
         send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
      else
         act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
      return;
   }

   af.type      = sn;
   af.level     = level * 3/4;
   af.duration  = level;
   af.location  = APPLY_STR;
   af.modifier  = -5;
   af.bitvector = AFF_PLAGUE;
   affect_join(victim,&af);

   send_to_char
   ("You scream in agony as plague sores erupt from your skin.\n\r",victim);
   act("$n screams in agony as plague sores erupt from $s skin.",
   victim,NULL,NULL,TO_ROOM);
}

void spell_portal( int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)sn;(void)vo;
   CHAR_DATA *victim;
   OBJ_DATA *portal;
   char buf[256];

   if ( ( victim = get_char_world( ch, target_name ) ) == NULL
   ||   victim == ch
   ||   victim->in_room == NULL
   ||   !can_see_room(ch,victim->in_room)
   ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
   ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
   ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
   ||   IS_SET(victim->in_room->room_flags, ROOM_LAW)
   ||   victim->level >= level + 3
   ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
   ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
   ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON))
   ||   (IS_NPC(victim) && saves_spell( level, victim ) ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if (IS_SET(ch->in_room->room_flags, ROOM_LAW))
   {
      send_to_char ("You cannot portal from this room.\n\r", ch);
      return;
   }
   portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
   portal->timer = (ch->level/10);
   portal->destination = victim->in_room;

   snprintf( buf, sizeof(buf), portal->description, victim->in_room->name );

   /* Put portal in current room */
   free_string( portal->description );
   portal->description = str_dup( buf );
   obj_to_room(portal, ch->in_room);

   /* Create second portal */
   portal = create_object(get_obj_index(OBJ_VNUM_PORTAL), 0);
   portal->timer = (ch->level/10);
   portal->destination = ch->in_room;

   snprintf( buf, sizeof(buf), portal->description, ch->in_room->name );

   /* Put portal, in destination room, for this room */
   free_string( portal->description );
   portal->description = str_dup( buf );
   obj_to_room(portal, victim->in_room);

   send_to_char("You wave your hands madly, and create a portal.\n\r", ch);
   return;
}

void spell_poison( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( saves_spell( level, victim ) )
   {
      act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM);
      send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = level;
   af.location  = APPLY_STR;
   af.modifier  = -2;
   af.bitvector = AFF_POISON;
   affect_join( victim, &af );
   send_to_char( "You feel very sick.\n\r", victim );
   act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
   return;
}



void spell_protection_evil( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_PROTECTION_EVIL) )
   {
      if (victim == ch)
         send_to_char("You are already protected from evil.\n\r",ch);
      else
         act("$N is already protected from evil.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = 24;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_PROTECTION_EVIL;
   affect_to_char( victim, &af );
   send_to_char( "You feel protected from evil.\n\r", victim );
   if ( ch != victim )
      act("$N is protected from harm.",ch,NULL,victim,TO_CHAR);
   return;
}

void spell_protection_good( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_PROTECTION_GOOD) )
   {
      if (victim == ch)
         send_to_char("You are already protected from good.\n\r",ch);
      else
         act("$N is already protected from good.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = 24;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_PROTECTION_GOOD;
   affect_to_char( victim, &af );
   send_to_char( "You feel protected from good.\n\r", victim );
   if ( ch != victim )
      act("$N is protected from harm.",ch,NULL,victim,TO_CHAR);
   return;
}



void spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   victim->move = UMIN( victim->move + level, victim->max_move );
   if (victim->max_move == victim->move)
      send_to_char("You feel fully refreshed!\n\r",victim);
   else
      send_to_char( "You feel less tired.\n\r", victim );
   if ( ch != victim )
      send_to_char( "Ok.\n\r", ch );
   return;
}



void spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn; (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   bool found = FALSE;
   OBJ_DATA *obj;
   int iWear;

   if (check_dispel(level,victim,gsn_curse))
   {
      send_to_char("You feel better.\n\r",victim);
      act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM);
   }

   for ( iWear = 0; (iWear < MAX_WEAR && !found); iWear ++)
   {
      if ((obj = get_eq_char(victim,iWear)) == NULL)
         continue;

      if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
      {   /* attempt to remove curse */
         if (!saves_dispel(level,obj->level,0))
         {
            found = TRUE;
            REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
            REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
            act("$p glows blue.",victim,obj,NULL,TO_CHAR);
            act("$p glows blue.",victim,obj,NULL,TO_ROOM);
         }
      }
   }

   for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content)
   {
      if (IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
      {   /* attempt to remove curse */
         if (!saves_dispel(level,obj->level,0))
         {
            found = TRUE;
            REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
            REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
            act("Your $p glows blue.",victim,obj,NULL,TO_CHAR);
            act("$n's $p glows blue.",victim,obj,NULL,TO_ROOM);
         }
      }
   }
}

void spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
   {
      if (victim == ch)
         send_to_char("You are already in sanctuary.\n\r",ch);
      else
         act("$N is already in sanctuary.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = number_fuzzy( level / 6 );
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_SANCTUARY;
   affect_to_char( victim, &af );
   act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM );
   send_to_char( "You are surrounded by a white aura.\n\r", victim );
   return;
}

void spell_talon( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) )
   {
      if (victim == ch)
         send_to_char("Your hands are already as strong as talons.\n\r",ch);
      else
         act("$N already has talon like hands.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = number_fuzzy( level / 6 );
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_TALON;
   affect_to_char( victim, &af );
   act( "$n's hands become as strong as talons.", victim, NULL, NULL, TO_ROOM );
   send_to_char( "You hands become as strong as talons.\n\r", victim );
   return;
}

void spell_shield( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) )
   {
      if (victim == ch)
         send_to_char("You are already shielded from harm.\n\r",ch);
      else
         act("$N is already protected by a shield.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = 8 + level;
   af.location  = APPLY_AC;
   af.modifier  = -20;
   af.bitvector = 0;
   affect_to_char( victim, &af );
   act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM );
   send_to_char( "You are surrounded by a force shield.\n\r", victim );
   return;
}



void spell_shocking_grasp( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   static const int dam_each[] =
   {
      0,
      0,  0,  0,  0,  0,   0, 20, 25, 29, 33,
      36, 39, 39, 39, 40,  40, 41, 41, 42, 42,
      43, 43, 44, 44, 45,  45, 46, 46, 47, 47,
      48, 48, 49, 49, 50,  50, 51, 51, 52, 52,
      53, 53, 54, 54, 55,  55, 56, 56, 57, 57
   };
   int dam;

   level   = UMIN(level, (int)(sizeof(dam_each)/sizeof(dam_each[0]) - 1));
   level   = UMAX(0, level);
   dam     = number_range( dam_each[level] / 2, dam_each[level] * 2 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_LIGHTNING );
   return;
}



void spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( IS_AFFECTED(victim, AFF_SLEEP)
   ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD))
   ||   level < victim->level
   ||   saves_spell( level, victim ) )
      return;

   af.type      = sn;
   af.level     = level;
   af.duration  = 4 + level;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_SLEEP;
   affect_join( victim, &af );

   if ( IS_AWAKE(victim) )
   {
      send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
      act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
      victim->position = POS_SLEEPING;
   }

   return;
}



void spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( ch, sn ) )
   {
      if (victim == ch)
         send_to_char("Your skin is already as hard as a rock.\n\r",ch);
      else
         act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR);
      return;
   }
   af.type      = sn;
   af.level     = level;
   af.duration  = level;
   af.location  = APPLY_AC;
   af.modifier  = -40;
   af.bitvector = 0;
   affect_to_char( victim, &af );
   act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM );
   send_to_char( "Your skin turns to stone.\n\r", victim );
   return;
}

/*
 * improved by Faramir 7/8/96 because of various silly
 * summonings, like shopkeepers into Midgaard, thieves to the pit etc
 */

void spell_summon( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   CHAR_DATA *victim;

   if ( ( victim = get_char_world( ch, target_name ) ) == NULL
   ||   victim == ch
   ||   victim->in_room == NULL
   ||   ch->in_room == NULL
   ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
   ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
   ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
   ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
   ||   victim->level >= level + 3
   ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)
   ||   victim->fighting != NULL
   ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
   ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON))
   ||   (IS_NPC(victim) && saves_spell( level, victim ) )
   ||   (IS_SET(ch->in_room->room_flags, ROOM_SAFE ))    )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }
   if (  IS_SET(ch->in_room->room_flags, ROOM_LAW )) {
     send_to_char("You'd probably get locked behind bars for that!\n\r", ch);
     return;
   }
   if (IS_NPC(victim))  {
     if ( victim->pIndexData->pShop != NULL
	  || IS_SET(victim->act, ACT_IS_HEALER)
	  || IS_SET(victim->act, ACT_GAIN )
	  || IS_SET(victim->act, ACT_PRACTICE) ) {
       act("The guildspersons' convention prevents your summons.", ch, NULL, NULL, TO_CHAR);
       act("The guildspersons' convention protects $n from summons.", victim, NULL, NULL, TO_ROOM );
       act("$n attempted to summon you!", ch, NULL, victim, TO_VICT );
       return;
     }
   }


   if (victim->riding != NULL)
      unride_char (victim, victim->riding);
   act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
   char_from_room( victim );
   char_to_room( victim, ch->in_room );
   act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
   act( "$n has summoned you!", ch, NULL, victim,   TO_VICT );
   do_look( victim, "auto" );
   return;
}



void spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   ROOM_INDEX_DATA *pRoomIndex;

   if ( victim->in_room == NULL
   ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
   || ( !IS_NPC(ch) && victim->fighting != NULL )
   || ( victim != ch
   && ( saves_spell( level, victim ) || saves_spell( level, victim ) ) ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   for ( ; ; )
   {
      pRoomIndex = get_room_index( number_range( 0, 65535 ) );
      if ( pRoomIndex != NULL )
         if ( can_see_room(ch,pRoomIndex)
         &&   !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
         &&   !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) )
            break;
   }

   if (victim != ch)
      send_to_char("You have been teleported!\n\r",victim);

   act( "$n vanishes!", victim, NULL, NULL, TO_ROOM );
   char_from_room( victim );
   char_to_room( victim, pRoomIndex );
   if (!ch->riding) {
      act( "$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM );
   }
   else {
      act( "$n slowly fades into existence, about 5 feet off the ground!",
      victim,NULL,NULL, TO_ROOM );
      act( "$n falls to the ground with a thud!", victim, NULL, NULL, TO_ROOM);
      act( "You fall to the ground with a thud!", victim, NULL, NULL, TO_CHAR);
      fallen_off_mount(ch);
   } /* end ride check */
   do_look( victim, "auto" );
   return;
}



void spell_ventriloquate( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)vo;
   char buf1[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   char speaker[MAX_INPUT_LENGTH];
   CHAR_DATA *vch;

   target_name = one_argument( target_name, speaker );

   snprintf( buf1, sizeof(buf1), "%s says '%s'.\n\r",              speaker, target_name );
   snprintf( buf2, sizeof(buf2), "Someone makes %s say '%s'.\n\r", speaker, target_name );
   buf1[0] = UPPER(buf1[0]);

   for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
   {
      if ( !is_name( speaker, vch->name ) )
         send_to_char( saves_spell( level, vch ) ? buf2 : buf1, vch );
   }

   return;
}



void spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)ch;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   AFFECT_DATA af;

   if ( is_affected( victim, sn ) || saves_spell( level, victim ) )
      return;
   af.type      = sn;
   af.level     = level;
   af.duration  = level / 2;
   af.location  = APPLY_STR;
   af.modifier  = -1 * (level / 5);
   af.bitvector = AFF_WEAKEN;
   affect_to_char( victim, &af );
   send_to_char( "You feel weaker.\n\r", victim );
   act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
   return;
}



/* RT recall spell is back */

void spell_word_of_recall( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   ROOM_INDEX_DATA *location;

   if (IS_NPC(victim))
      return;

   if ((location = get_room_index( ROOM_VNUM_TEMPLE)) == NULL)
   {
      send_to_char("You are completely lost.\n\r",victim);
      return;
   }

   if (IS_SET(victim->in_room->room_flags,ROOM_NO_RECALL) ||
   IS_AFFECTED(victim,AFF_CURSE))
   {
      send_to_char("Spell failed.\n\r",victim);
      return;
   }

   if (victim->fighting != NULL)
      stop_fighting(victim,TRUE);
   if ( (victim->riding) && (victim->riding->fighting) )
      stop_fighting(victim->riding,TRUE);

   ch->move /= 2;
   if (victim->invis_level < HERO)
      act("$n disappears.",victim,NULL,NULL,TO_ROOM);
   if (victim->riding)
      char_from_room(victim->riding);
   char_from_room(victim);
   char_to_room(victim,location);
   if (victim->riding)
      char_to_room(victim->riding,location);
   if (victim->invis_level < HERO)
      act("$n appears in the room.",victim,NULL,NULL,TO_ROOM);
   do_look(victim,"auto");
   if (ch->pet != NULL)
      do_recall(ch->pet,"");
}

/*
 * NPC spells.
 */
void spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   OBJ_DATA *t_obj,*n_obj;
   int dam;
   int hpch;
   int i;

   if ( number_percent( ) < 2 * level && !saves_spell( level, victim )
   && ch->in_room->vnum != CHAL_ROOM )
   {
      for ( obj_lose = victim->carrying; obj_lose != NULL; obj_lose = obj_next )
      {
         int iWear;

         obj_next = obj_lose->next_content;

         if ( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
         case ITEM_ARMOR:
            if ( obj_lose->value[0] > 0 )
            {
               act( "$p is pitted and etched!",
               victim, obj_lose, NULL, TO_CHAR );
               if ( ( iWear = obj_lose->wear_loc ) != WEAR_NONE )
                  for (i = 0; i < 4; i ++)
                     victim->armor[i] -= apply_ac( obj_lose, iWear, i );
               for (i = 0; i < 4; i ++)
                  obj_lose->value[i] -= 1;
               obj_lose->cost      = 0;
               if ( iWear != WEAR_NONE )
                  for (i = 0; i < 4; i++)
                     victim->armor[i] += apply_ac( obj_lose, iWear, i );
            }
            break;

         case ITEM_CONTAINER:
            if (!IS_OBJ_STAT(obj_lose, ITEM_PROTECT_CONTAINER))
            {
               act( "$p fumes and dissolves, destroying some of the contents.",
               victim, obj_lose, NULL, TO_CHAR );
               /* save some of  the contents */

               for (t_obj = obj_lose->contains; t_obj != NULL; t_obj = n_obj)
               {
                  n_obj = t_obj->next_content;
                  obj_from_obj(t_obj);

                  if (number_bits(2) == 0 || victim->in_room == NULL)
                     extract_obj(t_obj);
                  else
                  obj_to_room(t_obj,victim->in_room);
               }
               extract_obj( obj_lose );
               break;
            }
            else
            {
               act("$p was protected from damage, saving the contents!",
               victim, obj_lose, NULL, TO_CHAR);
            }
         }
      }
   }

   hpch = UMAX( 10, ch->hit );
   if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && !IS_NPC (ch))
      hpch = 1000;
   dam  = number_range( hpch/20+1, hpch/10 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_ACID );
   return;
}



void spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
   /* Limit damage for PCs added by Rohan on all draconian*/
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   OBJ_DATA *t_obj, *n_obj;
   int dam;
   int hpch;

   if ( number_percent( ) < 2 * level && !saves_spell( level, victim )
   && ch->in_room->vnum != CHAL_ROOM )
   {
      for ( obj_lose = victim->carrying; obj_lose != NULL;
       obj_lose = obj_next )
      {
         char *msg;

         obj_next = obj_lose->next_content;
         if ( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
         default:
            continue;
         case ITEM_POTION:
            msg = "$p bubbles and boils!";
            break;
         case ITEM_SCROLL:
            msg = "$p crackles and burns!";
            break;
         case ITEM_STAFF:
            msg = "$p smokes and chars!";
            break;
         case ITEM_WAND:
            msg = "$p sparks and sputters!";
            break;
         case ITEM_FOOD:
            msg = "$p blackens and crisps!";
            break;
         case ITEM_PILL:
            msg = "$p melts and drips!";
            break;
         }

         act( msg, victim, obj_lose, NULL, TO_CHAR );
         if (obj_lose->item_type == ITEM_CONTAINER)
         {
            /* save some of  the contents */

            if (!IS_OBJ_STAT(obj_lose, ITEM_PROTECT_CONTAINER))
            {
               msg = "$p ignites and burns!";
               break;
               act( msg, victim, obj_lose, NULL, TO_CHAR );

               for (t_obj = obj_lose->contains; t_obj != NULL; t_obj = n_obj)
               {
                  n_obj = t_obj->next_content;
                  obj_from_obj(t_obj);

                  if (number_bits(2) == 0 || ch->in_room == NULL)
                     extract_obj(t_obj);
                  else
                  obj_to_room(t_obj,ch->in_room);
               }
            }
            else
            {
               act("$p was protected from damage, saving the contents!",
               victim, obj_lose, NULL, TO_CHAR);
            }
         }
         if (!IS_OBJ_STAT (obj_lose, ITEM_PROTECT_CONTAINER))
            extract_obj( obj_lose );
      }
   }

   hpch = UMAX( 10, ch->hit );
   if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && !IS_NPC (ch))
      hpch = 1000;
   dam  = number_range( hpch/20+1, hpch/10 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_FIRE );
   return;
}



void spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   OBJ_DATA *obj_lose;
   OBJ_DATA *obj_next;
   int dam;
   int hpch;

   if ( number_percent( ) < 2 * level && !saves_spell( level, victim )
   && ch->in_room->vnum != CHAL_ROOM )
   {
      for ( obj_lose = victim->carrying; obj_lose != NULL;
   obj_lose = obj_next )
      {
         char *msg;

         obj_next = obj_lose->next_content;
         if ( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
         default:
            continue;
         case ITEM_DRINK_CON:
         case ITEM_POTION:
            msg = "$p freezes and shatters!";
            break;
         }

         act( msg, victim, obj_lose, NULL, TO_CHAR );
         extract_obj( obj_lose );
      }
   }

   hpch = UMAX( 10, ch->hit );
   if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && !IS_NPC (ch))
      hpch = 1000;
   dam  = number_range( hpch/20+1, hpch/10 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_COLD );
   return;
}



void spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)vo;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   int dam;
   int hpch;

   for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
   {
      vch_next = vch->next_in_room;
      if ( !is_safe_spell(ch,vch,TRUE))
      {
         hpch = UMAX( 10, ch->hit );
         if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && !IS_NPC (ch))
            hpch = 1000;
         dam  = number_range( hpch/20+1, hpch/10 );
         if ( saves_spell( level, vch ) )
            dam /= 2;
         damage( ch, vch, dam, sn, DAM_POISON );
      }
   }
   return;
}



void spell_lightning_breath( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;
   int hpch;

   hpch = UMAX( 10, ch->hit );
   if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && !IS_NPC (ch))
      hpch = 1000;
   dam = number_range( hpch/20+1, hpch/10 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_LIGHTNING );
   return;
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;

   dam = number_range( 25, 100 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_PIERCE );
   return;
}

void spell_high_explosive( int sn, int level, CHAR_DATA *ch, void *vo )
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int dam;
   dam = number_range( 30, 120 );
   if ( saves_spell( level, victim ) )
      dam /= 2;
   damage( ch, victim, dam, sn, DAM_PIERCE );
   return;
}

void explode_bomb( OBJ_DATA *bomb, CHAR_DATA *ch, CHAR_DATA *thrower)
{
	int chance;
	int sn, position;
	void *vo = (void *) ch;
	
	chance = URANGE( 5, (50 + ( ( bomb->value[0] - ch->level ) * 8 ) ) , 100 );
	if (number_percent() > chance ) {
		act( "$p emits a loud bang and disappears in a cloud of smoke.", ch, bomb, NULL, TO_ROOM );
		act( "$p emits a loud bang, but thankfully does not affect you.", ch, bomb, NULL, TO_CHAR );
		extract_obj( bomb );
		return;
	}
	
	for ( position = 1; ( (position <= 4) && (bomb->value[position] < MAX_SKILL) && 
		(bomb->value[position] != -1)); position++ ) {
		sn = bomb->value[position];
		if (skill_table[sn].target == TAR_CHAR_OFFENSIVE)
			(*skill_table[sn].spell_fun) ( sn, bomb->value[0], thrower, vo );
	}
	extract_obj( bomb);
}

void spell_teleport_object( int sn, int level, CHAR_DATA *ch, void *vo)
{
  (void)sn;(void)level;(void)vo;
   CHAR_DATA *victim;
   OBJ_DATA *object;
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];
   ROOM_INDEX_DATA *old_room;

   target_name = one_argument(target_name,arg1);
   one_argument(target_name,arg2);

   if (arg1[0] == '\0') {
      send_to_char( "Teleport what and to whom?\n\r", ch);
      return;
   }

   if ( (object = get_obj_carry( ch, arg1)) == NULL) {
      send_to_char( "You're not carrying that.\n\r", ch);
      return;
   }

   if ( arg2[0] == '\0') {
      send_to_char( "Teleport to whom?\n\r", ch);
      return;
   }

   if ( (victim = get_char_world( ch, arg2 )) == NULL ) {
      send_to_char( "Teleport to whom?\n\r", ch);
      return;
   }

   if (victim == ch)
   {
      send_to_char ("|CYou decide that you want to show off and you teleport an object\n\r|Cup to the highest heavens and straight back into your inventory!\n\r|w", ch);
      act ("|C$n decides that $e wants to show off and $e teleports an object\n\r|Cup to the highest heavens and straight back into $s own inventory!|w", ch, NULL, NULL, TO_ROOM);
      return;
   }

   if ( !can_see(ch, victim) ) {
      send_to_char( "Teleport to whom?\n\r", ch);
      return;
   }

   if (IS_SET (ch->in_room->room_flags, ROOM_NO_RECALL))
   {
      send_to_char ("You failed.\n\r", ch);
      return;
   }

   if (IS_SET (victim->in_room->room_flags, ROOM_NO_RECALL))
   {
      send_to_char ("You failed.\n\r", ch);
      return;
   }

   /* Check to see if the victim is actually already in the same room */
   if (ch->in_room != victim->in_room) {
      act("You feel a brief presence in the room.", victim,NULL, NULL,TO_CHAR);
      act("You feel a brief presence in the room.", victim,NULL, NULL,TO_ROOM);
      snprintf( buf, sizeof(buf), "'%s' %s", object->name, victim->name);
      old_room = ch->in_room;
      char_from_room( ch );
      char_to_room( ch, victim->in_room );
      do_give( ch, buf );
      char_from_room( ch );
      char_to_room( ch, old_room );
   }
   else {
      snprintf( buf, sizeof(buf), "'%s' %s", object->name, victim->name);
      do_give( ch, buf );
   } /* ..else... if not in same room */
   return;
}

void spell_undo_spell( int sn, int level, CHAR_DATA *ch, void *vo )
{
  (void)sn;(void)level;(void)vo;
   CHAR_DATA *victim;
   int spell_undo;
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];

   target_name = one_argument(target_name,arg1);
   one_argument(target_name,arg2);

   if (arg2[0] == '\0') {
      victim = ch;
   }
   else {
      if ( (victim = get_char_room(ch, arg2)) == NULL) {
         send_to_char( "They're not here.\n\r",ch );
         return;
      }
   }

   if (arg1[0] == '\0') {
      send_to_char( "Undo which spell?\n\r",ch );
      return;
   }

   if ((spell_undo = skill_lookup(arg1)) < 0) {
      send_to_char( "What kind of spell is that?\n\r", ch);
      return;
   }

   if (!is_affected(victim,spell_undo)) {
      if ( victim == ch) {
         send_to_char( "You're not affected by that.\n\r", ch );
      }
      else {
         act( "$N doesn't seem to be affected by that.",
         ch, NULL, victim, TO_CHAR);
      }
      return;
   }

   if (check_dispel(ch->level,victim,spell_undo)) {
      if (!str_cmp(skill_table[spell_undo].name, "blindness"))
	 act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "calm"))
	 act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "change sex"))
	 act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "charm person"))
	 act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "chill touch"))
	 act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "faerie fire"))
	 act("$n's outline fades.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "octarine fire"))
	 act("$n's octarine outline fades.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "fly"))
	 act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "frenzy"))
	 act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "giant strength"))
	 act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "haste"))
	 act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "talon"))
	 act("$n hand's return to normal.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "lethargy"))
	 act("$n is looking more lively.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "insanity"))
	 act("$n looks less confused.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "invis"))
	 act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "mass invis"))
	 act("$n fades into existance.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "sanctuary"))
	 act("The white aura around $n's body vanishes.",
	     victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "shield"))
	 act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "stone skin"))
	 act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
      if (!str_cmp(skill_table[spell_undo].name, "weaken"))
	 act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);

      send_to_char( "Ok.\n\r", ch);
   }
   else {
      send_to_char( "Spell failed.\n\r", ch);
   }

   return;
}
