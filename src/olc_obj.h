/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_obj.h:  Faramir's OLC object editor                              */
/*                                                                       */
/*************************************************************************/



/* Okay - the defined functions we have are ... */

DECLARE_DO_FUN( do_object );
DECLARE_DO_FUN( do_clipboard );

#define GET_HOLD      16385
#define GET_WIELD      8193
#define GET_BODY          9


/* INFO display flags */
#define OLC_V_COST        1 
#define OLC_V_WEIGHT      2
#define OLC_V_APPLY       3
#define OLC_V_FLAGS       4
#define OLC_V_MAGICAL     5
#define OLC_V_STRING      6
#define OLC_V_WEAPON      7
#define OLC_V_WEAR        8
#define OLC_V_MATERIAL    9

extern bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj);
extern void find_limits (AREA_DATA *area, int *lower, int *higher);
extern int create_new_room (int lower,int higher);
extern int _delete_obj( int vnum );

