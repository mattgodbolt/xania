#pragma once

#include "Types.hpp"

#include <optional>
#include <string_view>

// Standard types of nutrition that player characters have a condition for (inebriation, satiation, hydration).
// There's no character specific data in this.
class Nutrition {
public:
    Nutrition(std::string_view nil_message, std::string_view unhealthy_adjective, const sh_int satisfaction_threshold,
              std::string_view satisfaction_message, const bool positive_scale);

    [[nodiscard]] sh_int max_condition() const noexcept;
    [[nodiscard]] std::string_view nil_message() const noexcept;
    [[nodiscard]] std::string_view unhealthy_adjective() const noexcept;
    [[nodiscard]] sh_int satisfaction_threshold() const noexcept;
    [[nodiscard]] std::string_view satisfaction_message() const noexcept;
    [[nodiscard]] bool positive_scale() const noexcept;

    enum LiquidAffect { Inebriation = 0, Satiation = 1, Hydration = 2 };

private:
    // For the time being all nutrition types use the same upper limit.
    static constexpr auto MaxCondition = 48;
    std::string_view nil_message_;
    std::string_view unhealthy_adjective_;
    const sh_int satisfaction_threshold_;
    std::string_view satisfaction_message_;
    const bool positive_scale_;
};

// Associates a player character's mutable nutrition rating with an immutable nutrition type.
class PcNutrition {
public:
    PcNutrition(const Nutrition &nutrition, const sh_int condition);

    [[nodiscard]] sh_int get() const noexcept;
    // Apply a delta to the current value with bounds checking.
    // Returns the message for the player following a change or empty if none.
    [[nodiscard]] std::optional<std::string_view> apply_delta(const sh_int delta) noexcept;
    // Set the value, with bounds checking (used when loading from player files, and "set mob [drunk|full|thirst] ...")
    void set(const sh_int condition) noexcept;
    // True if the condition has crossed the satisfaction threshold.
    [[nodiscard]] bool is_satisfied() const noexcept;
    // True if the condition has crossed the unhealthy threshold.
    [[nodiscard]] bool is_unhealthy() const noexcept;
    [[nodiscard]] std::string_view unhealthy_adjective() const noexcept;

    // For initializing the condition when PcData is created.
    static PcNutrition sober();
    static PcNutrition fed();
    static PcNutrition hydrated();

private:
    const Nutrition &nutrition_;
    sh_int condition_;
};