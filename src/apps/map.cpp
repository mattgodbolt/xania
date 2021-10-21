#include "Area.hpp"
#include "AreaList.hpp"
#include "Exit.hpp"
#include "Room.hpp"
#include "db.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>
#include <unistd.h>
#include <unordered_set>

extern Room *room_hash[MAX_KEY_HASH];

static constexpr PerDirection<std::string_view> compass_pt = {"n", "e", "s", "w", "ne", "sw"};
static constexpr PerDirection<std::string_view> bidir_name = {"n/s", "e/w", "n/s", "e/w", "u/d", "u/d"};

void render_area(FILE *out_file, Area *area) {
    fmt::print(out_file, "  subgraph cluster_{} {{\n", area->num());
    fmt::print(out_file, "    clusterrank=local;\n");
    fmt::print(out_file, "    label=\"{}\";\n", area->description());
    fmt::print(out_file, "    style=filled;\n");
    fmt::print(out_file, "    node [shape=box];\n");
    for (auto *first_room_with_hash : room_hash) {
        for (auto *room = first_room_with_hash; room; room = room->next) {
            if (room->area != area)
                continue;
            fmt::print("    v{} [label=\"{}\"];\n", room->vnum, room->name);
            for (auto door : all_directions) {
                if (auto pexit = room->exit[door]) {
                    auto *to = pexit->u1.to_room;
                    if (!to)
                        continue;
                    if (to != room && to->exit[reverse(door)] && to->exit[reverse(door)]->u1.to_room == room) {
                        // Two way; write a single exit only for the lowest vnummed.
                        if (room->vnum <= to->vnum) {
                            fmt::print("    v{}:{} -> v{}:{} [dir=both label=\"{}\"];\n", room->vnum, compass_pt[door],
                                       to->vnum, compass_pt[reverse(door)], bidir_name[door]);
                        }
                    } else {
                        fmt::print("    v{}:{} -> v{} [label={}];\n", room->vnum, compass_pt[door], to->vnum,
                                   to_string(door));
                    }
                }
            }
        }
    }
    fmt::print(out_file, " }}\n");
}

int main(int argc, const char **argv) {
    bool help{};
    std::string output;
    std::string area_dir = DEFAULT_AREA_PATH;
    std::optional<int> filter_area{};
    auto cli =
        lyra::cli() | lyra::help(help).description("Output DOT files for the whole map")
        | lyra::opt(area_dir, "area dir")["--areas"]("read area files from this directory (default " + area_dir + ")")
        | lyra::opt(filter_area, "area num")["--area-filter"]("dump only this area number")
        | lyra::arg(output, "output DOT file")("output to the given DOT file, default stdout");

    auto result = cli.parse({argc, argv});
    if (!result) {
        fmt::print("Error in command line: {}\n", result.errorMessage());
        exit(1);
    } else if (help) {
        fmt::print("{}", cli);
        exit(0);
    }

    if (chdir(area_dir.c_str()) != 0)
        throw fmt::system_error(errno, "Unable to change to area directory {}", area_dir);
    boot_db();

    auto out_file = (output.empty() || output == "-") ? stdout : fopen(output.c_str(), "w");
    fmt::print(out_file, "digraph {{\n");

    for (auto &a : AreaList::singleton()) {
        if (filter_area && a->num() != filter_area)
            continue;
        render_area(out_file, a.get());
    }

    fmt::print("}}\n");
    if (out_file != stdout)
        fclose(out_file);
}
