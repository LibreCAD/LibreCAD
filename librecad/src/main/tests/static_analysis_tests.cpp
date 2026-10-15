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
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_insert.h"
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

    void importBlock_data()
    {
        QTest::addColumn<QString>("innerName");
        QTest::addColumn<bool>("reserveSuffix");
        QTest::addColumn<QString>("destinationName");
        QTest::newRow("plain") << QString() << false << QString();
        QTest::newRow("nested") << QString("detail") << false << QString();
        QTest::newRow("filename-collision") << QString("part") << false << QString();
        QTest::newRow("suffix-collision") << QString("part") << true << QString();
        QTest::newRow("destination-filename") << QString("part") << false << QString("part");
        QTest::newRow("destination-suffix") << QString("part") << false << QString("part-0");
        QTest::newRow("both-suffixes") << QString("part") << true << QString("part-1");
        QTest::newRow("destination-inner") << QString("detail") << false << QString("detail");
        QTest::newRow("destination-ancestor") << QString("detail") << false << QString("assembly");
    }

    void importBlock()
    {
        QFETCH(QString, innerName);
        QFETCH(bool, reserveSuffix);
        QFETCH(QString, destinationName);
        RS_Graphic source;
        source.newDoc();
        source.setUnit(RS2::Millimeter);
        RS_EntityContainer* parent = &source;
        if (!innerName.isEmpty()) {
            auto* block = new RS_Block(&source, RS_BlockData(innerName, {0, 0}, false));
            source.addBlock(block);
            parent = block;
        }
        auto* line = new RS_Line(parent, RS_LineData({1, 2}, {11, 2}));
        line->setLayer(source.findLayer("0"));
        parent->addEntity(line);
        if (!innerName.isEmpty()) {
            auto* assembly = new RS_Block(&source, RS_BlockData("assembly", {0, 0}, false));
            auto* child = new RS_Insert(assembly, RS_InsertData(innerName, {0, 0}, {1, 1}, 0, 1, 1, {0, 0}));
            child->setLayer(source.findLayer("0"));
            assembly->addEntity(child);
            source.addBlock(assembly);
            child->update();
            auto* insert = new RS_Insert(&source, RS_InsertData("assembly", {3, 4}, {1, 1}, 0, 1, 1, {0, 0}));
            insert->setLayer(source.findLayer("0"));
            source.addEntity(insert);
            insert->update();
        }
        if (reserveSuffix) {
            source.addBlock(new RS_Block(&source, RS_BlockData("part-0", {0, 0}, false)));
        }
        const QString path = directory.filePath("part.dxf");
        QVERIFY(source.saveAs(path, RS2::FormatDXFRW, true));

        RS_Graphic target;
        target.newDoc();
        target.setUnit(RS2::Millimeter);
        RS_Block* existing = nullptr;
        if (!destinationName.isEmpty()) {
            existing = new RS_Block(&target, RS_BlockData(destinationName, {0, 0}, false));
            existing->addEntity(new RS_Line(existing, RS_LineData({500, 500}, {600, 500})));
            target.addBlock(existing);
        }
        RS_LibraryInsertData data{path, {100, 200}, 2.0, M_PI / 2.0};
        RS_Creation creation(&target);
        creation.createLibraryInsert(data);
        QCOMPARE(target.count(), 1u);
        QCOMPARE(target.entityAt(0)->rtti(), RS2::EntityInsert);
        auto* imported = static_cast<RS_Insert*>(target.entityAt(0));
        imported->update();
        QCOMPARE(imported->count(), 1u);
        RS_Entity* geometry = imported->entityAt(0);
        unsigned depth = 0;
        while (geometry->rtti() == RS2::EntityInsert) {
            auto* nested = static_cast<RS_Insert*>(geometry);
            QCOMPARE(nested->count(), 1u);
            geometry = nested->entityAt(0);
            ++depth;
        }
        QCOMPARE(depth, innerName.isEmpty() ? 0u : 2u);
        QCOMPARE(geometry->rtti(), RS2::EntityLine);
        const RS_Vector start = innerName.isEmpty() ? RS_Vector(96, 202) : RS_Vector(88, 208);
        QVERIFY(geometry->getStartpoint().distanceTo(start) < 1e-9);
        QVERIFY(geometry->getEndpoint().distanceTo(start + RS_Vector(0, 20)) < 1e-9);

        const unsigned blockCount = target.countBlocks();
        creation.createLibraryInsert(data);
        QCOMPARE(target.count(), 2u);
        QCOMPARE(target.countBlocks(), blockCount);
        auto* repeated = static_cast<RS_Insert*>(target.entityAt(1));
        QCOMPARE(repeated->getName(), imported->getName());
        QCOMPARE(repeated->count(), 1u);
        QVERIFY(target.undo());
        QVERIFY(repeated->isUndone());
        QVERIFY(!imported->isUndone());
        QVERIFY(target.redo());
        QVERIFY(!repeated->isUndone());
        repeated->update();
        QCOMPARE(repeated->count(), 1u);

        const QString savedPath = directory.filePath("imported.dxf");
        QVERIFY(target.saveAs(savedPath, RS2::FormatDXFRW, true));
        QFile savedFile(savedPath);
        QVERIFY(savedFile.open(QIODevice::ReadOnly));
        QVERIFY(!savedFile.readAll().contains("LibreCAD.LibraryInsertSource"));
        savedFile.close();
        RS_Graphic restored;
        QVERIFY(restored.open(savedPath, RS2::FormatUnknown));
        QCOMPARE(restored.count(), 2u);
        const unsigned savedBlockCount = restored.countBlocks();
        RS_Creation restoredCreation(&restored);
        restoredCreation.createLibraryInsert(data);
        QCOMPARE(restored.count(), 3u);
        QVERIFY(restored.countBlocks() > savedBlockCount);
        const unsigned restoredBlockCount = restored.countBlocks();
        restoredCreation.createLibraryInsert(data);
        QCOMPARE(restored.count(), 4u);
        QCOMPARE(restored.countBlocks(), restoredBlockCount);
        QCOMPARE(static_cast<RS_Insert*>(restored.entityAt(2))->getName(),
                 static_cast<RS_Insert*>(restored.entityAt(3))->getName());
        for (RS_Entity* entity : restored) {
            QCOMPARE(entity->rtti(), RS2::EntityInsert);
            auto* insert = static_cast<RS_Insert*>(entity);
            insert->update();
            QCOMPARE(insert->count(), 1u);
            RS_Entity* savedGeometry = insert->entityAt(0);
            while (savedGeometry->rtti() == RS2::EntityInsert) {
                auto* nested = static_cast<RS_Insert*>(savedGeometry);
                QCOMPARE(nested->count(), 1u);
                savedGeometry = nested->entityAt(0);
            }
            QCOMPARE(savedGeometry->rtti(), RS2::EntityLine);
            QVERIFY(savedGeometry->getStartpoint().distanceTo(start) < 1e-9);
            QVERIFY(savedGeometry->getEndpoint().distanceTo(start + RS_Vector(0, 20)) < 1e-9);
        }
        if (existing) {
            QVERIFY(existing->entityAt(0)->getStartpoint().distanceTo({500, 500}) < 1e-9);
            QVERIFY(existing->entityAt(0)->getEndpoint().distanceTo({600, 500}) < 1e-9);
        }
        RS_Creation(imported->getBlockForInsert()).createLibraryInsert(data);
        imported->update();
        QCOMPARE(imported->count(), 2u);
        QCOMPARE(imported->entityAt(1)->rtti(), RS2::EntityInsert);
        auto* edited = static_cast<RS_Insert*>(imported->entityAt(1));
        QCOMPARE(edited->count(), 1u);
    }

    void importDifferentLibraries_data()
    {
        QTest::addColumn<bool>("reverse");
        QTest::addColumn<bool>("reopen");
        QTest::addColumn<bool>("sameBasename");
        QTest::newRow("suffix-after") << false << false << false;
        QTest::newRow("suffix-before") << true << false << false;
        QTest::newRow("suffix-after-reopen") << false << true << false;
        QTest::newRow("suffix-before-reopen") << true << true << false;
        QTest::newRow("same-basename") << false << false << true;
        QTest::newRow("same-basename-reopen") << false << true << true;
    }

    void importDifferentLibraries()
    {
        QFETCH(bool, reverse);
        QFETCH(bool, reopen);
        QFETCH(bool, sameBasename);
        const QString nestedPath = directory.filePath("part.dxf");
        QVERIFY(QDir().mkpath(directory.filePath("other")));
        const QString plainPath = directory.filePath(sameBasename ? "other/part.dxf" : "part-0.dxf");
        RS_Graphic nested;
        nested.newDoc();
        nested.setUnit(RS2::Millimeter);
        auto* block = new RS_Block(&nested, RS_BlockData("part", {0, 0}, false));
        auto* line = new RS_Line(block, RS_LineData({1, 2}, {11, 2}));
        line->setLayer(nested.findLayer("0"));
        block->addEntity(line);
        nested.addBlock(block);
        auto* reference = new RS_Insert(&nested, RS_InsertData("part", {3, 4}, {1, 1}, 0, 1, 1, {0, 0}));
        reference->setLayer(nested.findLayer("0"));
        nested.addEntity(reference);
        reference->update();
        QVERIFY(nested.saveAs(nestedPath, RS2::FormatDXFRW, true));
        RS_Graphic plain;
        plain.newDoc();
        plain.setUnit(RS2::Millimeter);
        auto* plainLine = new RS_Line(&plain, RS_LineData({500, 500}, {600, 500}));
        plainLine->setLayer(plain.findLayer("0"));
        plain.addEntity(plainLine);
        QVERIFY(plain.saveAs(plainPath, RS2::FormatDXFRW, true));

        RS_Graphic target;
        target.newDoc();
        target.setUnit(RS2::Millimeter);
        RS_LibraryInsertData first{reverse ? plainPath : nestedPath, {0, 0}, 1.0, 0.0};
        RS_Creation(&target).createLibraryInsert(first);
        if (reopen) {
            const QString savedPath = directory.filePath("combined.dxf");
            QVERIFY(target.saveAs(savedPath, RS2::FormatDXFRW, true));
            QVERIFY(target.open(savedPath, RS2::FormatUnknown));
        }
        RS_LibraryInsertData second{reverse ? nestedPath : plainPath, {0, 0}, 1.0, 0.0};
        RS_Creation creation(&target);
        creation.createLibraryInsert(second);
        QCOMPARE(target.count(), 2u);
        for (unsigned i = 0; i < target.count(); ++i) {
            QCOMPARE(target.entityAt(i)->rtti(), RS2::EntityInsert);
            auto* inserted = static_cast<RS_Insert*>(target.entityAt(i));
            inserted->update();
            QCOMPARE(inserted->count(), 1u);
            RS_Entity* geometry = inserted->entityAt(0);
            const bool isNested = (i == 0) != reverse;
            if (isNested) {
                QCOMPARE(geometry->rtti(), RS2::EntityInsert);
                auto* child = static_cast<RS_Insert*>(geometry);
                QCOMPARE(child->count(), 1u);
                geometry = child->entityAt(0);
            }
            QCOMPARE(geometry->rtti(), RS2::EntityLine);
            const RS_Vector start = isNested ? RS_Vector(4, 6) : RS_Vector(500, 500);
            QVERIFY(geometry->getStartpoint().distanceTo(start) < 1e-9);
            QVERIFY(geometry->getEndpoint().distanceTo(start + RS_Vector(isNested ? 10 : 100, 0)) < 1e-9);
        }
        const unsigned blockCount = target.countBlocks();
        creation.createLibraryInsert(second);
        QCOMPARE(target.countBlocks(), blockCount);
        QCOMPARE(target.count(), 3u);
        auto* previous = static_cast<RS_Insert*>(target.entityAt(1));
        auto* repeated = static_cast<RS_Insert*>(target.entityAt(2));
        QCOMPARE(repeated->getName(), previous->getName());
        QCOMPARE(repeated->count(), 1u);
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
