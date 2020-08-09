// allkeys.cpp
// chris busch

#include "Database.hpp"

namespace chat {

void Database::add_record() {
    Record *record = new Record;
    if (top_ != nullptr)
        top_->next_ = record;
    current_ = top_ = record;
    top_->next_ = nullptr;
    if (record_count_ == 0)
        first_ = top_;
    record_count_++;
}

void Database::reset() { current_ = first_; }

void Database::next_record() {
    if (current_) {
        current_ = current_->next_;
    }
}

Database::~Database() {
    if (first_) {
        reset();
    }
}

}