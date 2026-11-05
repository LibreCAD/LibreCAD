/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Hidden corpus loop-count dumper: iterates every RS_Hatch in a user-supplied
// list of DXF files, runs validate() for pattern hatches and update() for
// solid hatches as HatchFillIface does, and writes one TSV row per (file,
// block, index, frame). The dumper changes no behaviour; the welding step
// re-runs it to spot loop count drops under translation and rotation.

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "lc_containertraverser.h"
#include "lc_filtertestsupport.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_hatch.h"

namespace {

constexpr double kDegToRad = 3.141592653589793 / 180.0;

const char* envOrNull(const char* name) {
    const char* value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

std::vector<std::string> readLines(const std::filesystem::path& list) {
    std::vector<std::string> lines;
    std::ifstream in(list);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
            lines.push_back(line);
    }
    return lines;
}

// Dumps one TSV row per (file, block, hatchIndex, frame). Columns:
//   file, block, index, frame, solid, pattern, countLoops, countAllLoops,
//   error, area.
// frame is "stored", "move_5e5_4e6" or "rot30_move_3e7_3e7". For pattern
// hatches only validate() runs; update() needs the pattern library.
struct DumpRow {
    std::string file;
    std::string block;
    int index = 0;
    std::string frame;
    bool solid = false;
    std::string pattern;
    int countLoops = 0;
    int countAllLoops = 0;
    int error = 0;
    double area = 0.0;
};

void runHatchFrames(RS_Hatch& hatch, std::vector<DumpRow>& rows,
                     const std::string& file, const std::string& block,
                     int index) {
    const auto run = [&](const std::string& label) {
        DumpRow row;
        row.file = file;
        row.block = block;
        row.index = index;
        row.frame = label;
        row.solid = hatch.isSolid();
        row.pattern = hatch.getPattern().toUtf8().toStdString();
        if (hatch.isSolid()) {
            hatch.update();
            row.error = static_cast<int>(hatch.getUpdateError());
            row.area = hatch.getTotalArea();
        } else {
            const bool ok = hatch.validate();
            row.error = ok ? static_cast<int>(RS_Hatch::HATCH_OK)
                            : static_cast<int>(RS_Hatch::HATCH_INVALID_CONTOUR);
        }
        row.countLoops = hatch.countLoops();
        row.countAllLoops = hatch.countAllLoops();
        rows.push_back(std::move(row));
    };

    run("stored");
    hatch.move(RS_Vector(5e5, 4e6));
    run("move_5e5_4e6");
    hatch.move(RS_Vector(-5e5, -4e6));
    hatch.rotate(RS_Vector(0.0, 0.0), 30.0 * kDegToRad);
    hatch.move(RS_Vector(3e7, 3e7));
    run("rot30_move_3e7_3e7");
    hatch.move(RS_Vector(-3e7, -3e7));
    hatch.rotate(RS_Vector(0.0, 0.0), -30.0 * kDegToRad);
}

void dumpContainer(const RS_EntityContainer& container,
                    std::vector<DumpRow>& rows,
                    const std::string& file,
                    const std::string& block) {
    int index = 0;
    for (RS_Entity* entity :
         lc::LC_ContainerTraverser{const_cast<RS_EntityContainer&>(container),
                                    RS2::ResolveNone}.entities()) {
        if (entity != nullptr && entity->rtti() == RS2::EntityHatch) {
            runHatchFrames(*static_cast<RS_Hatch*>(entity), rows,
                            file, block, index++);
        }
    }
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch loop corpus dump", "[.][corpus][lc-hatchfix][nesting]") {
    const char* corpus = envOrNull("LC_HATCH_CORPUS");
    const char* outPath = envOrNull("LC_HATCH_CORPUS_OUT");
    if (corpus == nullptr || outPath == nullptr) {
        SKIP("LC_HATCH_CORPUS and LC_HATCH_CORPUS_OUT must be set");
    }
    REQUIRE(lc::test::ensureSettings());

    const auto files = readLines(corpus);
    REQUIRE_FALSE(files.empty());

    std::vector<DumpRow> rows;
    rows.reserve(files.size() * 4);
    for (const std::string& fileText : files) {
        const std::filesystem::path file(fileText);
        if (!std::filesystem::exists(file))
            continue;
        RS_Graphic graphic;
        lc::test::QuietLog quiet;
        if (!lc::test::importDrawing(graphic, file, RS2::FormatDXFRW))
            continue;
        dumpContainer(graphic, rows, file.string(), std::string());
        RS_BlockList* blocks = graphic.getBlockList();
        if (blocks != nullptr) {
            for (unsigned i = 0; i < blocks->count(); ++i) {
                RS_Block* block = blocks->at(i);
                if (block == nullptr)
                    continue;
                dumpContainer(*block, rows, file.string(),
                               block->getName().toUtf8().toStdString());
            }
        }
    }

    std::ofstream out(outPath);
    out << "file\tblock\tindex\tframe\tsolid\tpattern\tcountLoops\t"
           "countAllLoops\terror\tarea\n";
    for (const DumpRow& row : rows) {
        out << row.file << '\t' << row.block << '\t' << row.index << '\t'
            << row.frame << '\t' << (row.solid ? 1 : 0) << '\t'
            << row.pattern << '\t' << row.countLoops << '\t'
            << row.countAllLoops << '\t' << row.error << '\t';
        if (std::isfinite(row.area))
            out << row.area;
        else
            out << "nan";
        out << '\n';
    }
    out.flush();
    REQUIRE(out.good());
    CHECK_FALSE(rows.empty());
}
