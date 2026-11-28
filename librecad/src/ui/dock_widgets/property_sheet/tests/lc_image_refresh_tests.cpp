/****************************************************************************
** Copyright (C) 2026 LibreCAD (librecad.org)
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <vector>

#include "lc_actiontestsupport.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_image.h"

TEST_CASE("Raster refresh preserves shared image definition paths", "[image][transaction]") {
    (void)lc::test::application();
    const int imageCount = GENERATE(2, 64, 128, 256);
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const QString imagePath = temp.filePath(QStringLiteral("image.png"));
    QImage raster(16, 12, QImage::Format_RGB32);
    raster.fill(Qt::red);
    REQUIRE(raster.save(imagePath));
    const QString source = temp.filePath(QStringLiteral("source.dxf"));
    const QString output = temp.filePath(QStringLiteral("output.dxf"));
    RS_Graphic original;
    original.initForNewDocument();
    for (int i = 0; i < imageCount; ++i)
        original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29 * i, 2),
            RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12), imagePath, 50, 50, 0)));
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(original, source, RS2::FormatDXFRW));
    RS_Graphic imported;
    REQUIRE(filter.fileImport(imported, source, RS2::FormatDXFRW));
    imported.setFilename(source);
    std::vector<RS_Image *> images;
    for (RS_Entity *entity : imported)
        if (entity && entity->rtti() == RS2::EntityImage)
            images.push_back(static_cast<RS_Image *>(entity));
    REQUIRE(images.size() == static_cast<std::size_t>(imageCount));
    REQUIRE(images[0]->getFile() == images[1]->getFile());
    const double otherX = images[1]->getInsertionPoint().x;
    images[0]->update();
    CHECK(images[0]->getFile() == imagePath);
    CHECK(images[0]->getFile() == images[1]->getFile());
    images[0]->setInsertionPoint(RS_Vector(4, 2));
    images[0]->setFile(QStringLiteral("different.png"));
    QFile destination(output);
    REQUIRE(destination.open(QIODevice::WriteOnly));
    REQUIRE(destination.write("sentinel") == 8);
    destination.close();
    CHECK_FALSE(filter.fileExport(imported, output, RS2::FormatDXFRW));
    REQUIRE(destination.open(QIODevice::ReadOnly));
    CHECK(destination.readAll() == QByteArray("sentinel"));
    destination.close();
    images[0]->setFile(imagePath);
    REQUIRE(filter.fileExport(imported, output, RS2::FormatDXFRW));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, output, RS2::FormatDXFRW));
    REQUIRE(reread.dwgAdvancedMetadata().rasterImages().size() == static_cast<std::size_t>(imageCount));
    REQUIRE(reread.dwgAdvancedMetadata().imageDefinitions().size() == 1);
    REQUIRE(reread.dwgAdvancedMetadata().imageDefinitionReactors().size() == static_cast<std::size_t>(imageCount));
    std::vector<double> x;
    for (RS_Entity *entity : reread)
        if (entity && entity->rtti() == RS2::EntityImage)
            x.push_back(static_cast<RS_Image *>(entity)->getInsertionPoint().x);
    REQUIRE(x.size() == static_cast<std::size_t>(imageCount));
    CHECK(x[0] == 4.0);
    CHECK(x[1] == otherX);
    const QString proof = qEnvironmentVariable("LIBRECAD_IMAGE_REFRESH_PROOF");
    if (imageCount == 64 && !proof.isEmpty()) {
        REQUIRE(QFile::copy(source, proof + QStringLiteral("-source.dxf")));
        REQUIRE(QFile::copy(output, proof + QStringLiteral("-output.dxf")));
    }
}
