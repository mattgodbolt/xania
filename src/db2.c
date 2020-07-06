/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db2.c: extra database utility functions                              */
/*                                                                       */
/*************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include <time.h>
#include "sys/types.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif

#include "merc.h"
#include "db.h"
#include "lookup.h"

void   		mprog_read_programs     args( ( FILE* fp,
					MOB_INDEX_DATA *pMobIndex ) );
void assign_area_vnum args(( int vnum )); /* OLC */

/* values for db2.c */
struct 					social_type	social_table		[MAX_SOCIALS];
int						social_count		= 0;

/* snarf a socials file */
void load_socials( FILE *fp)
{
   for ( ; ; )
   {
      struct social_type social;
      char *temp;
      /* clear social */
      social.char_no_arg = NULL;
      social.others_no_arg = NULL;
      social.char_found = NULL;
      social.others_found = NULL;
      social.vict_found = NULL;
      social.char_not_found = NULL;
      social.char_auto = NULL;
      social.others_auto = NULL;

      temp = fread_word(fp);
      if (!strcmp(temp,"#0"))
         return;  /* done */

      strcpy(social.name,temp);
      fread_to_eol(fp);

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.char_no_arg = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.char_no_arg = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.others_no_arg = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.others_no_arg = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.char_found = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.char_found = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.others_found = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.others_found = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.vict_found = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.vict_found = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.char_not_found = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.char_not_found = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.char_auto = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.char_auto = temp;

      temp = fread_string_eol(fp);
      if (!strcmp(temp,"$"))
         social.others_auto = NULL;
      else if (!strcmp(temp,"#"))
      {
         social_table[social_count] = social;
         social_count++;
         continue;
      }
      else
      social.others_auto = temp;

      social_table[social_count] = social;
      social_count++;
   }
   return;
}






/*
 * Snarf a mob section.  new style
 */
