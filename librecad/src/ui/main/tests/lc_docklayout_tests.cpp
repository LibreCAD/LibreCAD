// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 librecad.org
// Copyright (C) 2026 Dongxu Li (github.com/dxli)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "lc_actiontestsupport.h"
#include "lc_actiongroupmanager.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_testsettingsisolation.h"
#include "lc_workspacesmanager.h"
#include "lc_propertysheetwidget.h"
#include "lc_properties_sheet.h"
#include "lc_guardedconnectionslist.h"
#include "lc_property_multi.h"
#include "lc_property_rsvector.h"
#include "lc_property_layer.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_system.h"
#include "qg_blockwidget.h"
#include "qg_pentoolbar.h"
#include "rs_fileio.h"
#include "rs_layer.h"
#include "rs_block.h"
#include "rs_line.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"

struct LC_DockLayoutTestAccess {
    using Workspace = LC_WorkspacesManager::LC_Workspace;
    static QC_ApplicationWindow* createWindow() { return new QC_ApplicationWindow; }
    static const QMap<QString, QAction*>& actions(QC_ApplicationWindow& window) {
        return window.m_actionGroupManager->getActionsMap();
    }
    static void fit(QC_ApplicationWindow& window) { window.fitDocksToWindow(); }
    static void isolate(QC_ApplicationWindow& window) {
        window.m_autoCollapsedGroups.clear();
        window.m_collapsedGroupMembers.clear();
        window.m_collapsedMemberKey.clear();
        window.m_collapsedSelectedTabs.clear();
        window.m_collapsedPressure.clear();
        for (auto it = window.m_requestedDockVisibility.begin(); it != window.m_requestedDockVisibility.end(); ++it)
            it.value() = false;
        window.applyRequestedDockVisibility();
        for (auto* toolbar : window.findChildren<QToolBar*>()) toolbar->hide();
        window.statusBar()->hide();
        window.menuBar()->hide();
        window.m_fittedDockState.clear();
    }
    static void add(QC_ApplicationWindow& window, QDockWidget* dock) {
        window.m_requestedDockVisibility[dock->objectName()] = true;
        dock->installEventFilter(&window);
    }
    static void collapse(QC_ApplicationWindow& window, QDockWidget* dock) {
        window.collapseDockGroup({dock}, window.dockGroupKey(dock), dock, Qt::Vertical);
    }
    static int surfaceBlocks(QC_ApplicationWindow& window, RS_Graphic& graphic) {
        lc::test::TestGraphicView view;
        view.setDocument(&graphic);
        window.m_blockWidget->setGraphicView(&view);
        const int count = window.maybeSurfaceBlocksDock(&graphic);
        window.m_blockWidget->setGraphicView(nullptr);
        return count;
    }
    static Workspace capture() {
        Workspace workspace;
        LC_WorkspacesManager().fillByState(workspace);
        return workspace;
    }
    static Workspace settingsRoundTrip(const Workspace& workspace) {
        LC_WorkspacesManager manager;
        manager.applyToSettings(workspace);
        Workspace restored;
        manager.fillBySettings(restored);
        return restored;
    }
    static Workspace fileRoundTrip(const Workspace& workspace, bool legacy) {
        LC_WorkspacesManager writer;
        writer.m_workspacesList.append(new Workspace(workspace));
        writer.saveWorkspaces();
        if (legacy) {
            QFile file(writer.getWorkspacesFileName());
            REQUIRE(file.open(QIODevice::ReadOnly));
            auto root = QJsonDocument::fromJson(file.readAll()).object();
            file.close();
            auto entries = root["workspaces"].toArray();
            auto entry = entries[0].toObject();
            entry.remove("dockVisibility");
            entry.remove("preset");
            entries[0] = entry;
            root["workspaces"] = entries;
            REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            REQUIRE(file.write(QJsonDocument(root).toJson()) > 0);
        }
        LC_WorkspacesManager reader;
        reader.loadWorkspaces();
        REQUIRE(reader.m_workspacesList.size() == 1);
        return *reader.m_workspacesList.first();
    }
};

namespace {
std::unique_ptr<QC_ApplicationWindow>* testWindowOwner = nullptr;

class DockWindowCleanup final : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;
    void testRunEnded(const Catch::TestRunStats&) override {
        // Match main(): destroy widgets before Qt's static resources are torn down.
        if (testWindowOwner) testWindowOwner->reset();
    }
};

CATCH_REGISTER_LISTENER(DockWindowCleanup)

void settle(int milliseconds = 250) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

QC_ApplicationWindow& testWindow() {
    REQUIRE(lc::test::application() != nullptr);
    REQUIRE(lc::test::SettingsIsolation::isActive());
    static bool initialized = [] {
        qApp->setQuitOnLastWindowClosed(false);
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SYSTEM->init("LibreCAD", "3.0", "librecad", QString(LIBRECAD_SOURCE_DIR) + "/unix");
        RS_SETTINGS->writeSingle("Startup", "FirstLoad", true);
        RS_SETTINGS->writeSingle("Defaults", "AutoBackupDocument", false);
        RS_SETTINGS->writeSingle("Paths", "OtherSettingsDir", lc::test::SettingsIsolation::directory());
        return true;
    }();
    (void)initialized;
    if (!testWindowOwner) testWindowOwner = &QC_ApplicationWindow::getAppWindow();
    else {
        testWindowOwner->reset();
        testWindowOwner->reset(LC_DockLayoutTestAccess::createWindow());
    }
    return **testWindowOwner;
}

