#pragma once

#include "Descriptor.hpp"
#include "merc.h"

#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

#include <unordered_map>

class DescriptorList {
    // We hand out pointers to Descriptors, so this structure must be address-stable.
    std::unordered_map<uint32_t, Descriptor> descriptors_;

public:
    // Return all descriptors, including those in the process of logging in.
    [[nodiscard]] auto all() const noexcept {
        return descriptors_ | ranges::views::transform([](auto &pr) -> const Descriptor & { return pr.second; });
    }
    [[nodiscard]] auto all() noexcept {
        return descriptors_ | ranges::views::transform([](auto &pr) -> Descriptor & { return pr.second; });
    }

    // Return all descriptors for characters in the "playing" state. Accounts for the rare case where a quitting player
    // is briefly "playing" but has no "person" behind the descriptor.
    [[nodiscard]] auto playing() const noexcept {
        return all() | ranges::views::filter([](const Descriptor &d) { return d.is_playing() && d.person(); });
    }

    // Return all descriptors for playing characters, skipping the given character.
    [[nodiscard]] auto all_but(const CHAR_DATA *ch) const noexcept {
        return playing() | ranges::views::filter([ch](const Descriptor &d) { return d.character() != ch; });
    }
    // Return all descriptors for playing characters who are visible to the given character.
    [[nodiscard]] auto all_visible_to(const CHAR_DATA *ch) const noexcept {
        return all_but(ch) | ranges::views::filter([ch](const Descriptor &d) { return can_see(ch, d.character()); });
    }
    // Return all descriptors for playing characters who can see the given character.
    [[nodiscard]] auto all_who_can_see(const CHAR_DATA *ch) const noexcept {
        return all_but(ch) | ranges::views::filter([ch](const Descriptor &d) { return can_see(d.character(), ch); });
    }

    // Try and find a descriptor by channel id.
    Descriptor *find_by_channel(uint32_t channel_id);

    // Create a new descriptor associated with the given channel. Fails if the channel is a duplicate.
    // The returned descriptor is owned by the list, and will remain valid until the descriptor is closed and
    // reap_closed is called.
    Descriptor *create(uint32_t channel_id);

    // Recover resources used by closed sockets.
    void reap_closed();
};

DescriptorList &descriptors();
