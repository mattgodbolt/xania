#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class IdAllocator {
public:
    using IdType = uint32_t;

private:
    class ReservationObj {
        IdAllocator &allocator_;
        IdType id_;
        friend IdAllocator;
        ReservationObj(IdAllocator &allocator, IdType id) : allocator_(allocator), id_(id) {}

    public:
        ~ReservationObj() { allocator_.release(id_); }
        ReservationObj(const ReservationObj &) = delete;
        ReservationObj(ReservationObj &&) = delete;
        ReservationObj &operator=(const ReservationObj &) = delete;
        ReservationObj &operator=(ReservationObj &&) = delete;

        [[nodiscard]] IdType id() const noexcept { return id_; }
    };

public:
    using Reservation = std::shared_ptr<ReservationObj>;

private:
    IdType next_id_{};
    std::vector<IdType> released_ids_;

    void release(IdType id);

public:
    [[nodiscard]] Reservation reserve();
};