struct DockFixture {
    QC_ApplicationWindow& window {testWindow()};
    QList<QWidget*> extras;
    DockFixture() {
        window.resize(700, 500);
        window.prepareWindowForShow();
        window.show();
        settle();
    }
    ~DockFixture() {
        window.hide();
        for (QWidget* widget : extras) delete widget;
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    void isolate() { LC_DockLayoutTestAccess::isolate(window); }
    QDockWidget* addDock(const QString& name, QSize minimum = QSize(150, 60)) {
        auto* dock = new QDockWidget(name, &window);
        dock->setObjectName(name);
        auto* content = new QWidget(dock);
        content->setMinimumSize(minimum);
        dock->setWidget(content);
        window.addDockWidget(Qt::RightDockWidgetArea, dock);
        LC_DockLayoutTestAccess::add(window, dock);
        extras.append(dock);
        return dock;
    }
};

QStringList tabOrder(QC_ApplicationWindow& window, const QString& first, const QString& second) {
    for (auto* bar : window.findChildren<QTabBar*>()) {
        QStringList names;
        for (int i = 0; i < bar->count(); ++i) {
            if (bar->tabText(i) == first || bar->tabText(i) == second) names.append(bar->tabText(i));
        }
        if (names.size() == 2) return names;
    }
    return {};
}
}

// Explicit opt-in evidence capture. This uses the same isolated settings as
// the dock tests, and never exports a user's workspace or drawing contents.
TEST_CASE("Capture the CAD workspace baseline", "[.baseline]") {
    const QString output = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    REQUIRE_FALSE(output.isEmpty());
    REQUIRE(QDir().mkpath(output));
    lc::test::application();
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    RS_SETTINGS->writeSingle("Paths", "Fonts", QString(LIBRECAD_SOURCE_DIR) + "/librecad/support/fonts");
    RS_SETTINGS->writeSingle("Paths", "Patterns", QString(LIBRECAD_SOURCE_DIR) + "/librecad/support/patterns");
    RS_FONTLIST->init();
    RS_PATTERNLIST->init();
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle(500);
    const auto* graphic = window.getActionContext()->getGraphic();
    REQUIRE(graphic != nullptr);
    REQUIRE(graphic->count() == 5);
    QJsonArray actions;
    const auto& registry = LC_DockLayoutTestAccess::actions(window);
    for (auto it = registry.cbegin(); it != registry.cend(); ++it) {
        REQUIRE(it.value() != nullptr);
        actions.append(QJsonObject{{"id", it.key()}, {"label", it.value()->text()},
                                   {"shortcut", it.value()->shortcut().toString(QKeySequence::PortableText)}});
    }
    for (const auto& id : {"FileNew", "FileOpen", "FileSave", "EditUndo", "EditRedo", "DrawLine", "LayersAdd", "FilePrintPDF"}) {
        REQUIRE(registry.contains(QLatin1String(id)));
    }
    QJsonArray docks;
    for (auto* dock : window.findChildren<QDockWidget*>()) {
        docks.append(QJsonObject{{"id", dock->objectName()}, {"title", dock->windowTitle()},
                                 {"area", int(window.dockWidgetArea(dock))}, {"visible", dock->isVisible()}});
    }
    REQUIRE(window.findChild<QDockWidget*>("property_sheet") != nullptr);
    REQUIRE(window.findChild<QDockWidget*>("layer_dockwidget") != nullptr);
    REQUIRE(window.findChild<QDockWidget*>("command_dockwidget") != nullptr);
    CHECK(window.dockWidgetArea(window.findChild<QDockWidget*>("command_dockwidget")) == Qt::BottomDockWidgetArea);
    CHECK_FALSE(window.getAction("InfoCursorEnable")->isChecked());
    QJsonObject result{{"qt", QString(qVersion())}, {"platform", QGuiApplication::platformName()},
                       {"actions", actions}, {"docks", docks}, {"model_entities", int(graphic->count())},
                       {"info_cursor_enabled", window.getAction("InfoCursorEnable")->isChecked()},
                       {"settings_isolated", lc::test::SettingsIsolation::isActive()}};
    QJsonArray penControls;
    for (auto* combo : window.getPenToolBar()->findChildren<QComboBox*>()) {
        penControls.append(QJsonObject{{"id", combo->objectName()}, {"visible", combo->isVisible()},
                                      {"height", combo->height()}, {"hint_height", combo->sizeHint().height()}});
        CHECK(combo->isVisible());
    }
    result["pen_controls"] = penControls;
    result["pen_height"] = window.getPenToolBar()->height();
    result["canvas_width"] = window.getMdiArea()->width();
    result["canvas_height"] = window.getMdiArea()->height();
    QFile file(QDir(output).filePath("runtime-workspace.json"));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    REQUIRE(file.write(QJsonDocument(result).toJson()) > 0);
    REQUIRE(window.grab().save(QDir(output).filePath("workspace-baseline.png")));
    auto* properties = window.findChild<QDockWidget*>("property_sheet");
    properties->show();
    properties->raise();
    REQUIRE(registry.contains("SelectAll"));
    registry["SelectAll"]->trigger();
    settle(500);
    int selected = 0;
    for (const auto* entity : *graphic) {
        if (entity->isSelected()) ++selected;
    }
    REQUIRE(selected > 0);
    result.insert("selected_entities_after_select_all", selected);
    if (const auto* filter = properties->findChild<QComboBox*>("cbSelection")) {
        result.insert("properties_filter_after_select_all", filter->currentText());
    }
    file.resize(0);
    file.seek(0);
    REQUIRE(file.write(QJsonDocument(result).toJson()) > 0);
    REQUIRE(window.grab().save(QDir(output).filePath("properties-baseline.png")));
    CHECK(result["properties_filter_after_select_all"].toString().startsWith("All"));
    window.resize(1024, 640);
    settle();
    CHECK(window.findChild<QDockWidget*>("command_dockwidget")->isVisible());
    REQUIRE(window.grab().save(QDir(output).filePath("workspace-1024.png")));
    result["small_canvas_width"] = window.getMdiArea()->width();
    result["small_canvas_height"] = window.getMdiArea()->height();
    window.resize(1280, 800);
    window.activateCadWorkspace("classic-v1");
    settle();
    REQUIRE(window.grab().save(QDir(output).filePath("workspace-classic.png")));
    window.activateCadWorkspace("cad-familiar-v1");
    settle();
    file.resize(0);
    file.seek(0);
    REQUIRE(file.write(QJsonDocument(result).toJson()) > 0);
}

