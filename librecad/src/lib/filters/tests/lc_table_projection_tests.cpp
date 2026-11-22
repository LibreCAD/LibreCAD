/* Regression coverage for TABLE's single INSERT display projection. GPL-2.0-or-later. */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_settings.h"

TEST_CASE("TABLE grid dimensions do not multiply its block projection", "[dxf][table]") {
    static int argc = 1;
    static char arg[] = "table_projection_tests";
    static char *argv[] = {arg, nullptr};
    static QCoreApplication *app = QCoreApplication::instance()
        ? QCoreApplication::instance() : new QCoreApplication(argc, argv);
    (void)app;
    static bool settingsReady = [] {
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;

    const bool renderGrid = GENERATE(false, true);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile input(dir.filePath("empty.dxf"));
    REQUIRE(input.open(QIODevice::WriteOnly));
    REQUIRE(input.write("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
                        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n") > 0);
    input.close();

    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, input.fileName(), RS2::FormatDXFRW));
    const auto start = graphic.count();
    DRW_Table table;
    table.handle = 0x601;
    table.name = "TABLE_BLOCK";
    table.colcount = 3;
    table.rowcount = 2;
    table.colspace = 7;
    table.rowspace = 8;
    table.basePoint = DRW_Coord(10, 20, 3);
    table.xscale = 2;
    table.yscale = 3;
    table.zscale = 4;
    table.angle = 0.25;
    table.m_hasSemanticContent = renderGrid;
    table.m_content.m_columns.resize(3);
    table.m_content.m_rows.resize(2);
    for (auto &column : table.m_content.m_columns) column.m_width = 4;
    for (auto &row : table.m_content.m_rows) {
        row.m_height = 3;
        row.m_cells.resize(3);
    }
    filter.addTable(table);
    RS_Insert *projection = nullptr;
    for (auto i = start; i < graphic.count(); ++i) {
        if (auto *insert = dynamic_cast<RS_Insert *>(graphic.entityAt(static_cast<int>(i)))) {
            REQUIRE(projection == nullptr);
            projection = insert;
        }
    }
    REQUIRE(projection != nullptr);
    CHECK(projection->getCols() == 1);
    CHECK(projection->getRows() == 1);
    const auto projectedData = projection->getData();
    CHECK(projectedData.insertionPoint.x == 10);
    CHECK(projectedData.insertionPoint.y == 20);
    CHECK(projectedData.scaleFactor.x == 2);
    CHECK(projectedData.scaleFactor.y == 3);
    CHECK(projectedData.angle == 0.25);
    CHECK(table.colcount == 3);
    CHECK(table.rowcount == 2);
    const auto &tables = graphic.dwgAdvancedMetadata().tables();
    REQUIRE(tables.size() == 1);
    CHECK(tables[0].rowCount == 2);
    CHECK(tables[0].columnCount == 3);
    CHECK(tables[0].fallbackRendered == renderGrid);

    DRW_Insert array;
    array.name = "MINSERT_BLOCK";
    array.colcount = 4;
    array.rowcount = 5;
    array.colspace = 7;
    array.rowspace = 8;
    const auto arrayIndex = graphic.count();
    filter.addInsert(array);
    auto *insert = dynamic_cast<RS_Insert *>(graphic.entityAt(static_cast<int>(arrayIndex)));
    REQUIRE(insert != nullptr);
    CHECK(insert->getCols() == 4);
    CHECK(insert->getRows() == 5);
    CHECK(insert->getSpacing().x == 7);
    CHECK(insert->getSpacing().y == 8);
}
