#include "merc.h"

#include "AREA_DATA.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>

#include <unordered_set>

extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

static constexpr PerDirection<std::string_view> compass_pt = {"n", "e", "s", "w", "ne", "sw"};
static constexpr PerDirection<std::string_view> bidir_name = {"n/s", "e/w", "n/s", "e/w", "u/d", "u/d"};

void render_area(FILE *out_file, AREA_DATA *area) {
    fmt::print(out_file, "  subgraph cluster_{} {{\n", area->vnum);
    fmt::print(out_file, "    clusterrank=local;\n");
    fmt::print(out_file, "    label=\"{}\";\n", area->areaname);
    fmt::print(out_file, "    style=filled;\n");
    fmt::print(out_file, "    node [shape=box];\n");
    for (auto *first_room_with_hash : room_index_hash) {
        for (auto *pRoomIndex = first_room_with_hash; pRoomIndex; pRoomIndex = pRoomIndex->next) {
            if (pRoomIndex->area != area)
                continue;
            fmt::print("    v{} [label=\"{}\"];\n", pRoomIndex->vnum, pRoomIndex->name);
            for (auto door : all_directions) {
                if (auto pexit = pRoomIndex->exit[door]) {
                    auto *to = pexit->u1.to_room;
                    if (!to)
                        continue;
                    if (to != pRoomIndex && to->exit[reverse(door)]
                        && to->exit[reverse(door)]->u1.to_room == pRoomIndex) {
                        // Two way; write a single exit only for the lowest vnummed.
                        if (pRoomIndex->vnum <= to->vnum) {
                            fmt::print("    v{}:{} -> v{}:{} [dir=both label=\"{}\"];\n", pRoomIndex->vnum, compass_pt[door],
                                       to->vnum, compass_pt[reverse(door)], bidir_name[door]);
                        }
                    } else {
                        fmt::print("    v{}:{} -> v{} [label={}];\n", pRoomIndex->vnum, compass_pt[door], to->vnum,
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
        if (filter_area && a->vnum != filter_area)
            continue;
        render_area(out_file, a.get());
    }

    fmt::print("}}\n");
    if (out_file != stdout)
        fclose(out_file);
}