TEST_CASE("CAD presets retain arrangements and shared commands", "[.gui][dock-window][cad-workspace]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.activateCadWorkspace("cad-familiar-v1", true);
    settle();
    auto* ribbon = window.findChild<QToolBar*>("cad_ribbon");
    auto* quick = window.findChild<QToolBar*>("cad_quick_access");
    auto* properties = window.findChild<QDockWidget*>("property_sheet");
    auto* layers = window.findChild<QDockWidget*>("layer_dockwidget");
    auto* command = window.findChild<QDockWidget*>("command_dockwidget");
    REQUIRE(ribbon);
    REQUIRE(quick);
    REQUIRE(properties);
    REQUIRE(layers);
    REQUIRE(command);
    CHECK(ribbon->isVisible());
    CHECK(quick->actions().contains(window.getAction("FileSave")));
    CHECK(window.getPenToolBar()->orientation() == Qt::Vertical);
    CHECK(window.getPenToolBar()->parentWidget()->objectName() == "cad_pen_host");
    CHECK(window.requestedDockVisibility().value(properties->objectName()));
    CHECK(window.requestedDockVisibility().value(layers->objectName()));
    CHECK(window.getPropertySheetWidget()->isVisible());
    CHECK(window.dockWidgetArea(command) == Qt::BottomDockWidgetArea);
    CHECK(command->isVisible());
    INFO("canvas " << window.getMdiArea()->width() << 'x' << window.getMdiArea()->height());
    CHECK(window.getMdiArea()->width() >= 650);
    CHECK(window.getMdiArea()->height() >= 420);
    bool sharedLine = false;
    for (auto* control : ribbon->findChildren<QToolButton*>()) {
        if (control->defaultAction() == window.getAction("DrawLine")) sharedLine = true;
    }
    CHECK(sharedLine);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_SETTINGS->writeSingle("Paths", "OtherSettingsDir", directory.path());
    const QMap<QString, QByteArray> preserved{{"librecad.alias", "phase1line line\n"},
                                            {"shortcuts.lcsc", "{\"phase1-sentinel\":true}\n"}};
    for (auto it = preserved.begin(); it != preserved.end(); ++it) {
        QFile file(directory.filePath(it.key()));
        REQUIRE(file.open(QIODevice::WriteOnly));
        REQUIRE(file.write(it.value()) == it.value().size());
    }
    properties->close();
    window.activateCadWorkspace("classic-v1");
    settle();
    CHECK(ribbon->isHidden());
    CHECK(quick->isHidden());
    CHECK(window.getPenToolBar()->orientation() == Qt::Horizontal);
    CHECK(window.getPenToolBar()->parentWidget() == &window);
    window.activateCadWorkspace("cad-familiar-v1");
    settle();
    CHECK(properties->isHidden());
    window.activateCadWorkspace("cad-familiar-v1", true);
    settle();
    CHECK(window.requestedDockVisibility().value(properties->objectName()));
    for (auto it = preserved.begin(); it != preserved.end(); ++it) {
        QFile file(directory.filePath(it.key()));
        REQUIRE(file.open(QIODevice::ReadOnly));
        CHECK(file.readAll() == it.value());
    }
    RS_SETTINGS->writeSingle("Paths", "OtherSettingsDir", lc::test::SettingsIsolation::directory());
}

