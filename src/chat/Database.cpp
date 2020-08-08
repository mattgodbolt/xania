// allkeys.cpp
// chris busch

#include "Database.hpp"

namespace chat {

Record *Database::add_record() {
    Record *record;
    if ((record = new Record) == nullptr) {
        return nullptr;
    }
    if (top_ != nullptr)
        top_->next_ = record;
    current_ = top_ = record;
    top_->next_ = nullptr;
    if (record_count_ == 0)
        first_ = top_;
    record_count_++;
    return current_;
}

Record *Database::reset() { return (current_ = first_); }

Record *Database::next_record() {
    if (current_ != nullptr) {
        current_ = current_->next_;
    }
    return current_;
}

Database::~Database() {
    if (first_ != nullptr) {
        reset();
    }
}

}