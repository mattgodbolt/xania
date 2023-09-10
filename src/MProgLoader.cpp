/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#include "MProgLoader.hpp"
#include "Area.hpp"
#include "AreaList.hpp"
#include "Logging.hpp"
#include "MProgProgram.hpp"
#include "MProgTypeFlag.hpp"
#include "MobIndexData.hpp"
#include "WrappedFd.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <optional>

namespace MProg {

namespace impl {

TypeFlag name_to_type(std::string_view name) {
    if (matches(name, "in_file_prog"))
        return TypeFlag::InFile;
    if (matches(name, "act_prog"))
        return TypeFlag::Act;
    if (matches(name, "speech_prog"))
        return TypeFlag::Speech;
    if (matches(name, "rand_prog"))
        return TypeFlag::Random;
    if (matches(name, "fight_prog"))
        return TypeFlag::Fight;
    if (matches(name, "hitprcnt_prog"))
        return TypeFlag::HitPercent;
    if (matches(name, "death_prog"))
        return TypeFlag::Death;
    if (matches(name, "entry_prog"))
        return TypeFlag::Entry;
    if (matches(name, "greet_prog"))
        return TypeFlag::Greet;
    if (matches(name, "all_greet_prog"))
        return TypeFlag::AllGreet;
    if (matches(name, "give_prog"))
        return TypeFlag::Give;
    if (matches(name, "bribe_prog"))
        return TypeFlag::Bribe;
    return TypeFlag::Error;
}

std::optional<Program> try_load_one_mob_prog(std::string_view file_name, FILE *prog_file) {
    const auto prog_type = name_to_type(fread_word(prog_file));
    if (prog_type == TypeFlag::Error) {
        bug("mobprog {} type error {}", file_name, magic_enum::enum_name<TypeFlag>(prog_type));
        return std::nullopt;
    }
    if (prog_type == TypeFlag::InFile) {
        bug("mobprog {} contains a call to file which is not supported yet.", file_name);
        return std::nullopt;
    }
    const auto prog_args = fread_string(prog_file);
    const auto script = fread_string(prog_file);
    const std::vector<std::string> lines = split_lines<std::vector<std::string>>(script);
    if (lines.empty()) {
        bug("mobprog {} contains no script!", file_name);
        return std::nullopt;
    }
    const auto prog = Program{prog_type, prog_args, std::move(lines)};
    return prog;
}

} // namespace impl

bool read_program(std::string_view file_name, FILE *prog_file, MobIndexData *mobIndex) {
    bool done = false;
    while (!done) {
        switch (fread_letter(prog_file)) {
        case '>': {
            if (const auto opt_mob_prog = impl::try_load_one_mob_prog(file_name, prog_file)) {
                set_enum_bit(mobIndex->progtypes, opt_mob_prog->type);
                mobIndex->mobprogs.push_back(std::move(*opt_mob_prog));
            } else {
                return false;
            }
            break;
        }
        case '|':
            if (mobIndex->mobprogs.empty()) {
                bug("mobprog {} empty file.", file_name);
                return false;
            } else {
                done = true;
            }
            break;
        default: bug("mobprog {} syntax error.", file_name); return false;
        }
    }
    return true;
}

// Snarf a MOBprogram section from the area file.
void load_mobprogs(FILE *fp) {
    char letter;
    auto area_last = AreaList::singleton().back();
    if (area_last == nullptr) {
        bug("load_mobprogs: no #AREA seen yet!");
        exit(1);
    }

    for (;;) {
        switch (letter = fread_letter(fp)) {
        default: bug("load_mobprogs: bad command '{}'.", letter); exit(1);
        case 'S':
        case 's': fread_to_eol(fp); return;
        case '*': fread_to_eol(fp); break;
        case 'M':
        case 'm':
            const auto vnum = fread_number(fp);
            if (auto *mob = get_mob_index(vnum)) {
                const auto file_name = fread_word(fp);
                const auto file_path = fmt::format("{}{}", Configuration::singleton().area_dir(), file_name);
                if (auto prog_file = WrappedFd::open(file_path)) {
                    if (!read_program(file_name, static_cast<FILE *>(prog_file), mob)) {
                        exit(1);
                    }
                    fread_to_eol(fp);
                    break;
                } else {
                    bug("Mob: {} couldnt open mobprog file {}.", mob->vnum, file_path);
                    exit(1);
                }
            } else {
                bug("load_mobprogs: vnum {} doesnt exist", vnum);
                exit(1);
            }
        }
    }
}

}