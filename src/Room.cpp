/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "common/BitOps.hpp"

bool Room::is_outside() const { return !is_inside(); }

bool Room::is_inside() const { return check_enum_bit(room_flags, RoomFlag::Indoors); }
