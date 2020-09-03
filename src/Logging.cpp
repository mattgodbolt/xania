#include "Logging.hpp"

#include "DescriptorList.hpp"
#include "common/Time.hpp"

namespace {
std::string area_filename;
FILE *area_file{};
}

BugAreaFileContext::BugAreaFileContext(std::string_view name, FILE *file) {
    area_filename = name;
    area_file = file;
}

BugAreaFileContext::~BugAreaFileContext() {
    area_filename.clear();
    area_file = nullptr;
}

/* Reports a bug. */
void bug(std::string_view message) {
    if (area_file) {
        int iLine;
        if (area_file == stdin) {
            iLine = 0;
        } else {
            auto iChar = ftell(area_file);
            fseek(area_file, 0, SEEK_SET);
            for (iLine = 0; ftell(area_file) < iChar; iLine++) {
                while (getc(area_file) != '\n')
                    ;
            }
            fseek(area_file, iChar, SEEK_SET);
        }
        log_string("[*****] FILE: {} LINE: {}", area_filename, iLine);
    }

    log_string("[*****] BUG: {}", message);
}

/* New log - takes a level and broadcasts to IMMs on WIZNET */
void log_new(std::string_view str, int loglevel, int level) {
    // One day use spdlog here?
    fmt::print(stderr, "{} :: {}\n", Clock::now(), str);

    if (loglevel == EXTRA_WIZNET_DEBUG)
        level = std::max(level, 96); // Prevent non-SOCK ppl finding out addresses

    auto wiznet_msg = fmt::format("|GWIZNET:|g {}|w\n\r", str);
    for (auto &d : descriptors().playing()) {
        auto *ch = d.person();
        if (ch->is_npc() || !is_set_extra(ch, EXTRA_WIZNET_ON) || !is_set_extra(ch, loglevel)
            || (ch->get_trust() < level))
            continue;
        d.character()->send_to(wiznet_msg);
    }
}
