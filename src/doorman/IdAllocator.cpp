#include "IdAllocator.hpp"

void IdAllocator::release(IdType id) { released_ids_.push_back(id); }

IdAllocator::Reservation IdAllocator::reserve() {
    if (!released_ids_.empty()) {
        auto id = released_ids_.back();
        released_ids_.pop_back();
        return Reservation(new ReservationObj(*this, id));
    }
    auto id = next_id_++;
    return Reservation(new ReservationObj(*this, id));
}