TEST_CASE("Ribbon drafting properties undo and DXF reopen stay coherent", "[.gui][dock-window][cad-workspace]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.activateCadWorkspace("cad-familiar-v1", true);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle();
    auto* graphic = window.getActionContext()->getGraphic();
    REQUIRE(graphic);
    auto* layerCombo = window.findChild<QComboBox*>("cad_current_layer");
    REQUIRE(layerCombo);
    const int geometryIndex = layerCombo->findText("GEOMETRY");
    REQUIRE(geometryIndex >= 0);
    layerCombo->setCurrentIndex(geometryIndex);
    layerCombo->activated(geometryIndex);
    CHECK(graphic->getActiveLayer()->getName() == "GEOMETRY");
    auto* command = window.findChild<QWidget*>("Command");
    REQUIRE(command);
    const QString captureOutput = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    auto capture = [&window, &captureOutput](const QString& name) {
        if (!captureOutput.isEmpty()) REQUIRE(window.grab().save(QDir(captureOutput).filePath(name)));
    };
    window.getAction("DrawLine")->trigger();
    REQUIRE(QMetaObject::invokeMethod(command, "handleCommand", Q_ARG(QString, "0,100")));
    settle();
    capture("workspace-line-command.png");
    REQUIRE(QMetaObject::invokeMethod(command, "handleCommand", Q_ARG(QString, "100,100")));
    window.slotKillAllActions();
    settle();
    auto draftedLine = [graphic]() -> RS_Line* {
        for (auto* entity : *graphic) {
            if (!entity->isDeleted() && entity->rtti() == RS2::EntityLine &&
                std::abs(entity->getStartpoint().y - 100) < 1e-6) return static_cast<RS_Line*>(entity);
        }
        return nullptr;
    };
    auto* line = draftedLine();
    REQUIRE(line);
    CHECK(line->getLength() == Catch::Approx(100));
    graphic->select(QList<RS_Entity*>{line}, true);
    auto* sheet = window.getPropertySheetWidget()->findChild<LC_PropertiesSheet*>();
    REQUIRE(sheet);
    settle();
    REQUIRE(sheet->getPropertyContainer());
    for (auto* section : sheet->getPropertyContainer()->findChildProperties("$_secGeometry")) section->expand();
    auto* end = sheet->getPropertyWithName("$_secGeometry$end");
    REQUIRE(end);
    capture("workspace-line-properties.png");
    sheet->setActiveProperty(end);
    auto* atomicEnd = end->asAtomic();
    if (auto* multi = dynamic_cast<LC_PropertyMulti*>(atomicEnd)) atomicEnd = multi->getFirstProperty();
    auto* vectorEnd = dynamic_cast<LC_PropertyRSVector*>(atomicEnd);
    REQUIRE(vectorEnd);
    {
        LC_GuardedConnectionsList editConnections;
        sheet->connectPropertyToEdit(end, editConnections);
        if (auto* multi = dynamic_cast<LC_PropertyMulti*>(end)) multi->markEdited(true);
        REQUIRE(vectorEnd->setValue(RS_Vector{120, 100}, PropertyChangeReasonEdit));
        settle();
    }
    REQUIRE(draftedLine());
    CHECK(draftedLine()->getLength() == Catch::Approx(120));
    for (auto* section : sheet->getPropertyContainer()->findChildProperties("$_secGeneral")) section->expand();
    auto* layerProperty = sheet->getPropertyWithName("$_secGeneral$layer");
    REQUIRE(layerProperty);
    sheet->setActiveProperty(layerProperty);
    auto* atomicLayer = layerProperty->asAtomic();
    if (auto* multi = dynamic_cast<LC_PropertyMulti*>(atomicLayer)) atomicLayer = multi->getFirstProperty();
    auto* selectedLayer = dynamic_cast<LC_PropertyLayer*>(atomicLayer);
    REQUIRE(selectedLayer);
    {
        LC_GuardedConnectionsList editConnections;
        sheet->connectPropertyToEdit(layerProperty, editConnections);
        if (auto* multi = dynamic_cast<LC_PropertyMulti*>(layerProperty)) multi->markEdited(true);
        REQUIRE(selectedLayer->setValue(graphic->findLayer("ANNOTATION"), PropertyChangeReasonEdit));
        settle();
    }
    CHECK(draftedLine()->getLayer()->getName() == "ANNOTATION");
    window.getAction("EditUndo")->trigger();
    settle();
    CHECK(draftedLine()->getLayer()->getName() == "GEOMETRY");
    CHECK(draftedLine()->getLength() == Catch::Approx(120));
    window.getAction("EditUndo")->trigger();
    settle();
    CHECK(draftedLine()->getLength() == Catch::Approx(100));
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto saved = directory.filePath("drafting.dxf");
    REQUIRE(RS_FileIO::instance()->fileExport(*graphic, saved, RS2::FormatDXFRW));
    window.openFile(saved);
    settle();
    auto* reopened = window.getActionContext()->getGraphic();
    REQUIRE(reopened);
    CHECK(reopened != graphic);
    RS_Line* reopenedLine = nullptr;
    for (auto* entity : *reopened) {
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityLine &&
            std::abs(entity->getStartpoint().y - 100) < 1e-6) reopenedLine = static_cast<RS_Line*>(entity);
    }
    REQUIRE(reopenedLine);
    CHECK(reopenedLine->getLength() == Catch::Approx(100));
    CHECK(reopenedLine->getLayer()->getName() == "GEOMETRY");
    reopened->select(QList<RS_Entity*>{reopenedLine}, true);
    settle();
    CHECK(window.getPropertySheetWidget()->findChild<QComboBox*>("cbSelection")->currentData().toInt() == RS2::EntityLine);
    for (const auto& name : {"0", "GEOMETRY", "ANNOTATION", "LOCKED"})
        CHECK(layerCombo->findText(name) >= 0);
    window.activateWindow(window.getMdiArea()->subWindowList().first());
    settle();
    CHECK(window.getActionContext()->getGraphic() == graphic);
    CHECK(draftedLine()->getLength() == Catch::Approx(100));
    graphic->setModified(false);
    for (auto* control : window.findChild<QToolBar*>("cad_ribbon")->findChildren<QToolButton*>()) {
        if (control->defaultAction() == window.getAction("DrawLine")) CHECK(control->text() == "Line");
    }
    window.requestDockVisible(window.findChild<QDockWidget*>("layer_dockwidget"));
    settle();
    capture("workspace-layers.png");
}

