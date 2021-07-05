/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

std::string ObjectIndex::type_name() const { return lower_case(magic_enum::enum_name<ObjectType>(type)); }
