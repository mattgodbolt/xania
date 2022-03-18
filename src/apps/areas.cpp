#include "Exit.hpp"
#include "Room.hpp"
#include "db.h"

#include "Area.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>
#include <unistd.h>
#include <unordered_set>

int main(int argc, const char **argv) {
    bool help{};
    std::string output;
    std::string area_dir = DEFAULT_AREA_PATH;
    auto cli =
        lyra::cli() | lyra::help(help).description("Output DOT showing area interconnectedness")
        | lyra::opt(area_dir, "area dir")["--areas"]("read area files from this directory (default " + area_dir + ")")
        | lyra::arg(output, "output DOT file")("output to the given DOT file, default stdout");

    auto result = cli.parse({argc, argv});
    if (!result) {
        fmt::print("Error in command line: {}\n", result.message());
        exit(1);
    } else if (help) {
        fmt::print("{}", cli);
        exit(0);
    }

    if (chdir(area_dir.c_str()) != 0)
        throw fmt::system_error(errno, "Unable to change to area directory {}", area_dir);
    boot_db();

    struct AreaInfo {
        std::string name;
        std::unordered_set<AreaInfo *> adjacent;
    };
    std::unordered_map<std::string, AreaInfo> areas;

    for (auto &room : all_rooms()) {
        auto *this_area = room.area;
        auto &area_info = areas[this_area->description()];
        for (auto direction : all_directions) {
            if (auto pexit = room.exits[direction]) {
                auto *to = pexit->u1.to_room;
                if (to && to->area != this_area) {
                    area_info.adjacent.emplace(&areas[to->area->description()]);
                }
            }
        }
    }

    auto out_file = (output.empty() || output == "-") ? stdout : fopen(output.c_str(), "w");
    fmt::print(out_file, "digraph {{\n");
    for (auto &[name, info] : areas) {
        for (auto &adj : info.adjacent)
            fmt::print(out_file,
                       R"(  "{}" -> "{}";)"
                       "\n",
                       name, adj->name);
    }
    fmt::print("}}\n");
    if (out_file != stdout)
        fclose(out_file);
}