TEST_CASE("CAD presets and palette choices survive restart", "[.gui][dock-window][cad-workspace]") {
    {
        DockFixture fixture;
        fixture.window.activateCadWorkspace("classic-v1", true);
        fixture.window.storeSettings();
        RS_SETTINGS->writeSingle("Startup", "FirstLoad", false);
    }
    {
        DockFixture fixture;
        auto& window = fixture.window;
        CHECK(window.cadWorkspacePreset() == "classic-v1");
        CHECK(window.findChild<QToolBar*>("cad_ribbon")->isHidden());
        CHECK(window.getPenToolBar()->parentWidget() == &window);
        window.activateCadWorkspace("cad-familiar-v1", true);
        window.findChild<QDockWidget*>("property_sheet")->close();
        window.storeSettings();
    }
    {
        DockFixture fixture;
        auto& window = fixture.window;
        window.resize(1280, 800);
        settle();
        CHECK(window.cadWorkspacePreset() == "cad-familiar-v1");
        CHECK(window.findChild<QToolBar*>("cad_ribbon")->isVisible());
        CHECK(window.getPenToolBar()->parentWidget()->objectName() == "cad_pen_host");
        CHECK(window.findChild<QDockWidget*>("property_sheet")->isHidden());
        CHECK(window.dockWidgetArea(window.findChild<QDockWidget*>("command_dockwidget")) == Qt::BottomDockWidgetArea);
        window.activateCadWorkspace("cad-familiar-v1", true);
    }
    RS_SETTINGS->writeSingle("Startup", "FirstLoad", true);
}

TEST_CASE("Main window fits its available screen", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1600, 1024);
    window.prepareWindowForShow();
    settle();
    REQUIRE(window.windowHandle());
    CHECK(window.windowHandle()->screen()->availableGeometry().contains(window.frameGeometry()));
    CHECK(window.getMdiArea()->width() >= 480);
    CHECK(window.getMdiArea()->height() >= 320);
}

TEST_CASE("Legacy workspace migrates command dock and cursor once", "[.gui][dock-window]") {
    {
        DockFixture fixture;
        auto& window = fixture.window;
        window.resize(1280, 800);
        auto* command = window.findChild<QDockWidget*>("command_dockwidget");
        auto* layers = window.findChild<QDockWidget*>("layer_dockwidget");
        REQUIRE(command);
        REQUIRE(layers);
        window.addDockWidget(Qt::RightDockWidgetArea, command);
        window.tabifyDockWidget(layers, command);
        window.toggleBottomDockArea(false);
        window.storeSettings();
        RS_SETTINGS->writeSingle("Startup", "FirstLoad", false);
        RS_SETTINGS->writeSingle("Startup", "CommandWorkspaceDefaultsApplied", false);
        RS_SETTINGS->writeSingle("InfoOverlayCursor", "Enabled", true);
    }
    {
        DockFixture fixture;
        auto& window = fixture.window;
        window.resize(1280, 800);
        settle();
        auto* command = window.findChild<QDockWidget*>("command_dockwidget");
        REQUIRE(command);
        CHECK(window.dockWidgetArea(command) == Qt::BottomDockWidgetArea);
        CHECK(window.dockAreaRequested(Qt::BottomDockWidgetArea));
        CHECK(window.tabifiedDockWidgets(command).isEmpty());
        CHECK_FALSE(command->isHidden());
        CHECK_FALSE(window.getAction("InfoCursorEnable")->isChecked());
        CHECK_FALSE(window.getAction("InfoCursorAbs")->isEnabled());
        LC_InfoCursorOverlayPrefs prefs;
        prefs.loadSettings();
        CHECK_FALSE(prefs.enabled);
        window.storeSettings();
    }
    {
        DockFixture fixture;
        auto& window = fixture.window;
        auto* command = window.findChild<QDockWidget*>("command_dockwidget");
        REQUIRE(command);
        CHECK(window.dockWidgetArea(command) == Qt::BottomDockWidgetArea);
        CHECK_FALSE(window.getAction("InfoCursorEnable")->isChecked());
        // A deliberate later opt-in/rearrangement survives another restart.
        window.getAction("InfoCursorEnable")->trigger();
        window.addDockWidget(Qt::RightDockWidgetArea, command);
        window.storeSettings();
    }
    {
        DockFixture fixture;
        auto& window = fixture.window;
        auto* command = window.findChild<QDockWidget*>("command_dockwidget");
        REQUIRE(command);
        CHECK(window.dockWidgetArea(command) == Qt::RightDockWidgetArea);
        CHECK(window.getAction("InfoCursorEnable")->isChecked());
        LC_InfoCursorOverlayPrefs prefs;
        prefs.loadSettings();
        CHECK(prefs.enabled);
        // Leave the shared test settings at the new defaults.
        window.getAction("InfoCursorEnable")->trigger();
        window.addDockWidget(Qt::BottomDockWidgetArea, command);
        window.storeSettings();
        RS_SETTINGS->writeSingle("Startup", "FirstLoad", true);
    }
}