void load_mobiles( FILE *fp )
{
	MOB_INDEX_DATA *pMobIndex;

	if( !area_last ) {     /* OLC */
		bug( "Load_mobiles: no #AREA seen yet!", 0 );
		exit( 1 );
	}
	for ( ; ; )
		{
			sh_int vnum;
			char letter;
			int iHash;

			letter                          = fread_letter( fp );
			if ( letter != '#' )
				{
					bug( "Load_mobiles: # not found.", 0 );
					exit( 1 );
				}

			vnum                            = fread_number( fp );
			if ( vnum == 0 )
				break;

			fBootDb = FALSE;
			if ( get_mob_index( vnum ) != NULL )
				{
					bug( "Load_mobiles: vnum %d duplicated.", vnum );
					exit( 1 );
				}
			fBootDb = TRUE;

			pMobIndex                       = alloc_perm( sizeof(*pMobIndex) );
			pMobIndex->vnum                 = vnum;
			pMobIndex->area                 = area_last;  /* OLC */
			pMobIndex->new_format		= TRUE;
			newmobs++;
			pMobIndex->player_name          = fread_string( fp );
			pMobIndex->short_descr          = fread_string( fp );
			// Kill off errant capitals - see load_object
			DeCapitate (pMobIndex->short_descr);
			pMobIndex->long_descr           = fread_string( fp );
			pMobIndex->description          = fread_string( fp );
			pMobIndex->race		 	= race_lookup(fread_string( fp ));

			pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
			pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);

			pMobIndex->act                  = fread_flag( fp ) | ACT_IS_NPC
				| race_table[pMobIndex->race].act;
			pMobIndex->affected_by          = fread_flag( fp )
				| race_table[pMobIndex->race].aff;
			pMobIndex->pShop                = NULL;
			pMobIndex->alignment            = fread_number( fp );
			
			pMobIndex->group = fread_number( fp );

			pMobIndex->level                = fread_number( fp );
			pMobIndex->hitroll              = fread_number( fp );

			/* read hit dice */
			pMobIndex->hit[DICE_NUMBER]     = fread_number( fp );
			/* 'd'          */                fread_letter( fp );
			pMobIndex->hit[DICE_TYPE]   	= fread_number( fp );
			/* '+'          */                fread_letter( fp );
			pMobIndex->hit[DICE_BONUS]      = fread_number( fp );

			/* read mana dice */
			pMobIndex->mana[DICE_NUMBER]	= fread_number( fp );
			fread_letter( fp );
			pMobIndex->mana[DICE_TYPE]	= fread_number( fp );
			fread_letter( fp );
			pMobIndex->mana[DICE_BONUS]	= fread_number( fp );

			/* read damage dice */
			pMobIndex->damage[DICE_NUMBER]	= fread_number( fp );
			fread_letter( fp );
			pMobIndex->damage[DICE_TYPE]	= fread_number( fp );
			fread_letter( fp );
			pMobIndex->damage[DICE_BONUS]	= fread_number( fp );
			pMobIndex->dam_type		= attack_lookup (fread_word(fp));

			/* read armor class */
			pMobIndex->ac[AC_PIERCE]	= fread_number( fp ) * 10;
			pMobIndex->ac[AC_BASH]		= fread_number( fp ) * 10;
			pMobIndex->ac[AC_SLASH]		= fread_number( fp ) * 10;
			pMobIndex->ac[AC_EXOTIC]	= fread_number( fp ) * 10;

			/* read flags and add in data from the race table */
			pMobIndex->off_flags		= fread_flag( fp )
				| race_table[pMobIndex->race].off;
			pMobIndex->imm_flags		= fread_flag( fp )
				| race_table[pMobIndex->race].imm;
			pMobIndex->res_flags		= fread_flag( fp )
				| race_table[pMobIndex->race].res;
			pMobIndex->vuln_flags		= fread_flag( fp )
				| race_table[pMobIndex->race].vuln;

			/* vital statistics */
			pMobIndex->start_pos		= position_lookup(fread_word(fp));
			pMobIndex->default_pos	      	= position_lookup(fread_word(fp));
			pMobIndex->sex			= sex_lookup( fread_word(fp));
			pMobIndex->gold			= fread_number( fp );

			pMobIndex->form			= fread_flag( fp ) 
				| race_table[pMobIndex->race].form;
			pMobIndex->parts		= fread_flag( fp )
				| race_table[pMobIndex->race].parts;
			/* size */
			pMobIndex->size = size_lookup( fread_word(fp));
			pMobIndex->material		= material_lookup(fread_word( fp ));

			for ( ; ; ) {
				letter = fread_letter( fp );

				if (letter == 'F') {
					char *word;
					long vector;

					word                    = fread_word(fp);
					vector			= fread_flag(fp);

					if (!str_prefix(word,"act")) {
						REMOVE_BIT(pMobIndex->act,vector);
					}
					else if (!str_prefix(word,"aff")) {
						REMOVE_BIT(pMobIndex->affected_by,vector);

					}
					else if (!str_prefix(word,"off")) {
						REMOVE_BIT(pMobIndex->off_flags,vector);
					}
					else if (!str_prefix(word,"imm")) {
						REMOVE_BIT(pMobIndex->imm_flags,vector);
					}
					else if (!str_prefix(word,"res")) {
						REMOVE_BIT(pMobIndex->res_flags,vector);
					}
					else if (!str_prefix(word,"vul")) {
						REMOVE_BIT(pMobIndex->vuln_flags,vector);
					}
					else if (!str_prefix(word,"for")) {
						REMOVE_BIT(pMobIndex->form,vector);
					}
					else if (!str_prefix(word,"par"))	{

						REMOVE_BIT(pMobIndex->parts,vector);
					}
					else {					      
						bug("Flag remove: flag not found.",0);
						exit(1);
					}
				}
				else {
					{
						ungetc(letter,fp);
						break;
					}
				}
		      
		      
			
				if ( letter != 'S' )
					{
						bug( "Load_mobiles: vnum %d non-S.", vnum );
						exit( 1 );
					}
			}
				
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
			letter=fread_letter(fp);
			if (letter=='>') {
				ungetc(letter,fp);
				mprog_read_programs(fp,pMobIndex);
			}
			else ungetc(letter,fp);

			iHash                   = vnum % MAX_KEY_HASH;
			pMobIndex->next         = mob_index_hash[iHash];
			mob_index_hash[iHash]   = pMobIndex;
			top_mob_index++;
			top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;  /* OLC */
			assign_area_vnum( vnum );				   /* OLC */
			kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
		}

	return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects( FILE *fp )
{
	OBJ_INDEX_DATA *pObjIndex;
	char temp;  /* Used for Death's Wear Strings bit */
   
	if( !area_last) {
		bug( "Load_objects: no #AREA section found yet!", 0 );
		exit( 1 );
	}

	for ( ; ; )
		{
			sh_int vnum;
			char letter;
			int iHash;

			letter                          = fread_letter( fp );
			if ( letter != '#' )
				{
					bug( "Load_objects: # not found.", 0 );
					exit( 1 );
				}

			vnum                            = fread_number( fp );
			if ( vnum == 0 )
				break;

			fBootDb = FALSE;
			if ( get_obj_index( vnum ) != NULL )
				{
					bug( "Load_objects: vnum %d duplicated.", vnum );
					exit( 1 );
				}
			fBootDb = TRUE;

			pObjIndex                       = alloc_perm( sizeof(*pObjIndex) );
			pObjIndex->vnum                 = vnum;
			pObjIndex->area                 = area_last;  /* OLC */
			pObjIndex->new_format           = TRUE;
			pObjIndex->reset_num		= 0;
			newobjs++;
			pObjIndex->name                 = fread_string( fp );
			pObjIndex->short_descr          = fread_string( fp );
			/*
			 * MG added - snarf short descrips to kill:
			 * You hit The beastly fido
			 */
			DeCapitate (pObjIndex->short_descr);
			pObjIndex->description          = fread_string( fp );
			if( strlen( pObjIndex->description) == 0 ) {
				bug( "Load_objects: empty long description in object %d.", vnum );
			}
			pObjIndex->material		= material_lookup(fread_string( fp ));

			pObjIndex->item_type = item_lookup(fread_word(fp));

			pObjIndex->extra_flags          = fread_flag( fp );
			pObjIndex->wear_flags           = fread_flag( fp );

			temp = fread_letter( fp );
			if (temp == ',') {
				pObjIndex->wear_string = fread_string( fp );
			}
			else {
				pObjIndex->wear_string = NULL;
				ungetc(temp, fp);
			}

			switch(pObjIndex->item_type) {
			case ITEM_WEAPON:
				pObjIndex->value[0]		= weapon_type(fread_word(fp));
				pObjIndex->value[1]		= fread_number(fp);
				pObjIndex->value[2]		= fread_number(fp);
				pObjIndex->value[3]		= attack_lookup(fread_word(fp));
				pObjIndex->value[4]		= fread_flag(fp);
				break;
			case ITEM_CONTAINER:
				pObjIndex->value[0]		= fread_number(fp);
				pObjIndex->value[1]		= fread_flag(fp);
				pObjIndex->value[2]		= fread_number(fp);
				pObjIndex->value[3]		= fread_number(fp);
				pObjIndex->value[4]		= fread_number(fp);
				break;
			case ITEM_DRINK_CON:
			case ITEM_FOUNTAIN:
				pObjIndex->value[0]         = fread_number(fp);
				pObjIndex->value[1]         = fread_number(fp);
				pObjIndex->value[2]         = liq_lookup(fread_word(fp));
				pObjIndex->value[3]         = fread_number(fp);
				pObjIndex->value[4]         = fread_number(fp);
				break;
			case ITEM_WAND:
			case ITEM_STAFF:
				pObjIndex->value[0]		= fread_number(fp);
				pObjIndex->value[1]		= fread_number(fp);
				pObjIndex->value[2]		= fread_number(fp);
				pObjIndex->value[3]		= fread_spnumber(fp);
				pObjIndex->value[4]		= fread_number(fp);
				break;
			case ITEM_POTION:
			case ITEM_PILL:
			case ITEM_SCROLL:
			case ITEM_BOMB:
				pObjIndex->value[0]		= fread_number(fp);
				pObjIndex->value[1]		= fread_spnumber(fp);
				pObjIndex->value[2]		= fread_spnumber(fp);
				pObjIndex->value[3]		= fread_spnumber(fp);
				pObjIndex->value[4]		= fread_spnumber(fp);
				break;
			default:
				pObjIndex->value[0]             = fread_flag( fp );
				pObjIndex->value[1]             = fread_flag( fp );
				pObjIndex->value[2]             = fread_flag( fp );
				pObjIndex->value[3]             = fread_flag( fp );
				pObjIndex->value[4]		    = fread_flag( fp );
				break;
			}
	
			pObjIndex->level		= fread_number( fp );
			pObjIndex->weight               = fread_number( fp );
			pObjIndex->cost                 = fread_number( fp );

				/* condition */
			letter 				= fread_letter( fp );
			switch (letter)
				{
				case ('P') :
					pObjIndex->condition = 100;
					break;
				case ('G') :
					pObjIndex->condition =  90;
					break;
				case ('A') :
					pObjIndex->condition =  75;
					break;
				case ('W') :
					pObjIndex->condition =  50;
					break;
				case ('D') :
					pObjIndex->condition =  25;
					break;
				case ('B') :
					pObjIndex->condition =  10;
					break;
				case ('R') :
					pObjIndex->condition =   0;
					break;
				default:
					pObjIndex->condition = 100;
					break;
				}

			for ( ; ; ) {
				char letter;
				
				letter = fread_letter( fp );
				
				if ( letter == 'A' )
					{
						AFFECT_DATA *paf;
						
						paf                     = alloc_perm( sizeof(*paf) );
						paf->type               = -1;
						paf->level              = pObjIndex->level;
						paf->duration           = -1;
						paf->location           = fread_number( fp );
						paf->modifier           = fread_number( fp );
						paf->bitvector          = 0;
						paf->next               = pObjIndex->affected;
						pObjIndex->affected     = paf;
						top_affect++;
					}
				
				else if ( letter == 'E' ) {
					EXTRA_DESCR_DATA *ed;
					
					ed                      = alloc_perm( sizeof(*ed) );
					ed->keyword             = fread_string( fp );
					ed->description         = fread_string( fp );
					ed->next                = pObjIndex->extra_descr;
					pObjIndex->extra_descr  = ed;
					top_ed++;
				}
				
				else {
					ungetc( letter, fp );
					break;
				}
			}
			
			
			/*
			 * Translate spell "slot numbers" to internal "skill numbers."
			 */
			switch ( pObjIndex->item_type )
				{
				case ITEM_BOMB:
					pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
                                        // fall through
				case ITEM_PILL:
				case ITEM_POTION:
				case ITEM_SCROLL:
					pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
					pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
					pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
					break;

				case ITEM_STAFF:
				case ITEM_WAND:
					pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
					break;
				}
      

			iHash                   = vnum % MAX_KEY_HASH;
			pObjIndex->next         = obj_index_hash[iHash];
			obj_index_hash[iHash]   = pObjIndex;
			top_obj_index++;
			top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;  /* OLC */
			assign_area_vnum( vnum );				   /* OLC */
		}

	return;
}

char *print_flags(const int value) {
  static char buf[36];
  char *b=buf;
  int bit;
  static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef";
  buf[0]='\0';
  if (value==0) {
    sprintf( buf, "0");
    return (char *)&buf;
  }

  for ( bit = 0; bit < 32; bit++ ) {
    if ((value & (1<<bit)) !=0)
      *b++ = table[bit];
  }
  *b = '\0';
  return (char *)&buf;
}

