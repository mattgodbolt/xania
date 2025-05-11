/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include "Tip.hpp"
#include "Types.hpp"
#include <vector>

struct Logger;
struct DescriptorList;

class Tips {
public:
    // Returns the count of tips loaded, or -1 if it failed to load them.
    sh_int load(const std::string &tip_file);
    void send_tips(DescriptorList &descriptors);

private:
    std::vector<Tip> tips_{};
    size_t current_;
};
