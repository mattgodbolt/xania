/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include <string_view>
#include <variant>

struct Char;
struct Object;

// Used when casting a spell, this wraps std::variant to carry through the target of a spell through
// to the spell function being invoked. Most spells either target a Character or an Object. But a select few
// use custom handling, in which case the spell function will do its own processing
// of the "arguments" string_view.
// It's possible for someone to pass the wrong SpellTarget variant to a spell function, and it's up
// to the spell function to handle this to avoid a runtime error.
class SpellTarget {
public:
    explicit SpellTarget(Char *ch) : target_(ch) {}
    explicit SpellTarget(Object *obj) : target_(obj) {}
    explicit SpellTarget(std::string_view arguments) : target_(arguments) {}
    [[nodiscard]] Char *getChar() const {
        if (auto ptr = std::get_if<Char *>(&target_)) {
            return *ptr;
        } else {
            return nullptr;
        }
    }
    [[nodiscard]] Object *getObject() const {
        if (auto ptr = std::get_if<Object *>(&target_)) {
            return *ptr;
        } else {
            return nullptr;
        }
    }
    [[nodiscard]] std::string_view getArguments() const {
        if (auto ptr = std::get_if<std::string_view>(&target_)) {
            return *ptr;
        } else {
            return "";
        }
    }

private:
    const std::variant<std::string_view, Object *, Char *> target_;
};

// Function signature for all spells.
using SpellFunc = void (*)(int spell_num, int level, Char *ch, const SpellTarget &spell_target);
