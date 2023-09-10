/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectIndex.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectType.hpp"
#include "common/BitOps.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>
#include <memory>

std::string ObjectIndex::type_name() const { return lower_case(magic_enum::enum_name<ObjectType>(type)); }

bool ObjectIndex::is_no_remove() const { return check_enum_bit(extra_flags, ObjectExtraFlag::NoRemove); }

std::unique_ptr<Object> ObjectIndex::create_object() { return std::make_unique<Object>(this); }