TEST_CASE("Draft toggling preserves the drawing title", "[.gui][draft][3018]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.slotFileNewFromDefaultTemplate();
    auto* mdi = window.getCurrentMDIWindow();
    REQUIRE(mdi != nullptr);
    auto* view = mdi->getGraphicView();
    REQUIRE(view != nullptr);
    auto* draft = window.getAction("ViewDraft");
    REQUIRE(draft != nullptr);
    REQUIRE(draft->isEnabled());
    window.slotViewDraft(false);
    const QString normalTitle = mdi->windowTitle();

    for (bool enabled : {true, false, true, false}) {
        draft->trigger();
        CHECK(view->isDraftMode() == enabled);
        CHECK(draft->isChecked() == enabled);
        const QString expectedTitle = enabled
            ? normalTitle + " [" + QC_ApplicationWindow::tr("Draft Mode") + "]" : normalTitle;
        CHECK(mdi->windowTitle() == expectedTitle);
        settle(0);
    }
}

TEST_CASE("First-run maximization fits the available client area", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.prepareWindowForShow();
    window.showMaximized();
    settle();
    const QSize available = window.windowHandle()->screen()->availableGeometry().size();
    CHECK(window.isMaximized());
    CHECK(window.width() <= available.width());
    CHECK(window.height() <= available.height());
    CHECK(window.getMdiArea()->width() >= 480);
    CHECK(window.getMdiArea()->height() >= 320);
}

TEST_CASE("An overall 1024 by 768 window keeps a usable drawing area", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    const QSize frameMargins = window.frameGeometry().size() - window.size();
    INFO("initial minimum=" << window.minimumWidth() << 'x' << window.minimumHeight()
         << " frame=" << window.frameGeometry().width() << 'x' << window.frameGeometry().height());
    window.resize(QSize(1024, 768) - frameMargins);
    settle();
    INFO("fitted minimum=" << window.minimumWidth() << 'x' << window.minimumHeight()
         << " canvas=" << window.getMdiArea()->width() << 'x' << window.getMdiArea()->height());
    CHECK(window.frameGeometry().width() <= 1024);
    CHECK(window.frameGeometry().height() <= 768);
    CHECK(window.getMdiArea()->width() >= 480);
    CHECK(window.getMdiArea()->height() >= 320);
}

TEST_CASE("Command remains visible after moving into a disabled dock area", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.toggleBottomDockArea(false);
    auto* command = window.findChild<QDockWidget*>("command_dockwidget");
    REQUIRE(command);
    window.requestDockVisible(command);
    window.addDockWidget(Qt::BottomDockWidgetArea, command);
    CHECK(window.dockAreaRequested(Qt::BottomDockWidgetArea));
    window.slotFileNewFromDefaultTemplate();
    CHECK_FALSE(command->isHidden());
}

TEST_CASE("Fullscreen transition preserves a fitting dock", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto* dock = fixture.addDock("fullscreen_panel");
    settle();
    fixture.window.showFullScreen();
    settle(1500);
    INFO("fullscreen frame=" << fixture.window.width() << 'x' << fixture.window.height()
         << " canvas=" << fixture.window.getMdiArea()->width() << 'x' << fixture.window.getMdiArea()->height()
         << " dock=" << dock->width() << 'x' << dock->height());
    CHECK(fixture.window.isFullScreen());
    CHECK_FALSE(dock->isHidden());
    fixture.window.showNormal();
    settle(1500);
    CHECK_FALSE(dock->isHidden());
}

TEST_CASE("Split groups reclaim a shared column together", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto* first = fixture.addDock("split_first");
    auto* second = fixture.addDock("split_second");
    settle();
    fixture.window.resize(420, 500);
    settle();
    INFO("split frame=" << fixture.window.width() << 'x' << fixture.window.height()
         << " minimum=" << fixture.window.minimumWidth() << 'x' << fixture.window.minimumHeight()
         << " canvas=" << fixture.window.getMdiArea()->width() << 'x' << fixture.window.getMdiArea()->height());
    CHECK(first->isHidden());
    CHECK(second->isHidden());
    CHECK(fixture.window.getMdiArea()->width() >= 320);
}

TEST_CASE("Explicit priority cannot force oversized split panes beyond the screen", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    const QRect available = window.windowHandle()->screen()->availableGeometry();
    auto* first = fixture.addDock("oversized_first", QSize(available.width() + 100, 60));
    auto* second = fixture.addDock("oversized_second", QSize(available.width() + 100, 60));
    window.requestDockVisible(second);
    LC_DockLayoutTestAccess::fit(window);
    CHECK(first->isHidden());
    CHECK(second->isHidden());
    CHECK(available.contains(window.frameGeometry()));
    CHECK(window.requestedDockVisibility().value(second->objectName()));
}

