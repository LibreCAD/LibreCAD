// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

#include <memory>

#include <QScopeGuard>
#include <QtTest>
#include <QtWidgets>

#include "doc_plugin_interface.h"
#include "dwgreader.h"
#include "lc_actiondrawrectangle3points.h"
#include "lc_actionmodifylinejoin.h"
#include "lc_hyperbola.h"
#include "lc_layertreemodel.h"
#include "lc_layertreeview.h"
#include "lc_layertreewidget.h"
#include "lc_widgetoptionsdialog.h"
#include "picfile.h"
#include "qc_applicationwindow.h"
#include "qg_dlghatch.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_layerwidget.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_commands.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
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

bool saveLibrary(const QString& path, const RS_LineData& data,
                 const QStringList& blocks = {}, bool reserveSuffix = false)
{
    RS_Graphic source;
    source.newDoc();
    source.setUnit(RS2::Millimeter);
    for (const QString& name : blocks) {
        source.addBlock(new RS_Block(&source, RS_BlockData(name, {0, 0}, false)));
    }
    RS_EntityContainer* parent = blocks.isEmpty() ? static_cast<RS_EntityContainer*>(&source)
                                                 : source.findBlock(blocks.first());
    auto* line = new RS_Line(parent, data);
    line->setLayer(source.findLayer("0"));
    parent->addEntity(line);
    for (int i = 0; i < blocks.size(); ++i) {
        RS_EntityContainer* owner = i + 1 < blocks.size()
            ? source.findBlock(blocks.at(i + 1)) : static_cast<RS_EntityContainer*>(&source);
        const RS_Vector point = owner == &source ? RS_Vector(3, 4) : RS_Vector(0, 0);
        auto* insert = new RS_Insert(owner, RS_InsertData(blocks.at(i), point, {1, 1}, 0, 1, 1, {0, 0}));
        insert->setLayer(source.findLayer("0"));
        owner->addEntity(insert);
        insert->update();
    }
    if (reserveSuffix) {
        source.addBlock(new RS_Block(&source, RS_BlockData("part-0", {0, 0}, false)));
    }
    return source.saveAs(path, RS2::FormatDXFRW, true);
}

const RS_Line* libraryLine(RS_Insert* insert, unsigned depth)
{
    insert->update();
    RS_Entity* entity = insert;
    for (unsigned i = 0; i <= depth; ++i) {
        if (!entity || entity->rtti() != RS2::EntityInsert || static_cast<RS_Insert*>(entity)->count() != 1u) {
            return nullptr;
        }
        entity = static_cast<RS_Insert*>(entity)->entityAt(0);
    }
    return entity && entity->rtti() == RS2::EntityLine ? static_cast<RS_Line*>(entity) : nullptr;
}

// The unit of a new drawing and the paper size remembered for it, pinned for a
// test and put back afterwards.
class PaperDefaults {
public:
    explicit PaperDefaults(const QString& unit)
        : saved{entry("/Defaults", "/Unit"), entry("/Print", "/PaperSizeX"), entry("/Print", "/PaperSizeY")}
    {
        set(unit, "210", "297");
    }
    ~PaperDefaults() { set(saved[0], saved[1], saved[2]); }

    static QString entry(const QString& group, const QString& key)
    {
        auto guard = RS_SETTINGS->beginGroupGuard(group);
        return RS_SETTINGS->readEntry(key);
    }

private:
    static void set(const QString& unit, const QString& width, const QString& height)
    {
        {
            auto guard = RS_SETTINGS->beginGroupGuard("/Defaults");
            RS_SETTINGS->writeEntry("/Unit", unit);
        }
        auto guard = RS_SETTINGS->beginGroupGuard("/Print");
        RS_SETTINGS->writeEntry("/PaperSizeX", width);
        RS_SETTINGS->writeEntry("/PaperSizeY", height);
    }

    const QStringList saved;
};

} // namespace

class LC_StaticAnalysisTests : public QObject {
    Q_OBJECT

    QTemporaryDir directory;

