/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

// Mob progs can be triggered when an act() event occurs, e.g. where a message is
// broadcast to a room when a player drops an object. This enables
// triggers to fire conditionally depending on the context.
enum class MobTrig { No, Yes };
