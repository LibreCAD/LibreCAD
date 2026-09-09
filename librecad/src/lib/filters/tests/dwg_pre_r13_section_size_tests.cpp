/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// A pre-R13 section-size field carries two flags in its top bits and the size
// in the low 30. The width of the mask is the whole content of the decision,
// and getting it wrong is silent: a too-narrow mask does not reject an
// oversized section, it truncates it, and the read then fails on whichever
// record happens to straddle the false end.
//
// These values come from the file header layout, not from the reader.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "dwgreaderR11.h"

TEST_CASE("a pre-R13 section size keeps every bit below the two flags",
          "[dwg][prer13][header]") {
    SECTION("the present-section flag is dropped and the size survives whole") {
        // Observed in a 23 MB R11 drawing: blocks_start 0x0006A7C1 and this
        // size put the section end at 0x15F0604, 218 bytes (the pre-R13
        // trailer) before the 0x15F06DE end of file.
        CHECK(preR13SectionSize(0x41585E43u) == 0x01585E43u);
        CHECK(0x0006A7C1u + preR13SectionSize(0x41585E43u) == 0x015F0604u);
    }

    SECTION("bit 24 and above belong to the size, not to the flags") {
        // The regression this pins: a 24-bit mask silently drops these,
        // shortening the section by a multiple of 16 MB.
        CHECK(preR13SectionSize(0x41000000u) == 0x01000000u);
        CHECK(preR13SectionSize(0x40FFFFFFu) == 0x00FFFFFFu);
        CHECK(preR13SectionSize(0x7FFFFFFFu) == 0x3FFFFFFFu);
    }

    SECTION("an absent section reads as empty") {
        // extras_size is 0x80000000 whenever extras_start is 0.
        CHECK(preR13SectionSize(0x80000000u) == 0u);
        CHECK(preR13SectionSize(0xC0000000u) == 0u);
    }

    SECTION("a field with no flags set passes through unchanged") {
        CHECK(preR13SectionSize(0u) == 0u);
        CHECK(preR13SectionSize(0x00000827u) == 0x00000827u);
        CHECK(preR13SectionSize(0x3FFFFFFFu) == 0x3FFFFFFFu);
    }

    SECTION("neither flag can leak into the size") {
        for (std::uint32_t flags = 0; flags < 4; ++flags) {
            const std::uint32_t raw = (flags << 30) | 0x0123456u;
            INFO("flag bits " << flags);
            CHECK(preR13SectionSize(raw) == 0x0123456u);
        }
    }
}
