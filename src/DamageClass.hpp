/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

// Damage classes.
// TODO: Convert to enum?
#define DAM_NONE 0
#define DAM_BASH 1
#define DAM_PIERCE 2
#define DAM_SLASH 3
#define DAM_FIRE 4
#define DAM_COLD 5
#define DAM_LIGHTNING 6
#define DAM_ACID 7
#define DAM_POISON 8
#define DAM_NEGATIVE 9
#define DAM_HOLY 10
#define DAM_ENERGY 11
#define DAM_MENTAL 12
#define DAM_DISEASE 13
#define DAM_DROWNING 14
#define DAM_LIGHT 15
#define DAM_OTHER 16
#define DAM_HARM 17
#define MAX_DAM 18 /* should == number of dam types */
/* DON'T FORGET! to add to the the dam_string_table if you add
   a new type of damage here */
