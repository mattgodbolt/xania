/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "Tips.hpp"
#include "DescriptorFilter.hpp"
#include "DescriptorList.hpp"
#include "Logging.hpp"
#include "string_utils.hpp"

#include <range/v3/algorithm/for_each.hpp>

sh_int Tips::load(const std::string &tip_file) {
    tips_.clear();

    FILE *fp;
    if ((fp = fopen(tip_file.c_str(), "r")) == nullptr) {
        return -1;
    }
    for (;;) {
        int c;
        while (isspace(c = getc(fp)))
            ;
        ungetc(c, fp);
        if (feof(fp))
            break;
        tips_.emplace_back(Tip::from_file(fp));
    }
    fclose(fp);
    return tips_.size();
}

void Tips::send_tips(DescriptorList &descriptors) {
    if (tips_.empty())
        return;
    if (current_ >= tips_.size())
        current_ = 0;
    auto tip = fmt::format("|WTip: {}|w\n\r", tips_[current_].tip());
    ranges::for_each(descriptors.playing() | DescriptorFilter::to_person() | ranges::views::filter([](const Char &ch) {
                         return ch.is_set_extra(CharExtraFlag::TipWizard);
                     }),
                     [&tip](const Char &ch) { ch.send_to(tip); });
    current_++;
}

void do_tipwizard(Char *ch, ArgParser args) {
    if (args.empty()) {
        if (ch->toggle_extra(CharExtraFlag::TipWizard)) {
            ch->send_line("Tipwizard activated!");
        } else {
            ch->send_line("Tipwizard deactivated.");
        }
        return;
    }
    auto arg = args.shift();
    if (matches(arg, "on")) {
        ch->set_extra(CharExtraFlag::TipWizard);
        ch->send_line("Tipwizard activated!");
    } else if (matches(arg, "off")) {
        ch->remove_extra(CharExtraFlag::TipWizard);
        ch->send_line("Tipwizard deactivated.");
    } else
        ch->send_line("Syntax: tipwizard {on/off}");
}
