/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectIndex.hpp"
#include "BitsObjectExtra.hpp"
#include "ObjectType.hpp"
#include "ObjectWearFlag.hpp"
#include "common/BitOps.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

std::string ObjectIndex::type_name() const { return lower_case(magic_enum::enum_name<ObjectType>(type)); }

bool ObjectIndex::is_no_remove() const { return check_bit(extra_flags, ITEM_NOREMOVE); }
