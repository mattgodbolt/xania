/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db.h: file in/out, area database handler                             */
/*                                                                       */
/*************************************************************************/

/* files used in db.c */

/* vals from db.c */
extern bool fBootDb;
extern int newmobs;
extern int newobjs;
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern int top_mob_index;
extern int top_obj_index;
extern int top_affect;
extern int top_ed;

extern int top_vnum_room; /* OLC */
extern int top_vnum_mob; /* OLC */
extern int top_vnum_obj; /* OLC */
extern AREA_DATA *area_first;
extern AREA_DATA *area_last;

/* from db2.c */

extern int social_count;
