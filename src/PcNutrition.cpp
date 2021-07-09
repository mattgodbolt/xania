#include "PcNutrition.hpp"

#include <algorithm>

namespace {

// Inebriation has historically been modeled the opposite way to hunger and thirst.
Nutrition inebriation{"You are sober.", "drunk", 10, "You feel drunk.", false};
Nutrition hunger{"You are hungry.", "hungry", 40, "You feel well nourished.", true};
Nutrition thirst{"You are thirsty.", "thirsty", 40, "Your thirst is quenched.", true};

}

Nutrition::Nutrition(std::string_view nil_message, std::string_view unhealthy_adjective,
                     const sh_int satisfaction_threshold, std::string_view satisfaction_message, bool positive_scale)
    : nil_message_(nil_message), unhealthy_adjective_(unhealthy_adjective),
      satisfaction_threshold_(satisfaction_threshold), satisfaction_message_(satisfaction_message),
      positive_scale_(positive_scale) {}

sh_int Nutrition::max_condition() const noexcept { return MaxCondition; }

std::string_view Nutrition::nil_message() const noexcept { return nil_message_; }

std::string_view Nutrition::unhealthy_adjective() const noexcept { return unhealthy_adjective_; }

sh_int Nutrition::satisfaction_threshold() const noexcept { return satisfaction_threshold_; }

std::string_view Nutrition::satisfaction_message() const noexcept { return satisfaction_message_; }

bool Nutrition::positive_scale() const noexcept { return positive_scale_; }

PcNutrition::PcNutrition(const Nutrition &nutrition, const sh_int condition)
    : nutrition_(nutrition), condition_(condition) {}

sh_int PcNutrition::get() const noexcept { return condition_; }

std::optional<std::string_view> PcNutrition::apply_delta(const sh_int delta) noexcept {
    condition_ = std::clamp(static_cast<sh_int>(condition_ + delta), 0_s, nutrition_.max_condition());
    if (condition_ == 0) {
        return nutrition_.nil_message();
    } else if (condition_ > nutrition_.satisfaction_threshold()) {
        return nutrition_.satisfaction_message();
    } else
        return std::nullopt;
}

void PcNutrition::set(const sh_int condition) noexcept {
    condition_ = std::clamp(static_cast<sh_int>(condition), 0_s, nutrition_.max_condition());
}

// Note that for negative scale nutrition like inebriation a Char
// is considered satisfied after a tipple. That's why any value over the threshold is suitable.
bool PcNutrition::is_satisfied() const noexcept { return condition_ > nutrition_.satisfaction_threshold(); }

bool PcNutrition::is_unhealthy() const noexcept {
    if (nutrition_.positive_scale()) {
        return condition_ == 0;
    } else {
        return condition_ > nutrition_.satisfaction_threshold();
    }
}

std::string_view PcNutrition::unhealthy_adjective() const noexcept { return nutrition_.unhealthy_adjective(); }

PcNutrition PcNutrition::sober() { return {inebriation, 0}; }

PcNutrition PcNutrition::fed() { return {hunger, hunger.satisfaction_threshold()}; }

PcNutrition PcNutrition::hydrated() { return {thirst, thirst.satisfaction_threshold()}; }
