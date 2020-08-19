#pragma once

#include "Descriptor.hpp"
#include "merc.h"

#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

struct DescriptorFilter {
    // Accounts for the rare case where a quitting player is briefly "playing" but has no "person" behind the
    // descriptor.
    static auto playing() {
        return ranges::views::filter([](const Descriptor &d) { return d.is_playing() && d.person(); });
    }
    static auto except(const CHAR_DATA &ch) {
        return ranges::views::filter([&ch](const Descriptor &d) { return d.character() != &ch; });
    }
    static auto visible_to(const CHAR_DATA &ch) {
        return ranges::views::filter(
            [&ch](const Descriptor &d) { return d.character() && ch.can_see(*d.character()); });
    }
    static auto can_see(const CHAR_DATA &ch) {
        return ranges::views::filter(
            [&ch](const Descriptor &d) { return d.character() && d.character()->can_see(ch); });
    }
    static auto same_area(const CHAR_DATA &ch) {
        auto area = ch.in_room->area;
        return ranges::views::filter([area](const Descriptor &d) {
            return d.character() && d.character()->in_room && d.character()->in_room->area == area;
        });
    }
    static auto same_room(const CHAR_DATA &ch) {
        auto room = ch.in_room;
        return ranges::views::filter(
            [room](const Descriptor &d) { return d.character() && d.character()->in_room == room; });
    }
    static auto to_character() {
        return ranges::views::transform([](const Descriptor &d) -> const CHAR_DATA & { return *d.character(); });
    }
    static auto to_person() {
        return ranges::views::transform([](const Descriptor &d) -> const CHAR_DATA & { return *d.person(); });
    }
};