// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

#include <memory>

#include <QtTest>
#include <QtWidgets>

#include "doc_plugin_interface.h"
#include "dwgreader.h"
#include "lc_actiondrawrectangle3points.h"
#include "lc_actionmodifylinejoin.h"
#include "picfile.h"
#include "qc_applicationwindow.h"
#include "qg_dlghatch.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_painterqt.h"
#include "rs_patternlist.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"

namespace {

class LoopContainer : public RS_EntityContainer {
public:
    using RS_EntityContainer::getLoops;
};

class RectangleAction : public LC_ActionDrawRectangle3Points {
public:
    using LC_ActionDrawRectangle3Points::LC_ActionDrawRectangle3Points;
    using LC_ActionDrawRectangle3Points::createPolyline;
    using LC_ActionDrawRectangle3Points::doGetRelativeZeroAfterTrigger;
    using LC_ActionDrawRectangle3Points::doInitialSnapToRelativeZero;
    using LC_ActionDrawRectangle3Points::processCommandValue;
};

class LineJoinAction : public LC_ActionModifyLineJoin {
public:
    using LC_ActionModifyLineJoin::LC_ActionModifyLineJoin;
    using LC_ActionModifyLineJoin::doOnLeftMouseButtonRelease;
    using LC_ActionModifyLineJoin::doPreparePreviewEntities;
};

} // namespace

class LC_StaticAnalysisTests : public QObject {
    Q_OBJECT

