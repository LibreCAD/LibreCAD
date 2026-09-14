/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**********************************************************************/

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>

#include "lc_actiontestsupport.h"
#include "rs_graphic.h"
#include "rs_image.h"

// A relative image file resolves against the image's own drawing. It used to go
// through the application window, which console tools do not have: creating one
// there crashed dxf2png at exit for every DXF with an IMAGE.
TEST_CASE("RS_Image resolves a relative file against its own drawing", "[rs_image]") {
    lc::test::application();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QImage pixels(3, 2, QImage::Format_RGB32);
    pixels.fill(Qt::red);
    REQUIRE(pixels.save(dir.filePath("pixels.png")));

    QFile drawing(dir.filePath("drawing.dxf"));
    REQUIRE(drawing.open(QIODevice::WriteOnly));
    drawing.close();

    RS_Graphic graphic;
    graphic.setFilename(drawing.fileName());
    const RS_ImageData data(0, RS_Vector(0., 0.), RS_Vector(1., 0.), RS_Vector(0., 1.), RS_Vector(1., 1.),
                            "pixels.png", 50, 50, 0);
    const RS_Image image(&graphic, data);

    CHECK(image.getWidth() == 3);
    CHECK(image.getHeight() == 2);
}
