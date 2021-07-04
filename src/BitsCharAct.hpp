#pragma once

#include "common/StandardBits.hpp"
/*
 * Character act bits.
 * Used in #MOBILES section of area files.
 * TODO make an actual class to hold bits like this.
 */
#define ACT_IS_NPC (A) /* Auto set for mobs    */
#define ACT_SENTINEL (B) /* Stays in one room    */
#define ACT_SCAVENGER (C) /* Picks up objects     */
#define ACT_AGGRESSIVE (F) /* Attacks PC's         */
#define ACT_STAY_AREA (G) /* Won't leave area     */
#define ACT_WIMPY (H)
#define ACT_PET (I) /* Auto set for pets    */
#define ACT_TRAIN (J) /* Can train PC's       */
#define ACT_PRACTICE (K) /* Can practice PC's    */
#define ACT_SENTIENT (L) /* Is intelligent       */
#define ACT_TALKATIVE (M) /* Will respond to says and emotes */
#define ACT_UNDEAD (O)
#define ACT_CLERIC (Q)
#define ACT_MAGE (R)
#define ACT_THIEF (S)
#define ACT_WARRIOR (T)
#define ACT_NOALIGN (U)
#define ACT_NOPURGE (V)
#define ACT_IS_HEALER (aa)
#define ACT_GAIN (bb)
#define ACT_UPDATE_ALWAYS (cc)
#define ACT_CAN_BE_RIDDEN (dd) /* Can be ridden */