    QTemporaryDir directory;

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, directory.path());
        QCoreApplication::setOrganizationName("LibreCAD-StaticAnalysisTests");
        QCoreApplication::setApplicationName(QString::number(qApp->applicationPid()));
        QCoreApplication::setApplicationVersion("2.2.1");
        QSettings settings;
        settings.setValue("Startup/FirstLoad", false);
        settings.setValue("Defaults/AutoBackupDocument", false);
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        RS_SYSTEM->init("LibreCAD", "2.2.1", "librecad", QString(LIBRECAD_TEST_SOURCE_DIR) + "/unix");
        RS_FONTLIST->init();
        RS_PATTERNLIST->init();
        QC_ApplicationWindow::getAppWindow();
    }

    void cleanupTestCase()
    {
        QC_ApplicationWindow::getAppWindow().reset();
    }

    void selectionWithoutView()
    {
        RS_EntityContainer container;
        auto* line = new RS_Line(&container, RS_LineData({0, 0}, {10, 0}));
        container.addEntity(line);
        RS_Selection selection(container);
        selection.selectAll();
        QVERIFY(line->isSelected());
        selection.deselectAll();
        QVERIFY(!line->isSelected());
    }

    void selectionWithTypeFilter()
    {
        RS_EntityContainer container;
        auto* line = new RS_Line(&container, RS_LineData({0, 0}, {10, 0}));
        auto* circle = new RS_Circle(&container, RS_CircleData({0, 0}, 1));
        container.addEntity(line);
        container.addEntity(circle);
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&container);
        view.setTypeToSelect(RS2::EntityCircle);
        RS_Selection selection(container, &view);
        selection.selectAll();
        QVERIFY(!line->isSelected());
        QVERIFY(circle->isSelected());
        selection.deselectAll();
        QVERIFY(!circle->isSelected());
    }

    void lineDrawWithoutContext_data()
    {
        QTest::addColumn<bool>("missingPainter");
        QTest::newRow("painter") << true;
        QTest::newRow("view") << false;
    }

    void lineDrawWithoutContext()
    {
        QFETCH(bool, missingPainter);
        RS_Line line(nullptr, RS_LineData({0, 0}, {10, 0}));
        QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
        RS_PainterQt painter(&image);
        RS_StaticGraphicView view(100, 100, &painter);
        double offset = 7;
        line.draw(missingPainter ? nullptr : &painter, missingPainter ? &view : nullptr, offset);
        QCOMPARE(offset, 7.0);
    }

    void loopsWithNullEntry()
    {
        LoopContainer container;
        container.addEntity(new RS_Line(&container, RS_LineData({0, 0}, {1, 0})));
        container.setEntityAt(0, nullptr);
        container.addEntity(new RS_Circle(&container, RS_CircleData({0, 0}, 1)));
        QCOMPARE(container.getLoops().size(), std::size_t{1});
    }

    void missingHatchPattern()
    {
        RS_Hatch hatch(nullptr, RS_HatchData(false, 1, 0, "missing-static-analysis-pattern"));
        QG_DlgHatch dialog;
        dialog.setHatch(hatch, false);
        dialog.setPattern("missing-static-analysis-pattern");
        dialog.cbEnablePreview->setChecked(true);
        dialog.updatePreview();
        QCOMPARE(dialog.cbPattern->currentText(), QString("missing-static-analysis-pattern"));
    }

    void emptyPolylineExport()
    {
        RS_FilterDXFRW filter;
        RS_Polyline polyline;
        filter.writePolyline(&polyline);
        filter.writeLWPolyline(&polyline);
    }

    void polylineRoundTrip_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<bool>("closed");
        QTest::newRow("r12-open") << int(RS2::FormatDXFRW12) << false;
        QTest::newRow("r12-closed") << int(RS2::FormatDXFRW12) << true;
        QTest::newRow("modern-open") << int(RS2::FormatDXFRW) << false;
        QTest::newRow("modern-closed") << int(RS2::FormatDXFRW) << true;
    }

    void polylineRoundTrip()
    {
        QFETCH(int, format);
        QFETCH(bool, closed);
        RS_Graphic source;
        source.newDoc();
        auto* polyline = new RS_Polyline(&source);
        polyline->addVertex({0, 0}, 0.5);
        polyline->addVertex({10, 0});
        polyline->addVertex({10, 10});
        polyline->setClosed(closed, 0.0);
        polyline->setLayer(source.getActiveLayer());
        source.addEntity(polyline);
        const QString fileName = directory.filePath("polyline.dxf");
        QVERIFY(RS_FileIO::instance()->fileExport(source, fileName, RS2::FormatType(format)));
        RS_Graphic result;
        QVERIFY(RS_FileIO::instance()->fileImport(result, fileName));
        QCOMPARE(result.count(), 1u);
        RS_Entity* entity = result.entityAt(0);
        QCOMPARE(entity->rtti(), RS2::EntityPolyline);
        auto* restored = static_cast<RS_Polyline*>(entity);
        QCOMPARE(restored->isClosed(), closed);
        QCOMPARE(restored->count(), closed ? 3u : 2u);
        QCOMPARE(restored->entityAt(0)->rtti(), RS2::EntityArc);
        QVERIFY(restored->getStartpoint().distanceTo(polyline->getStartpoint()) < RS_TOLERANCE);
        QVERIFY(restored->getEndpoint().distanceTo(polyline->getEndpoint()) < RS_TOLERANCE);
    }

    void rectangleWidthResetsFourthCorner()
    {
        RS_Graphic graphic;
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&graphic);
        RectangleAction action(graphic, view);
        const RS_Vector first(3, 4);
        action.doInitialSnapToRelativeZero(first);
        action.setStatus(RectangleAction::SetPoint2);
        std::unique_ptr<RS_Polyline> base(action.createPolyline({13, 4}));
        action.setStatus(RectangleAction::SetPoint3);
        std::unique_ptr<RS_Polyline> rectangle(action.createPolyline({13, 9}));
        action.setEndZeroPointCorner(RectangleAction::SNAP_CORNER4);
        QVERIFY(action.doGetRelativeZeroAfterTrigger().distanceTo(first) > 1);
        action.setStatus(RectangleAction::SetWidth);
        bool toMainStatus = true;
        action.processCommandValue(20, toMainStatus);
        QVERIFY(action.doGetRelativeZeroAfterTrigger().distanceTo(first) < RS_TOLERANCE);
        QVERIFY(!toMainStatus);
        QCOMPARE(action.getStatus(), int(RectangleAction::SetHeight));
    }

    void lineJoinPreviewOwnership_data()
    {
        QTest::addColumn<unsigned>("createdCount");
        QTest::newRow("cancel") << 0u;
        QTest::newRow("join-lines") << 2u;
        QTest::newRow("join-polyline") << 1u;
    }

    void lineJoinPreviewOwnership()
    {
        QFETCH(unsigned, createdCount);
        RS_Graphic graphic;
        graphic.newDoc();
        graphic.addEntity(new RS_Line(&graphic, RS_LineData({0, 0}, {20, 0})));
        graphic.addEntity(new RS_Line(&graphic, RS_LineData({10, 5}, {10, 15})));
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&graphic);
        LineJoinAction action(graphic, view);
        action.init(LineJoinAction::SetLine1);
        action.setCreatePolyline(createdCount == 1);
        action.setRemoveOriginalLines(true);
        auto click = [&view](const RS_Vector& point) {
            const RS_Vector position = view.toGui(point);
            return QMouseEvent(QEvent::MouseButtonRelease, QPointF(position.x, position.y),
                               Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        };
        auto firstClick = click({5, 0});
        action.doOnLeftMouseButtonRelease(&firstClick, action.getStatus(), {5, 0});
        QCOMPARE(action.getStatus(), int(LineJoinAction::SetLine2));
        auto secondClick = click({10, 10});
        RS_Vector snap(10, 10);
        QList<RS_Entity*> previews;
        action.doPreparePreviewEntities(&secondClick, snap, previews, action.getStatus());
        QVERIFY(!previews.isEmpty());
        std::unique_ptr<RS_Entity> preview(previews.takeFirst());
        qDeleteAll(previews);
        QCOMPARE(preview->rtti(), RS2::EntityPolyline);
        QCOMPARE(static_cast<RS_Polyline*>(preview.get())->count(), 2u);
        action.doOnLeftMouseButtonRelease(&secondClick, action.getStatus(), snap);
        QCOMPARE(action.getStatus(), int(LineJoinAction::ResolveFirstLineTrim));
        if (createdCount == 0) {
            action.init(LineJoinAction::SetLine1);
        } else {
            action.doOnLeftMouseButtonRelease(&firstClick, action.getStatus(), {5, 0});
        }
        QCOMPARE(action.getStatus(), int(LineJoinAction::SetLine1));
        QCOMPARE(static_cast<RS_Polyline*>(preview.get())->count(), 2u);
        QCOMPARE(graphic.count(), 2u + createdCount);
        for (unsigned i = 0; i < graphic.count(); ++i) {
            QCOMPARE(graphic.entityAt(i)->isUndone(), createdCount != 0 && i < 2);
        }
        if (createdCount == 0) return;
        QCOMPARE(graphic.entityAt(2)->rtti(), createdCount == 1 ? RS2::EntityPolyline : RS2::EntityLine);
        QVERIFY(graphic.undo());
        for (unsigned i = 0; i < graphic.count(); ++i) {
            QCOMPARE(graphic.entityAt(i)->isUndone(), i >= 2);
        }
        QVERIFY(graphic.redo());
        for (unsigned i = 0; i < graphic.count(); ++i) {
            QCOMPARE(graphic.entityAt(i)->isUndone(), i < 2);
        }
    }

    void dwgSectionDefaults()
    {
        dwgSectionInfo section;
        QCOMPARE(section.size, duint64{0});
        QCOMPARE(section.maxSize, duint64{0});
        QCOMPARE(section.address, duint64{0});
        const dwgPageInfo page(1, 2, 3);
        QCOMPARE(page.Id, duint64{1});
        QCOMPARE(page.address, duint64{2});
        QCOMPARE(page.size, duint64{3});
        QCOMPARE(page.dataSize, duint64{0});
        QCOMPARE(page.startOffset, duint64{0});
        QCOMPARE(page.cSize, duint64{0});
        QCOMPARE(page.uSize, duint64{0});
    }

    void picBoxFieldCount_data()
    {
        QTest::addColumn<QByteArray>("record");
        QTest::addColumn<unsigned>("lineCount");
        QTest::newRow("truncated") << QByteArray("box invis fill 0.000 with .sw at (1,2) width 3 height\n") << 0u;
        QTest::newRow("complete") << QByteArray("box invis fill 0.000 with .sw at (1,2) width 3 height 4\n") << 4u;
    }

    void picBoxFieldCount()
    {
        QFETCH(QByteArray, record);
        QFETCH(unsigned, lineCount);
        QFile file(directory.filePath("box.pic"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(record), qint64(record.size()));
        file.close();
        RS_Graphic graphic;
        graphic.newDoc();
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&graphic);
        Doc_plugin_interface document(&graphic, &view, nullptr);
        picPunto dialog;
        const auto edits = dialog.findChildren<QLineEdit*>();
        QCOMPARE(edits.size(), 2);
        edits.at(0)->setText(file.fileName());
        edits.at(1)->setText("1");
        QTimer dismissMessage;
        connect(&dismissMessage, &QTimer::timeout, [] {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                if (auto* message = qobject_cast<QMessageBox*>(widget)) message->accept();
            }
        });
        dismissMessage.start(10);
        dialog.processFile(&document);
        QCOMPARE(graphic.count(), lineCount);
    }
};

QTEST_MAIN(LC_StaticAnalysisTests)
#include "static_analysis_tests.moc"
