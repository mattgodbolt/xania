#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class IdAllocator {
    class ReservationObj {
        IdAllocator &allocator_;
        uint32_t id_;
        friend IdAllocator;
        ReservationObj(IdAllocator &allocator, uint32_t id) : allocator_(allocator), id_(id) {}

    public:
        ~ReservationObj() { allocator_.release(id_); }
        ReservationObj(const ReservationObj &) = delete;
        ReservationObj(ReservationObj &&) = delete;
        ReservationObj &operator=(const ReservationObj &) = delete;
        ReservationObj &operator=(ReservationObj &&) = delete;

        [[nodiscard]] uint32_t id() const noexcept { return id_; }
    };

public:
    using Reservation = std::shared_ptr<ReservationObj>;

private:
    uint32_t next_id_{};
    std::vector<size_t> released_ids_;

    void release(uint32_t id);

public:
    [[nodiscard]] Reservation reserve();
};