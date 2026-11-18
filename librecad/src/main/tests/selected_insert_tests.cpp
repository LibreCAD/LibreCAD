// SPDX-License-Identifier: GPL-2.0-or-later
// A rebuilt selected insert must retain selection on its generated geometry.

#include <QtTest>

#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_spline.h"

class SelectedInsertTests : public QObject {
    Q_OBJECT

private slots:
    void rebuildSelectedInsert()
    {
        RS_Graphic graphic;
        auto* block = new RS_Block(&graphic, RS_BlockData("curve", {0., 0.}, false));
        RS_SplineData splineData(3, false);
        splineData.controlPoints = {{0., 0.}, {10., 10.}, {20., 10.}, {30., 0.}};
        auto* spline = new RS_Spline(block, splineData);
        spline->update();
        block->addEntity(spline);
        block->addEntity(new RS_Line(block, {0., 0.}, {10., 0.}));
        QVERIFY(graphic.addBlock(block));

        auto* insert = new RS_Insert(&graphic, RS_InsertData("curve", {0., 0.}, {1., 1.},
                                                             0., 1, 1, {0., 0.}, nullptr, RS2::Update));
        graphic.addEntity(insert);
        QVERIFY(insert->setSelected(true));
        for (int rebuild = 0; rebuild < 2; ++rebuild) {
            graphic.updateInserts();
            QVERIFY(insert->isSelected());
            QCOMPARE(insert->count(), 2u);
            auto* rebuiltSpline = dynamic_cast<RS_Spline*>(insert->entityAt(0));
            QVERIFY(rebuiltSpline);
            QVERIFY(rebuiltSpline->isSelected());
            QVERIFY(rebuiltSpline->count() > 0);
            for (RS_Entity* segment : *rebuiltSpline)
                QVERIFY(segment->isSelected());
            QVERIFY(insert->entityAt(1)->isSelected());
        }
        QVERIFY(insert->setSelected(false));
        auto* rebuiltSpline = static_cast<RS_Spline*>(insert->entityAt(0));
        for (RS_Entity* segment : *rebuiltSpline)
            QVERIFY(!segment->isSelected());
    }
};

QTEST_MAIN(SelectedInsertTests)
#include "selected_insert_tests.moc"
