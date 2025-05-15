#include "Area.hpp"
#include "AreaList.hpp"
#include "Exit.hpp"
#include "MudImpl.hpp"
#include "Room.hpp"
#include "db.h"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>
#include <unistd.h>

static constexpr PerDirection<std::string_view> compass_pt = {"n", "e", "s", "w", "ne", "sw"};
static constexpr PerDirection<std::string_view> bidir_name = {"n/s", "e/w", "n/s", "e/w", "u/d", "u/d"};

void render_area(FILE *out_file, Area *area) {
    fmt::print(out_file, "  subgraph cluster_{} {{\n", area->num());
    fmt::print(out_file, "    clusterrank=local;\n");
    fmt::print(out_file, "    label=\"{}\";\n", area->short_name());
    fmt::print(out_file, "    style=filled;\n");
    fmt::print(out_file, "    node [shape=box];\n");
    for (auto &room : all_rooms()) {
        if (room.area != area)
            continue;
        fmt::print("    v{} [label=\"{}\"];\n", room.vnum, room.name);
        for (auto direction : all_directions) {
            if (auto pexit = room.exits[direction]) {
                auto *to = pexit->u1.to_room;
                if (!to)
                    continue;
                if (to->vnum != room.vnum && to->exits[reverse(direction)]
                    && to->exits[reverse(direction)]->u1.to_room->vnum == room.vnum) {
                    // Two way; write a single exit only for the lowest vnummed.
                    if (room.vnum <= to->vnum) {
                        fmt::print("    v{}:{} -> v{}:{} [dir=both label=\"{}\"];\n", room.vnum, compass_pt[direction],
                                   to->vnum, compass_pt[reverse(direction)], bidir_name[direction]);
                    }
                } else {
                    fmt::print("    v{}:{} -> v{} [label={}];\n", room.vnum, compass_pt[direction], to->vnum,
                               to_string(direction));
                }
            }
        }
    }
    fmt::print(out_file, " }}\n");
}

template <>
struct fmt::formatter<lyra::cli> : ostream_formatter {};

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
        fmt::print("Error in command line: {}\n", result.message());
        exit(1);
    } else if (help) {
        fmt::print("{}", cli);
        exit(0);
    }

    if (chdir(area_dir.c_str()) != 0)
        throw fmt::system_error(errno, "Unable to change to area directory {}", area_dir);
    const auto mud = std::make_unique<MudImpl>();
    boot_db(*mud.get());

    auto out_file = (output.empty() || output == "-") ? stdout : fopen(output.c_str(), "w");
    fmt::print(out_file, "digraph {{\n");

    for (auto &a : mud->areas()) {
        if (filter_area && a->num() != filter_area)
            continue;
        render_area(out_file, a.get());
    }

    fmt::print("}}\n");
    if (out_file != stdout)
        fclose(out_file);
}
