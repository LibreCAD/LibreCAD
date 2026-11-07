/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// An R13/R14 DWG keeps a layer's frozen and locked state in bits of their own, which libdxfrw
// turns into the DXF flags (1 frozen, 4 locked). It took the locked bit as 8, so LibreCAD,
// which reads 4, opened every locked layer of an R13 or R14 file unlocked.
//
// The fixtures were written by the ODA File Converter (ACAD13 and ACAD14) from a DXF with the
// layers "Plain", "Frozen", "Locked" and "FrozenLocked"; R13/R14 keep the names in capitals.

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "lc_actiontestsupport.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"

TEST_CASE("Layers of an R13 or R14 DWG keep their frozen and locked state", "[dwg][layers][layer-flags][fixture]") {
    (void)lc::test::application();
    for (const char* fixture : {"lc_layer_flags_ac1012.dwg", "lc_layer_flags_ac1014.dwg"}) {
        CAPTURE(fixture);
        const QString path = QDir(QStringLiteral(LIBRECAD_TEST_DIR)).filePath(QString::fromLatin1(fixture));
        REQUIRE(QFileInfo::exists(path));
        RS_Graphic graphic;
        graphic.initForNewDocument();
        // with a callback: without one, a failed import waits in a message box
        REQUIRE(RS_FileIO::instance()->fileImport(graphic, path, RS2::FormatDWG,
                                                  [](bool, const QString&) { return false; }));

        struct Expected {
            const char* name;
            bool frozen;
            bool locked;
        };
        for (const Expected& expected : {Expected{"PLAIN", false, false}, Expected{"FROZEN", true, false},
                                         Expected{"LOCKED", false, true}, Expected{"FROZENLOCKED", true, true}}) {
            CAPTURE(expected.name);
            RS_Layer* layer = graphic.findLayer(QString::fromLatin1(expected.name));
            REQUIRE(layer != nullptr);
            CHECK(layer->isFrozen() == expected.frozen);
            CHECK(layer->isLocked() == expected.locked);
        }
    }
}
