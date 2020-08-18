#include "DescriptorList.hpp"

#include <vector>

void DescriptorList::reap_closed() {
    std::vector<uint32_t> dead_channels;
    for (auto &pr : descriptors_)
        if (pr.second.is_closed())
            dead_channels.emplace_back(pr.first);
    for (auto chan : dead_channels)
        descriptors_.erase(chan);
}

Descriptor *DescriptorList::create(uint32_t channel_id) {
    if (find_by_channel(channel_id))
        return nullptr;
    return &descriptors_.emplace(channel_id, channel_id).first->second;
}

Descriptor *DescriptorList::find_by_channel(uint32_t channel_id) {
    if (auto it = descriptors_.find(channel_id); it != descriptors_.end())
        return &it->second;
    return nullptr;
}