TEST_CASE("Hiding a toolbar restores docks without growing the window", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    auto* first = fixture.addDock("restore_first");
    auto* second = fixture.addDock("restore_second");
    auto* toolbar = new QToolBar(&window);
    toolbar->setObjectName("fixture_toolbar");
    toolbar->installEventFilter(&window);
    fixture.extras.append(toolbar);
    auto* content = new QWidget(toolbar);
    content->setFixedSize(170, 100);
    toolbar->addWidget(content);
    window.addToolBar(Qt::LeftToolBarArea, toolbar);
    toolbar->show();
    window.resize(760, 500);
    settle();
    REQUIRE(first->isHidden());
    REQUIRE(second->isHidden());
    const QSize size = window.size();
    toolbar->hide();
    settle();
    INFO("restored frame=" << window.width() << 'x' << window.height()
         << " canvas=" << window.getMdiArea()->width() << 'x' << window.getMdiArea()->height());
    CHECK(window.size() == size);
    CHECK_FALSE(first->isHidden());
    CHECK_FALSE(second->isHidden());
}

TEST_CASE("Saving intended docks does not change the effective geometry or layout", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    auto* first = fixture.addDock("save_first", QSize(150, 400));
    fixture.addDock("save_second", QSize(150, 400));
    LC_DockLayoutTestAccess::collapse(window, first);
    window.resize(700, 500);
    settle();
    const QByteArray state = window.saveState();
    const QRect geometry = window.geometry();
    const auto requested = window.requestedDockVisibility();
    CHECK_FALSE(window.dockLayoutStateForSaving().isEmpty());
    settle();
    CHECK(window.geometry() == geometry);
    CHECK(window.saveState() == state);
    CHECK(window.requestedDockVisibility() == requested);
}

TEST_CASE("Area toggles preserve individually hidden dock choices", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    auto* layers = window.findChild<QDockWidget*>("layer_dockwidget");
    auto* command = window.findChild<QDockWidget*>("command_dockwidget");
    REQUIRE(layers);
    REQUIRE(command);
    window.requestDockVisible(layers);
    window.requestDockVisible(command);
    command->close();
    const auto requested = window.requestedDockVisibility();
    window.toggleRightDockArea(false);
    CHECK(layers->isHidden());
    window.toggleRightDockArea(true);
    CHECK_FALSE(layers->isHidden());
    CHECK(command->isHidden());
    CHECK(window.requestedDockVisibility() == requested);
}

TEST_CASE("Collapsed tabs retain user order and selection", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    auto* first = fixture.addDock("tab_first");
    auto* second = fixture.addDock("tab_second");
    window.tabifyDockWidget(first, second);
    fixture.addDock("tab_split");
    window.resize(760, 500);
    settle();
    for (auto* bar : window.findChildren<QTabBar*>()) {
        if (bar->count() == 2 && bar->tabText(0) == first->windowTitle()) bar->moveTab(0, 1);
    }
    second->raise();
    const QStringList intended = tabOrder(window, first->windowTitle(), second->windowTitle());
    REQUIRE(intended.size() == 2);
    window.resize(420, 500);
    settle();
    REQUIRE(first->isHidden());
    REQUIRE(second->isHidden());
    window.resize(760, 500);
    settle();
    CHECK_FALSE(first->isHidden());
    CHECK_FALSE(second->isHidden());
    CHECK(tabOrder(window, first->windowTitle(), second->windowTitle()) == intended);
    CHECK(second->isVisible());
}

TEST_CASE("Command focus and floating geometry survive deferred fitting", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.slotFocusCommandLine();
    settle();
    auto* command = window.findChild<QDockWidget*>("command_dockwidget");
    REQUIRE(command);
    CHECK_FALSE(command->isHidden());
    window.toggleFloatingDockwidgets(false);
    command->setFloating(true);
    command->move(-10000, -10000);
    settle();
    CHECK(window.floatingDocksRequested());
    CHECK_FALSE(command->isHidden());
    CHECK(command->windowHandle()->screen()->availableGeometry().contains(command->frameGeometry()));
}

TEST_CASE("Failed and legacy state restore preserve visibility intent", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    const QHash<int, bool> areas{{Qt::RightDockWidgetArea, true}};
    auto* layers = window.findChild<QDockWidget*>("layer_dockwidget");
    REQUIRE(layers);
    window.requestDockVisible(layers);
    layers->hide();
    const QByteArray legacy = window.saveState();
    window.restoreDockLayout({}, false, areas, legacy);
    CHECK_FALSE(window.requestedDockVisibility().value(layers->objectName()));
    window.restoreDockLayout({{"unknown_dock", true}}, true, areas, QByteArray("invalid"));
    CHECK_FALSE(window.requestedDockVisibility().contains("unknown_dock"));
    CHECK(window.requestedDockVisibility().value(layers->objectName()));
    CHECK_FALSE(layers->isHidden());
}

