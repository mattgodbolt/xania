#include "IdAllocator.hpp"

#include "Logger.hpp"

static auto &logger() {
    static auto the_logger = logger_for("IdAllocator");
    return the_logger;
}

void IdAllocator::release(uint32_t id) {
    logger().debug("Releasing ID {}", id);
    released_ids_.push_back(id);
}

IdAllocator::Reservation IdAllocator::reserve() {
    if (!released_ids_.empty()) {
        auto id = released_ids_.back();
        released_ids_.pop_back();
        logger().debug("Allocating released ID {}", id);
        return Reservation(new ReservationObj(*this, id));
    }
    auto id = next_id_++;
    logger().debug("Allocating new ID {}", id);
    return Reservation(new ReservationObj(*this, id));
}
