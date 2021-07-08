/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Room.hpp"
#include "BitsRoomState.hpp"
#include "common/BitOps.hpp"

bool Room::is_outside() const { return !is_inside(); }

bool Room::is_inside() const { return check_bit(room_flags, ROOM_INDOORS); }