TEST_CASE("Blocks auto-surfacing enables its disabled dock area", "[.gui][dock-window]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.toggleRightDockArea(false);
    RS_Graphic graphic;
    auto* block = new RS_Block(&graphic, RS_BlockData("fixture block", RS_Vector{0, 0}, false));
    block->addEntity(new RS_Line(block, RS_LineData(RS_Vector{0, 0}, RS_Vector{1, 0})));
    graphic.addBlock(block);
    CHECK(LC_DockLayoutTestAccess::surfaceBlocks(window, graphic) == 1);
    CHECK(window.dockAreaRequested(Qt::RightDockWidgetArea));
    auto* dock = window.findChild<QDockWidget*>("block_dockwidget");
    REQUIRE(dock);
    CHECK(window.requestedDockVisibility().value(dock->objectName()));
    CHECK_FALSE(dock->isHidden());
}

TEST_CASE("Temporary toolbar wrapping is not persisted or left mutated by saving", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    QToolBar* second = nullptr;
    for (int i = 0; i < 2; ++i) {
        auto* toolbar = new QToolBar(&window);
        toolbar->setObjectName(QString("bottom_fixture_%1").arg(i));
        toolbar->installEventFilter(&window);
        fixture.extras.append(toolbar);
        auto* content = new QWidget(toolbar);
        content->setFixedSize(280, 20);
        toolbar->addWidget(content);
        window.addToolBar(Qt::BottomToolBarArea, toolbar);
        toolbar->show();
        second = toolbar;
    }
    window.resize(420, 500);
    settle();
    REQUIRE(window.toolBarBreak(second));
    const auto state = window.saveState();
    const auto geometry = window.geometry();
    const auto intended = window.dockLayoutStateForSaving();
    CHECK(intended != state);
    settle();
    CHECK(window.saveState() == state);
    CHECK(window.geometry() == geometry);
    CHECK(window.toolBarBreak(second));
}

TEST_CASE("Bottom toolbar wrapping follows the user's reordered layout", "[.gui][dock-window]") {
    DockFixture fixture;
    fixture.isolate();
    auto& window = fixture.window;
    SECTION("left to right") { window.setLayoutDirection(Qt::LeftToRight); }
    SECTION("right to left") { window.setLayoutDirection(Qt::RightToLeft); }
    QList<QToolBar*> toolbars;
    for (int i = 0; i < 3; ++i) {
        auto* toolbar = new QToolBar(&window);
        toolbar->setObjectName(QString("reordered_toolbar_%1").arg(i));
        toolbar->installEventFilter(&window);
        fixture.extras.append(toolbar);
        auto* content = new QWidget(toolbar);
        content->setFixedSize(260, 20);
        toolbar->addWidget(content);
        window.addToolBar(Qt::BottomToolBarArea, toolbar);
        toolbar->show();
        toolbars.append(toolbar);
    }
    window.insertToolBar(toolbars.first(), toolbars.last());
    settle();
    window.resize(500, 500);
    settle();
    QStringList itemOrder;
    for (int i = 0; i < window.layout()->count(); ++i) {
        if (auto* toolbar = qobject_cast<QToolBar*>(window.layout()->itemAt(i)->widget()))
            itemOrder.append(toolbar->objectName());
    }
    INFO("Qt toolbar item order: " << qPrintable(itemOrder.join(',')));
    CHECK_FALSE(window.toolBarBreak(toolbars.last()));
    CHECK(window.toolBarBreak(toolbars.first()));
    CHECK(window.toolBarBreak(toolbars[1]));
    CHECK(window.layout()->minimumSize().width() <= window.width());
}

TEST_CASE("Named and global workspaces round-trip requested dock preferences", "[.gui][dock-window]") {
    DockFixture fixture;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    {
        auto paths = RS_SETTINGS->beginGroupGuard("Paths");
        RS_SETTINGS->writeEntry("OtherSettingsDir", directory.path());
    }
    auto workspace = LC_DockLayoutTestAccess::capture();
    workspace.name = "dock regression";
    workspace.windowWidth = 777;
    workspace.windowHeight = 444;
    workspace.dockAreaBottomActive = false;
    workspace.docAreaFloatingActive = true;
    workspace.dockVisibility["layer_dockwidget"] = true;
    workspace.dockVisibility["command_dockwidget"] = false;
    const auto settings = LC_DockLayoutTestAccess::settingsRoundTrip(workspace);
    CHECK(settings.hasDockVisibility);
    CHECK(settings.dockVisibility == workspace.dockVisibility);
    CHECK(settings.windowWidth == 777);
    CHECK(settings.windowHeight == 444);
    CHECK(settings.preset == workspace.preset);
    const auto named = LC_DockLayoutTestAccess::fileRoundTrip(workspace, false);
    CHECK(named.hasDockVisibility);
    CHECK(named.dockVisibility == workspace.dockVisibility);
    CHECK(named.windowWidth == 777);
    CHECK(named.windowHeight == 444);
    CHECK(named.preset == workspace.preset);
    CHECK(named.docAreaFloatingActive);
    CHECK_FALSE(named.dockAreaBottomActive);
    const auto legacy = LC_DockLayoutTestAccess::fileRoundTrip(workspace, true);
    CHECK_FALSE(legacy.hasDockVisibility);
    CHECK(legacy.windowWidth == 777);
    CHECK(legacy.windowHeight == 444);
    CHECK(legacy.preset == "classic-v1");
    {
        auto paths = RS_SETTINGS->beginGroupGuard("Paths");
        RS_SETTINGS->writeEntry("OtherSettingsDir", lc::test::SettingsIsolation::directory());
    }
}