    // Widget Options is modal: the dialog is changed from a timer, then OK or Cancel is pressed.
    template <typename Change>
    void widgetOptions(const Change& change, const bool ok = true)
    {
        QTimer answer;
        connect(&answer, &QTimer::timeout, [&] {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                auto* dialog = qobject_cast<LC_WidgetOptionsDialog*>(widget);
                if (dialog == nullptr || !dialog->isVisible()) continue;
                answer.stop();
                change(dialog);
                if (ok) dialog->accept();
                else dialog->reject();
            }
        });
        answer.start(10);
        QC_ApplicationWindow::getAppWindow()->widgetOptionsDialog();
    }

    QString writeStyleSheet(const QString& name, const QByteArray& content)
    {
        const QString file = directory.filePath(name);
        QFile sheet(file);
        if (!sheet.open(QIODevice::WriteOnly) || sheet.write(content) != content.size()) return QString();
        return file;
    }

    static void clearStyleSheetSettings()
    {
        QSettings settings;
        settings.remove("Widgets/StyleSheet");
        settings.remove("Widgets/AllowStyleSheet");
        qApp->setStyleSheet(QString());
    }

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

    void lineGapCommands()
    {
        QCOMPARE(RS_COMMANDS->cmdToAction("gapline", false), RS2::ActionModifyLineGap);
        QCOMPARE(RS_COMMANDS->cmdToAction("gl", false), RS2::ActionModifyLineGap);
        QCOMPARE(RS_COMMANDS->keycodeToAction("gl"), RS2::ActionModifyLineGap);
        QCOMPARE(RS_COMMANDS->cmdToAction("breakdivide", false), RS2::ActionModifyBreakDivide);
        QCOMPARE(RS_COMMANDS->cmdToAction("bd", false), RS2::ActionModifyBreakDivide);
        QCOMPARE(RS_COMMANDS->keycodeToAction("bd"), RS2::ActionModifyBreakDivide);
    }

    void lineGapSavedAlias()
    {
        QFile aliases(RS_SYSTEM->getAppDataDir() + "/librecad.alias");
        QVERIFY(aliases.open(QIODevice::ReadOnly));
        const QByteArray original = aliases.readAll();
        aliases.close();
        const auto writeAlias = [&aliases](const QByteArray& contents) {
            if (!aliases.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
            const bool written = aliases.write(contents) == contents.size();
            aliases.close();
            RS_COMMANDS->updateAlias();
            return written;
        };
        const auto restore = qScopeGuard([&] { writeAlias(original); });

        // Saved aliases remain authoritative, including defaults from older builds.
        QVERIFY(writeAlias("gl\tbreakdivide\n"));
        QCOMPARE(RS_COMMANDS->cmdToAction("gapline", false), RS2::ActionModifyLineGap);
        QCOMPARE(RS_COMMANDS->cmdToAction("gl", false), RS2::ActionModifyBreakDivide);
        QCOMPARE(RS_COMMANDS->keycodeToAction("gl"), RS2::ActionModifyBreakDivide);

        QVERIFY(writeAlias("gl\tgapline\n"));
        lineGapCommands();
    }

    void hyperbolaIntersections_data()
    {
        QTest::addColumn<bool>("firstBranch");
        QTest::addColumn<bool>("secondBranch");
        QTest::addColumn<double>("rotation");
        QTest::addColumn<double>("extent");
        for (bool first : {false, true}) {
            for (bool second : {false, true}) {
                for (double rotation : {0., 0.7}) {
                    for (double extent : {0., 0.5, 2.}) {
                        const QByteArray name = QString("%1-%2-%3-%4")
                            .arg(first).arg(second).arg(rotation).arg(extent).toLatin1();
                        QTest::newRow(name.constData()) << first << second << rotation << extent;
                    }
                }
            }
        }
    }

    void hyperbolaIntersections()
    {
        QFETCH(bool, firstBranch);
        QFETCH(bool, secondBranch);
        QFETCH(double, rotation);
        QFETCH(double, extent);
        const RS_Vector center(12, -8);
        const RS_Vector axis = RS_Vector(1, 0).rotate(rotation);
        RS_Layer construction("construction");
        construction.setConstruction(true);
        LC_Hyperbola first(nullptr, {center, axis, 1., -extent, extent, firstBranch});
        LC_Hyperbola second(nullptr, {center + axis * 3., axis, 1., -extent, extent, secondBranch});
        const bool branchesMeet = !firstBranch && secondBranch;
        for (bool swap : {false, true}) {
            const RS_Entity* a = swap ? &second : &first;
            const RS_Entity* b = swap ? &first : &second;
            const auto extended = RS_Information::getIntersection(a, b, false);
            QCOMPARE(extended.size(), branchesMeet ? 2u : 0u);
            for (const RS_Vector& point : extended) {
                const RS_Vector local = (point - center).rotate(-rotation);
                QVERIFY(std::abs(local.x - 1.5) < 1e-7);
                QVERIFY(std::abs(std::abs(local.y) - std::sqrt(1.25)) < 1e-7);
            }
            QCOMPARE(RS_Information::getIntersection(a, b, true).size(),
                     branchesMeet && extent != 0.5 ? 2u : 0u);
            first.setLayer(&construction);
            second.setLayer(&construction);
            QCOMPARE(RS_Information::getIntersection(a, b, true).size(), branchesMeet ? 2u : 0u);
            first.setLayer(nullptr);
            second.setLayer(nullptr);
        }
    }

    void hyperbolaLineCircleIntersections()
    {
        RS_Line line(nullptr, {{-3, 1}, {3, 1}});
        RS_Circle circle(nullptr, {{0, 0}, std::sqrt(3.)});
        const RS_Entity* others[] = {&line, &circle};
        for (bool branch : {false, true}) {
            LC_Hyperbola hyperbola(nullptr, {{0, 0}, {1, 0}, 1., -0.5, 0.5, branch});
            for (const RS_Entity* other : others) {
                for (bool swap : {false, true}) {
                    const auto points = RS_Information::getIntersection(
                        swap ? other : &hyperbola, swap ? &hyperbola : other, false);
                    QCOMPARE(points.size(), other == &line ? 1u : 2u);
                    for (const RS_Vector& point : points) {
                        QVERIFY(std::abs(point.x - (branch ? -1. : 1.) * std::sqrt(2.)) < 1e-7);
                    }
                    QVERIFY(RS_Information::getIntersection(&hyperbola, other, true).empty());
                }
            }
        }
        QCOMPARE(RS_Information::getIntersection(&line, &circle, true).size(), 2u);
    }

    void trimHyperbola_data()
    {
        QTest::addColumn<bool>("firstBranch");
        QTest::addColumn<bool>("secondBranch");
        QTest::addColumn<bool>("descending");
        QTest::addColumn<bool>("both");
        QTest::addColumn<double>("rotation");
        QTest::addColumn<double>("extent");
        for (bool first : {false, true}) {
            for (bool second : {false, true}) {
                for (bool descending : {false, true}) {
                    for (bool both : {false, true}) {
                        for (double rotation : {0., 0.7}) {
                            for (double extent : {0.5, 2.}) {
                                const QByteArray name = QString("%1-%2-%3-%4-%5-%6")
                                    .arg(first).arg(second).arg(descending).arg(both)
                                    .arg(rotation).arg(extent).toLatin1();
                                QTest::newRow(name.constData())
                                    << first << second << descending << both << rotation << extent;
                            }
                        }
                    }
                }
            }
        }
    }

    void trimHyperbola()
    {
        QFETCH(bool, firstBranch);
        QFETCH(bool, secondBranch);
        QFETCH(bool, descending);
        QFETCH(bool, both);
        QFETCH(double, rotation);
        QFETCH(double, extent);
        RS_Graphic graphic;
        graphic.newDoc();
        const double xSign = firstBranch ? -1. : 1.;
        const double ySign = secondBranch ? -1. : 1.;
        const double start = descending ? extent : -extent;
        const double end = -start;
        const RS_Vector center(12, -8);
        const RS_Vector axis = RS_Vector(1, 0).rotate(rotation);
        const RS_Vector normal = RS_Vector(0, 1).rotate(rotation);
        auto* first = new LC_Hyperbola(&graphic, {center, axis, 1., start, end, firstBranch});
        auto* second = new LC_Hyperbola(&graphic, {center + axis * (3. * xSign), normal,
                                                 1., start, end, secondBranch});
        graphic.addEntity(first);
        graphic.addEntity(second);
        const RS_Vector intersection = center + axis * (xSign * 11. / 6.)
                                      + normal * (ySign * std::sqrt(85.) / 6.);
        const RS_Vector click = first->getPoint(-ySign * extent * 0.75, firstBranch);
        std::unique_ptr<LC_Hyperbola> preview(static_cast<LC_Hyperbola*>(first->clone()));
        QVERIFY(preview->prepareTrim(click, RS_Information::getIntersection(first, second, false))
                    .distanceTo(intersection) < 1e-7);
        RS_Modification modification(graphic);
        QVERIFY(modification.trim(click, first,
                                  second->getPoint(0., secondBranch), second, both));
        QCOMPARE(graphic.count(), both ? 4u : 3u);
        auto* trimmed = static_cast<LC_Hyperbola*>(graphic.entityAt(2));
        QVERIFY(trimmed->getStartpoint().distanceTo(preview->getStartpoint()) < 1e-7);
        QVERIFY(trimmed->getEndpoint().distanceTo(preview->getEndpoint()) < 1e-7);
        QCOMPARE(trimmed->isReversed(), firstBranch);
        const bool cutStart = (ySign < 0.) != descending;
        QVERIFY((cutStart ? trimmed->getStartpoint() : trimmed->getEndpoint())
                    .distanceTo(intersection) < 1e-7);
        QVERIFY((cutStart ? trimmed->getEndpoint() : trimmed->getStartpoint())
                    .distanceTo(cutStart ? first->getEndpoint() : first->getStartpoint()) < 1e-7);
        if (both) {
            auto* limiter = static_cast<LC_Hyperbola*>(graphic.entityAt(3));
            QCOMPARE(limiter->isReversed(), secondBranch);
            const bool limitCutStart = (xSign < 0.) != descending;
            QVERIFY((limitCutStart ? limiter->getStartpoint() : limiter->getEndpoint())
                        .distanceTo(intersection) < 1e-7);
        }
        QVERIFY(first->isUndone());
        QCOMPARE(second->isUndone(), both);
        QVERIFY(graphic.undo());
        QVERIFY(!first->isUndone());
        QVERIFY(!second->isUndone());
        QVERIFY(trimmed->isUndone());
        QVERIFY(graphic.redo());
        QVERIFY(first->isUndone());
        QVERIFY(!trimmed->isUndone());
    }

    void trimDisjointHyperbolaBranches()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        auto* first = new LC_Hyperbola(&graphic, {{0, 0}, {1, 0}, 1., -2., 2.});
        auto* second = new LC_Hyperbola(&graphic, {{3, 0}, {1, 0}, 1., -2., 2.});
        graphic.addEntity(first);
        graphic.addEntity(second);
        RS_Modification modification(graphic);
        for (bool both : {false, true}) {
            QVERIFY(!modification.trim(first->getStartpoint(), first, second->getStartpoint(), second, both));
            QCOMPARE(graphic.count(), 2u);
            QVERIFY(!first->isUndone());
            QVERIFY(!second->isUndone());
        }
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
        const QString path = directory.filePath("part.dxf");
        const QStringList blocks = innerName.isEmpty() ? QStringList() : QStringList{innerName, "assembly"};
        QVERIFY(saveLibrary(path, {{1, 2}, {11, 2}}, blocks, reserveSuffix));

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
        const RS_Line* geometry = libraryLine(imported, blocks.size());
        QVERIFY(geometry);
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
            const RS_Line* savedGeometry = libraryLine(static_cast<RS_Insert*>(entity), blocks.size());
            QVERIFY(savedGeometry);
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
        QVERIFY(saveLibrary(nestedPath, {{1, 2}, {11, 2}}, {"part"}));
        QVERIFY(saveLibrary(plainPath, {{500, 500}, {600, 500}}));

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
            const bool isNested = (i == 0) != reverse;
            const RS_Line* geometry = libraryLine(static_cast<RS_Insert*>(target.entityAt(i)), isNested ? 1 : 0);
            QVERIFY(geometry);
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

    // Issue #3196: going back from "Select part of first line that should remain
    // after trim" ran init(), which forgot line 1, and the next mouse move
    // highlighted it: a null pointer.
    void lineJoinBackKeepsFirstLine()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        graphic.addEntity(new RS_Line(&graphic, RS_LineData({0, 0}, {100, 0})));
        graphic.addEntity(new RS_Line(&graphic, RS_LineData({50, 20}, {50, 60})));
        RS_StaticGraphicView view(200, 200, nullptr);
        view.setContainer(&graphic);
        LineJoinAction action(graphic, view);
        action.init(LineJoinAction::SetLine1);
        auto send = [&view, &action](QEvent::Type type, Qt::MouseButton button, const RS_Vector& point) {
            const RS_Vector position = view.toGui(point);
            QMouseEvent event(type, QPointF(position.x, position.y), button, Qt::NoButton, Qt::NoModifier);
            if (type == QEvent::MouseMove) action.mouseMoveEvent(&event);
            else action.mouseReleaseEvent(&event);
        };
        send(QEvent::MouseButtonRelease, Qt::LeftButton, {30, 0});
        send(QEvent::MouseButtonRelease, Qt::LeftButton, {50, 40});
        QCOMPARE(action.getStatus(), int(LineJoinAction::ResolveFirstLineTrim));
        send(QEvent::MouseButtonRelease, Qt::RightButton, {50, 40});
        QCOMPARE(action.getStatus(), int(LineJoinAction::SetLine2));
        send(QEvent::MouseMove, Qt::NoButton, {80, 30});
        // line 1 is still chosen
        send(QEvent::MouseButtonRelease, Qt::LeftButton, {50, 40});
        QCOMPARE(action.getStatus(), int(LineJoinAction::ResolveFirstLineTrim));
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

    // Issue #3187: a drawing's unit gives its paper new numbers, not a paper to
    // remember for new drawings. Read in the old unit, they changed the
    // remembered size with every new drawing in inches, and with every copy
    // between drawings of different units, which sets the clipboard's unit.
    void unitKeepsRememberedPaper()
    {
        const PaperDefaults defaults("Inch");
        RS_Graphic inches;
        RS_Graphic feet;
        feet.setUnit(RS2::Foot);
        for (RS_Graphic* source : {&feet, &inches}) {
            source->newDoc();
            auto* line = new RS_Line(source, RS_LineData({0, 0}, {10, 0}));
            source->addEntity(line);
            line->setSelected(true);
            RS_Modification(*source).copy({0, 0}, false);
        }
        RS_Graphic next;
        QCOMPARE(next.getPaperSize().x, 210.0 / 25.4);
        QCOMPARE(next.getPaperSize().y, 297.0 / 25.4);
        QCOMPARE(PaperDefaults::entry("/Print", "/PaperSizeX").toDouble(), 210.0);
    }

    // Issue #3187: when the unit changed on the Units tab of Drawing Preferences,
    // the Paper tab kept a custom size and the margins in the old unit, which OK
    // took in the new one, and a paper format chosen reached the drawing at once,
    // even if the dialog was cancelled.
    void paperTabFollowsUnit()
    {
        const PaperDefaults defaults("Inch");
        enum Step { Cancel, Keep, Type };
        for (const Step step : {Cancel, Keep, Type}) {
            // a drawing in inches on a custom 30 x 15 in paper, margins of 10 mm
            RS_Graphic graphic;
            graphic.setPaperSize(RS_Vector(30.0, 15.0));
            graphic.setMargins(10.0, 10.0, 10.0, 10.0);
            QG_DlgOptionsDrawing dialog;
            dialog.setGraphic(&graphic);
            const auto choose = [&dialog](const char* name, const int index) {
                auto* box = dialog.findChild<QComboBox*>(name);
                box->setCurrentIndex(index);
                emit box->activated(index);
            };
            choose("cbUnit", RS2::Foot);
            auto* buttons = dialog.findChild<QDialogButtonBox*>("buttonBox");
            if (step == Cancel) {
                choose("cbPaperFormat", RS2::Letter);
                buttons->button(QDialogButtonBox::Cancel)->click();
                QCOMPARE(graphic.getUnit(), RS2::Inch);
                QCOMPARE(graphic.getPaperSize().x, 30.0);
                QCOMPARE(graphic.getPaperSize().y, 15.0);
                QCOMPARE(PaperDefaults::entry("/Print", "/PaperSizeX").toDouble(), 762.0);
                continue;
            }
            if (step == Type) {
                dialog.findChild<QLineEdit*>("lePaperWidth")->setText("1.5");
                dialog.findChild<QLineEdit*>("lePaperHeight")->setText("1");
            }
            buttons->button(QDialogButtonBox::Ok)->click();
            QCOMPARE(graphic.getUnit(), RS2::Foot);
            QCOMPARE(graphic.getPaperSize().x, step == Type ? 1.5 : 2.5);
            QCOMPARE(graphic.getPaperSize().y, step == Type ? 1.0 : 1.25);
            QVERIFY(std::abs(graphic.getMarginLeft() - 10.0) < 1e-3);
        }
    }

    // Issue #2297: "Style Sheet" is a checkbox like "Style", on unless saved off (a profile
    // from before the checkbox has its sheet applied). Widget Options showed the saved path
    // only if its file existed, so for a drive that is not mounted OK saved an empty one.
    void widgetOptionsShowSavedStyleSheet()
    {
        const auto restore = qScopeGuard(&clearStyleSheetSettings);
        const QString missing = directory.filePath("not mounted/missing.qss");
        QSettings settings;
        settings.setValue("Widgets/StyleSheet", missing);
        widgetOptions([&](LC_WidgetOptionsDialog* dialog) {
            QCOMPARE(dialog->stylesheet_field->text(), missing);
            QVERIFY(dialog->stylesheet_checkbox->isChecked());
        });
        QCOMPARE(settings.value("Widgets/StyleSheet").toString(), missing);
        QVERIFY(settings.value("Widgets/AllowStyleSheet").toBool());

        settings.setValue("Widgets/AllowStyleSheet", 0);
        widgetOptions([&](LC_WidgetOptionsDialog* dialog) {
            QCOMPARE(dialog->stylesheet_field->text(), missing);
            QVERIFY(!dialog->stylesheet_checkbox->isChecked());
        });
        QCOMPARE(settings.value("Widgets/StyleSheet").toString(), missing);
        QVERIFY(!settings.value("Widgets/AllowStyleSheet").toBool());

        // nothing saved is on too, so a path typed in is applied as before
        settings.remove("Widgets/StyleSheet");
        settings.remove("Widgets/AllowStyleSheet");
        widgetOptions([](LC_WidgetOptionsDialog* dialog) {
            QVERIFY(dialog->stylesheet_field->text().isEmpty());
            QVERIFY(dialog->stylesheet_checkbox->isChecked());
        }, false);
    }

    // Unticking removes the sheet at once and keeps the path, Ctrl+T does not bring it
    // back, ticking applies it again, Cancel changes nothing, and the folder button
    // ticks the box with the file it picked.
    void widgetOptionsSwitchStyleSheet()
    {
        const auto restore = qScopeGuard(&clearStyleSheetSettings);
        const QByteArray content = "QToolTip { color: red; }";
        const QString sheet = writeStyleSheet("widget.qss", content);
        QVERIFY(!sheet.isEmpty());
        QSettings settings;
        settings.setValue("Widgets/StyleSheet", sheet);
        widgetOptions([](LC_WidgetOptionsDialog*) {});
        QCOMPARE(qApp->styleSheet(), QString::fromLatin1(content));

        widgetOptions([](LC_WidgetOptionsDialog* dialog) { dialog->stylesheet_checkbox->setChecked(false); });
        QVERIFY(qApp->styleSheet().isEmpty());
        QCOMPARE(settings.value("Widgets/StyleSheet").toString(), sheet);
        QVERIFY(!settings.value("Widgets/AllowStyleSheet").toBool());
        QC_ApplicationWindow::getAppWindow()->reloadStyleSheet();
        QVERIFY(qApp->styleSheet().isEmpty());

        widgetOptions([](LC_WidgetOptionsDialog* dialog) { dialog->stylesheet_checkbox->setChecked(true); }, false);
        QVERIFY(qApp->styleSheet().isEmpty());
        widgetOptions([](LC_WidgetOptionsDialog* dialog) { dialog->stylesheet_checkbox->setChecked(true); });
        QCOMPARE(qApp->styleSheet(), QString::fromLatin1(content));

        // CTest runs this target with QT_QPA_PLATFORM=offscreen, so the picker is a
        // QWidget dialog that the timer can drive.
        widgetOptions([&](LC_WidgetOptionsDialog* dialog) {
            dialog->stylesheet_checkbox->setChecked(false);
            dialog->stylesheet_field->clear();
            // selectFile() leaves the name alone while the field has the focus, so type it;
            // a dialog that does not accept it is rejected instead of hanging the test.
            int attempts = 0;
            QTimer pick;
            connect(&pick, &QTimer::timeout, [&] {
                for (QWidget* widget : QApplication::topLevelWidgets()) {
                    auto* files = qobject_cast<QFileDialog*>(widget);
                    if (files == nullptr || !files->isVisible()) continue;
                    if (auto* name = files->findChild<QLineEdit*>("fileNameEdit")) name->setText(sheet);
                    if (++attempts < 100) static_cast<QDialog*>(files)->accept();
                    else files->reject();
                }
            });
            pick.start(10);
            dialog->stylesheet_button->click();
            QVERIFY(dialog->stylesheet_checkbox->isChecked());
            QCOMPARE(QDir::fromNativeSeparators(dialog->stylesheet_field->text()), sheet);
        });
    }

    // At start-up the sheet is applied if its box is ticked or was never saved.
    void startupStyleSheet_data()
    {
        QTest::addColumn<bool>("saved");
        QTest::addColumn<QVariant>("allow");
        QTest::addColumn<bool>("applied");
        QTest::newRow("path, flag never saved") << true << QVariant() << true;
        QTest::newRow("path, flag on") << true << QVariant(1) << true;
        QTest::newRow("path, flag off") << true << QVariant(0) << false;
        QTest::newRow("nothing saved") << false << QVariant() << false;
    }

    void startupStyleSheet()
    {
        QFETCH(bool, saved);
        QFETCH(QVariant, allow);
        QFETCH(bool, applied);
        const auto restore = qScopeGuard(&clearStyleSheetSettings);
        const QByteArray content = "QToolTip { color: blue; }";
        const QString sheet = writeStyleSheet("startup.qss", content);
        QVERIFY(!sheet.isEmpty());
        QSettings settings;
        if (saved) settings.setValue("Widgets/StyleSheet", sheet);
        if (allow.isValid()) settings.setValue("Widgets/AllowStyleSheet", allow);
        const bool disabled = allow.isValid() && !allow.toBool();
        if (disabled)
        {
            // Reinitialize after a sheet was loaded to make sure disabling it clears both
            // the application sheet and the path used by Ctrl+T.
            qApp->setStyleSheet(QString());
            settings.setValue("Widgets/AllowStyleSheet", 1);
            QC_ApplicationWindow::getAppWindow()->initSettings();
            QCOMPARE(qApp->styleSheet(), QString::fromLatin1(content));
            settings.setValue("Widgets/AllowStyleSheet", 0);
        }
        else
            qApp->setStyleSheet(QString());

        QC_ApplicationWindow::getAppWindow()->initSettings();
        QCOMPARE(qApp->styleSheet(), applied ? QString::fromLatin1(content) : QString());
        if (disabled)
        {
            QC_ApplicationWindow::getAppWindow()->reloadStyleSheet();
            QVERIFY(qApp->styleSheet().isEmpty());
        }
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
    // Issue #3229: a hidden layer can be the current layer, and stays hidden.
    // Since 2.2.1.6 clicking its name did nothing, and hiding the current
    // layer made another one current.
    void hiddenLayerCanBeCurrent()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        auto* construction = new RS_Layer("Construction");
        auto* walls = new RS_Layer("Walls");
        graphic.addLayer(construction);
        graphic.addLayer(walls);
        graphic.toggleLayer(construction);
        QVERIFY(construction->isFrozen());

        graphic.activateLayer(construction);
        QCOMPARE(graphic.getActiveLayer(), construction);
        QVERIFY(construction->isFrozen());

        // what is drawn is hidden with the layer, and goes to it
        auto* line = new RS_Line(&graphic, RS_LineData({0, 0}, {10, 0}));
        line->setLayerToActive();
        graphic.addEntity(line);
        QCOMPARE(line->getLayer(), construction);
        QVERIFY(!line->isVisible());
    }

    void hidingTheCurrentLayerKeepsItCurrent()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        auto* walls = new RS_Layer("Walls");
        graphic.addLayer(walls);
        graphic.activateLayer(walls);

        // its eye icon
        graphic.toggleLayer(walls);
        QVERIFY(walls->isFrozen());
        QCOMPARE(graphic.getActiveLayer(), walls);
        graphic.toggleLayer(walls);
        QVERIFY(!walls->isFrozen());

        // editing it
        RS_Layer edited(*walls);
        edited.freeze(true);
        graphic.editLayer(walls, edited);
        QVERIFY(walls->isFrozen());
        QCOMPARE(graphic.getActiveLayer(), walls);
        graphic.toggleLayer(walls);

        // "Hide all layers except current" leaves it shown
        graphic.freezeAllLayers(true);
        QVERIFY(!walls->isFrozen());
        QVERIFY(zero->isFrozen());
        QCOMPARE(graphic.getActiveLayer(), walls);

        // a hidden layer clicked in the list becomes current, hidden
        graphic.activateLayer(zero);
        QCOMPARE(graphic.getActiveLayer(), zero);
        QVERIFY(zero->isFrozen());
        QVERIFY(!walls->isFrozen());
    }

    // Only the removal of the current layer picks another one: a layer that is
    // shown first.
    void removedCurrentLayerFallsBackToShownLayer()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        auto* doors = new RS_Layer("Doors");
        auto* walls = new RS_Layer("Walls");
        graphic.addLayer(doors);
        graphic.addLayer(walls);
        zero->freeze(true);
        doors->freeze(true);
        graphic.activateLayer(walls);
        graphic.removeLayer(walls);
        // the first layers are hidden, none is shown: the first one, still hidden
        QCOMPARE(graphic.getActiveLayer(), zero);
        QVERIFY(zero->isFrozen());

        auto* roof = new RS_Layer("Roof");
        graphic.addLayer(roof);
        graphic.activateLayer(doors);
        graphic.removeLayer(doors);
        QCOMPARE(graphic.getActiveLayer(), roof);
        QVERIFY(!roof->isFrozen());
    }

    // The current layer a drawing was saved with is current when it opens, also
    // when it is hidden.
    void hiddenCurrentLayerSurvivesDxfRoundTrip()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        RS_Graphic source;
        source.newDoc();
        auto* construction = new RS_Layer("Construction");
        source.addLayer(construction);
        source.activateLayer(construction);
        source.toggleLayer(construction);
        QVERIFY(construction->isFrozen());
        const QString path = directory.filePath("hidden-current.dxf");
        QVERIFY(RS_FileIO::instance()->fileExport(source, path, RS2::FormatDXFRW));

        RS_Graphic result;
        QVERIFY(result.open(path, RS2::FormatDXFRW));
        RS_Layer* restored = result.findLayer("Construction");
        QVERIFY(restored != nullptr);
        QVERIFY(restored->isFrozen());
        QCOMPARE(result.getActiveLayer(), restored);
    }

    // Clicking the name of a hidden layer in the Layer Tree makes it current.
    void layerTreeActivatesHiddenLayer()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        auto* walls = new RS_Layer("Walls");
        graphic.addLayer(walls);
        graphic.activateLayer(zero);
        walls->freeze(true);
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&graphic);
        LC_LayerTreeWidget tree(nullptr, nullptr);
        tree.setLayerList(graphic.getLayerList());
        tree.set_view(&view);
        tree.set_document(&graphic);
        auto* treeView = tree.findChild<LC_LayerTreeView*>();
        QVERIFY(treeView != nullptr);
        QAbstractItemModel* model = treeView->model();
        QModelIndex wallsIndex;
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, LC_LayerTreeModel::NAME);
            if (index.data().toString() == "Walls")
                wallsIndex = index;
        }
        QVERIFY(wallsIndex.isValid());
        tree.slotTreeClicked(wallsIndex);
        QCOMPARE(graphic.getActiveLayer(), walls);
        QVERIFY(walls->isFrozen());
    }

    // A layer added hidden, as a paste or a library insert brings one in, does
    // not become current: what is pasted goes to the current layer, and would
    // not be seen. A layer added shown becomes current, as before.
    void addedHiddenLayerIsNotMadeCurrent()
    {
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        QG_LayerWidget widget(nullptr, nullptr);
        widget.setLayerList(graphic.getLayerList(), false);
        graphic.addLayerListListener(&widget);
        auto* hidden = new RS_Layer("Hidden");
        hidden->freeze(true);
        graphic.addLayer(hidden);
        QCOMPARE(graphic.getActiveLayer(), zero);
        auto* shown = new RS_Layer("Shown");
        graphic.addLayer(shown);
        QCOMPARE(graphic.getActiveLayer(), shown);
        graphic.removeLayerListListener(&widget);
    }

    // Issue #3228: "Hide all layers" now says "except current". With the Layer List filtered, the
    // current layer was still hidden and a layer the filter left visible was
    // made current instead.
    void hideAllLayersKeepsCurrent_data()
    {
        QTest::addColumn<bool>("filtered");
        QTest::newRow("every layer listed") << false;
        QTest::newRow("list filtered to Wall*") << true;
    }

    void hideAllLayersKeepsCurrent()
    {
        QFETCH(bool, filtered);
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        QVERIFY(zero != nullptr);
        auto* doors = new RS_Layer("Doors");
        auto* wall1 = new RS_Layer("Wall1");
        auto* wall2 = new RS_Layer("Wall2");
        for (RS_Layer* layer : {doors, wall1, wall2})
            graphic.addLayer(layer);
        graphic.activateLayer(wall1);
        QCOMPARE(graphic.getActiveLayer(), wall1);
        if (filtered) {
            zero->visibleInLayerList(false);
            doors->visibleInLayerList(false);
        }

        graphic.freezeAllLayers(true);
        QCOMPARE(graphic.getActiveLayer(), wall1);
        QVERIFY(!wall1->isFrozen());
        QVERIFY(wall2->isFrozen());
        // a layer the filter does not list is left as it is
        QCOMPARE(doors->isFrozen(), !filtered);
        QCOMPARE(zero->isFrozen(), !filtered);

        graphic.freezeAllLayers(false);
        for (const RS_Layer* layer : {zero, doors, wall1, wall2})
            QVERIFY(!layer->isFrozen());
        QCOMPARE(graphic.getActiveLayer(), wall1);
    }

    // The same in the Layer Tree, whose filter either highlights the matching
    // layers or shows only those.
    void hideAllLayersInTreeKeepsCurrent_data()
    {
        QTest::addColumn<QString>("filter");
        QTest::addColumn<bool>("highlight");
        QTest::newRow("no filter") << QString() << true;
        QTest::newRow("highlighting Wall") << QString("Wall") << true;
        QTest::newRow("showing Wall only") << QString("Wall") << false;
    }

    void hideAllLayersInTreeKeepsCurrent()
    {
        QFETCH(QString, filter);
        QFETCH(bool, highlight);
        RS_Graphic graphic;
        graphic.newDoc();
        RS_Layer* zero = graphic.findLayer("0");
        QVERIFY(zero != nullptr);
        auto* doors = new RS_Layer("Doors");
        auto* wall1 = new RS_Layer("Wall1");
        auto* wall2 = new RS_Layer("Wall2");
        for (RS_Layer* layer : {doors, wall1, wall2})
            graphic.addLayer(layer);
        graphic.activateLayer(wall1);
        RS_StaticGraphicView view(100, 100, nullptr);
        view.setContainer(&graphic);
        LC_LayerTreeWidget tree(nullptr, nullptr);
        tree.setLayerList(graphic.getLayerList());
        tree.set_view(&view);
        tree.set_document(&graphic);
        auto* filterEdit = tree.findChild<QLineEdit*>();
        QCheckBox* highlightMode = nullptr;
        for (QCheckBox* box : tree.findChildren<QCheckBox*>()) {
            if (box->text() == "Highlight Mode")
                highlightMode = box;
        }
        QVERIFY(filterEdit != nullptr);
        QVERIFY(highlightMode != nullptr);
        if (highlightMode->isChecked() != highlight)
            highlightMode->click();
        filterEdit->setText(filter);

        tree.hideAllLayers();
        QCOMPARE(graphic.getActiveLayer(), wall1);
        QVERIFY(!wall1->isFrozen());
        QVERIFY(wall2->isFrozen());
        // a layer the filter takes out of the tree is left as it is
        const bool listed = highlight || filter.isEmpty();
        QCOMPARE(doors->isFrozen(), listed);
        QCOMPARE(zero->isFrozen(), listed);
    }
};

QTEST_MAIN(LC_StaticAnalysisTests)
#include "static_analysis_tests.moc"
