#include "Tip.hpp"
#include "Char.hpp"
#include "DescriptorFilter.hpp"
#include "DescriptorList.hpp"
#include "Logging.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/numeric/accumulate.hpp>

namespace {

std::vector<Tip> tips;
size_t tip_current = 0;

}

Tip Tip::from_file(FILE *fp) { return Tip(fread_stdstring(fp)); }

void load_tipfile() {
    tips.clear();

    FILE *fp;
    if ((fp = fopen(Configuration::singleton().tip_file().c_str(), "r")) == nullptr) {
        bug("Couldn't open tip file \'{}\' for reading", Configuration::singleton().tip_file());
        return;
    }
    for (;;) {
        int c;
        while (isspace(c = getc(fp)))
            ;
        ungetc(c, fp);
        if (feof(fp))
            break;
        tips.emplace_back(Tip::from_file(fp));
    }
    fclose(fp);
    log_string("Loaded {} tips", tips.size());
}

void tip_players() {
    if (tips.empty())
        return;

    if (tip_current >= tips.size())
        tip_current = 0;

    auto tip = fmt::format("|WTip: {}|w\n\r", tips[tip_current].tip());
    ranges::for_each(descriptors().playing() | DescriptorFilter::to_person()
                         | ranges::views::filter([](const Char &ch) { return ch.is_set_extra(EXTRA_TIP_WIZARD); }),
                     [&tip](const Char &ch) { ch.send_to(tip); });
    tip_current++;
}

void do_tipwizard(Char *ch, ArgParser args) {
    if (args.empty()) {
        if (ch->toggle_extra(EXTRA_TIP_WIZARD)) {
            ch->send_line("Tipwizard activated!");
        } else {
            ch->send_line("Tipwizard deactivated.");
        }
        return;
    }
    auto arg = args.shift();
    if (matches(arg, "on")) {
        ch->set_extra(EXTRA_TIP_WIZARD);
        ch->send_line("Tipwizard activated!");
    } else if (matches(arg, "off")) {
        ch->remove_extra(EXTRA_TIP_WIZARD);
        ch->send_line("Tipwizard deactivated.");
    } else
        ch->send_line("Syntax: tipwizard {on/off}");
}
