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
    [[nodiscard]] auto all() const noexcept {
        return descriptors_ | ranges::views::transform([](auto &pr) -> const Descriptor & { return pr.second; });
    }
    [[nodiscard]] auto all() noexcept {
        return descriptors_ | ranges::views::transform([](auto &pr) -> Descriptor & { return pr.second; });
    }
    [[nodiscard]] auto playing() const noexcept {
        return all() | ranges::views::filter([](const Descriptor &d) { return d.is_playing(); });
    }
    [[nodiscard]] auto all_but(const CHAR_DATA *ch) const noexcept {
        return all()
               | ranges::views::filter([ch](const Descriptor &d) { return d.is_playing() && d.character() != ch; });
    }
    [[nodiscard]] auto all_visible_to(const CHAR_DATA *ch) const noexcept {
        return all() | ranges::views::filter([ch](const Descriptor &d) {
                   return d.is_playing() && d.character() != ch && can_see(ch, d.character());
               });
    }
    Descriptor *find_by_channel(uint32_t channel_id);
    // Create a new descriptor associated with the given channel. Fails if the channel is a duplicate.
    Descriptor *create(uint32_t channel_id);

    // reap closed sockets.
    void reap_closed();
};

DescriptorList &descriptors();