#pragma once

#include "CombatTypes.hpp"

struct Char;
class Rng;

class ImpactVerbs {
public:
    // Creates a new ImpactVerbs structure on the stack.
    [[nodiscard]] static ImpactVerbs create(const bool has_attack_verb, const int dam_proportion, const int dam_type);

    [[nodiscard]] std::string_view singular() const;
    [[nodiscard]] std::string_view plural() const;

private:
    ImpactVerbs(const std::string singular, const std::string plural);
    const std::string singular_;
    const std::string plural_;
};

class DamageMessages {
public:
    // Creates a new DamageMessages on the stack.
    [[nodiscard]] static DamageMessages create(const Char *ch, const Char *victim, const DamageContext &context,
                                               Rng &rng);

    [[nodiscard]] std::string_view to_char() const;
    [[nodiscard]] std::string_view to_victim() const;
    [[nodiscard]] std::string_view to_room() const;

private:
    DamageMessages(const std::string to_char, const std::string to_victim, const std::string to_room);

    const std::string to_char_;
    const std::string to_victim_;
    const std::string to_room_;
};