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
#include "lc_overlayentitiescontainer.h"
#include "lc_graphicviewport.h"
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
#include "qg_commandwidget.h"
#include "rs_commands.h"
#include "qg_dlgoptionsgeneral.h"
#include "rs_debug.h"
#include "rs_system.h"
#include "qg_blockwidget.h"
#include "qg_pentoolbar.h"
#include "qg_coordinatewidget.h"
#include "qg_selectionwidget.h"
#include "lc_gripinputwidget.h"
#include "rs_fileio.h"
#include "rs_layer.h"
#include "rs_block.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_circle.h"
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

TEST_CASE("Drawing cursor follows actions and clears without touching snap coordinates", "[.gui][dock-window][phase2-cursor]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle();
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    REQUIRE(view->usesDrawingCursor());
    const QPoint point(view->getWidth() / 2, view->getHeight() / 2);
    QMouseEvent move(QEvent::MouseMove, QPointF(point), QPointF(view->mapToGlobal(point)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view, &move);
    settle();
    auto* overlay = view->getViewPort()->getOverlaysDrawablesContainer(RS2::DrawingCursor);
    REQUIRE(overlay->first());
    CHECK(view->cursor().shape() == Qt::BlankCursor);
    const auto idle = view->grab().toImage();
    window.getAction("DrawLine")->trigger();
    settle();
    REQUIRE(overlay->first()); // state switches without another mouse move
    const auto pointEntry = view->grab().toImage();
    CHECK(pointEntry != idle);
    window.slotKillAllActions();
    window.getAction("SelectSingle")->trigger();
    settle();
    REQUIRE(overlay->first());
    CHECK(view->grab().toImage() != pointEntry);
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(view, &leave);
    CHECK(overlay->first() == nullptr);
    view->setMouseCursor(RS2::ClosedHandCursor);
    CHECK(view->cursor().shape() == Qt::ClosedHandCursor);
    CHECK(overlay->first() == nullptr);
    window.slotKillAllActions();
    view->getViewPort()->createUCS({100, 100}, M_PI / 6);
    const RS_Vector accepted = view->getViewPort()->toWorldFromUi(point.x(), point.y());
    QApplication::sendEvent(view, &move);
    CHECK(view->getViewPort()->toWorldFromUi(point.x(), point.y()) == accepted);
    view->getViewPort()->zoomIn();
    REQUIRE(overlay->first());
    CHECK_FALSE(window.getAction("InfoCursorEnable")->isChecked());
}

// Saving the complete preferences dialog also migrates legacy startup options;
// run this acceptance case in its own process rather than changing dock-suite defaults.
TEST_CASE("Cursor preferences survive reload without enabling information tooltips", "[.gui][phase2-preferences]") {
    DockFixture fixture;
    const bool nativeDialogsDisabled = QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
    {
        QG_DlgOptionsGeneral dialog(&fixture.window);
        REQUIRE(dialog.cbDrawingCursor == dialog.findChild<QCheckBox*>("cbDrawingCursor"));
        CHECK(dialog.cbDrawingCursor->isChecked());
        CHECK(dialog.cbCADSelectionStyle->isChecked());
        dialog.cbCADSelectionStyle->setChecked(false);
        dialog.sbHandleSize->setValue(10);
        dialog.sbDrawingPickbox->setValue(18);
        dialog.sbDrawingCrosshair->setValue(25);
        // Preferences may migrate legacy defaults and display notices owned by
        // either the dialog or main window. This process only contains isolated test windows.
        QTimer notice;
        QObject::connect(&notice, &QTimer::timeout, &dialog, [&dialog] {
            for (auto* widget : QApplication::topLevelWidgets()) {
                auto* box = qobject_cast<QMessageBox*>(widget);
                if (box) box->accept();
            }
        });
        notice.start(10);
        REQUIRE(QMetaObject::invokeMethod(&dialog, "ok"));
    }
    {
        QG_DlgOptionsGeneral restored(&fixture.window);
        CHECK(restored.sbDrawingPickbox->value() == 18);
        CHECK(restored.sbDrawingCrosshair->value() == 25);
        CHECK(restored.cbDrawingCursor->isChecked());
        CHECK_FALSE(restored.cbCADSelectionStyle->isChecked());
        CHECK(restored.sbHandleSize->value() == 10);
    }
    CHECK_FALSE(fixture.window.getAction("InfoCursorEnable")->isChecked());
    RS_SETTINGS->writeSingle("Appearance", "DrawingCursorPickboxSize", 10);
    RS_SETTINGS->writeSingle("Appearance", "DrawingCursorCrosshairPercent", 12);
    RS_SETTINGS->writeSingle("Appearance", "CADSelectionStyle", true);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogsDisabled);
}

TEST_CASE("Endpoint snapping and relative polar input remain precise under rotated UCS", "[.gui][dock-window][phase2-precision]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle();
    auto* graphic = window.getActionContext()->getGraphic();
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    RS_Vector endpoint(false);
    for (const auto* entity : *graphic) {
        if (entity->rtti() == RS2::EntityLine) {endpoint = entity->getStartpoint(); break;}
    }
    REQUIRE(endpoint.valid);
    auto* viewport = view->getViewPort();
    graphic->setAnglesCounterClockwise(true);
    graphic->setAnglesBase(0);
    viewport->createUCS({100, 100}, M_PI / 6);
    viewport->zoomAuto();
    RS_SnapMode mode;
    mode.snapEndpoint = true;
    view->setDefaultSnapMode(mode);
    window.getAction("DrawLine")->trigger();
    view->getCurrentAction()->setSnapMode(mode);
    double x, y;
    viewport->toUI(endpoint, x, y);
    const QPointF position(x + 1, y + 1);
    INFO("endpoint UI " << x << ',' << y << " canvas " << view->getWidth() << 'x' << view->getHeight());
    REQUIRE(QRectF(0, 0, view->getWidth(), view->getHeight()).contains(position));
    QMouseEvent move(QEvent::MouseMove, position, QPointF(view->mapToGlobal(position.toPoint())), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view, &move);
    CHECK(view->getCurrentAction()->getSnapMode()->snapEndpoint);
    REQUIRE(viewport->getOverlaysDrawablesContainer(RS2::Snapper)->first());
    REQUIRE(viewport->getOverlaysDrawablesContainer(RS2::DrawingCursor)->first());
    for (const auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
        QMouseEvent event(type, position, QPointF(view->mapToGlobal(position.toPoint())), Qt::LeftButton,
                          type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view, &event);
    }
    auto* command = window.findChild<QG_CommandWidget*>("Command");
    REQUIRE(command);
    CHECK(viewport->getRelativeZero().distanceTo(endpoint) < 1e-8);
    CHECK(viewport->getXAxisAngle() == Catch::Approx(-M_PI / 6));
    command->leCommand->acceptCommand("@25<30");
    window.slotKillAllActions();
    const RS_Vector expected = endpoint + RS_Vector::polar(25, M_PI / 3);
    bool found = false;
    QStringList geometry;
    for (const auto* entity : *graphic) {
        if (entity->rtti() == RS2::EntityLine && !entity->isDeleted()) {
            const auto start = entity->getStartpoint();
            const auto end = entity->getEndpoint();
            geometry << QString("(%1,%2)-(%3,%4)").arg(start.x).arg(start.y).arg(end.x).arg(end.y);
        }
        if (entity->rtti() == RS2::EntityLine && !entity->isDeleted()
            && entity->getStartpoint().distanceTo(endpoint) < 1e-8
            && entity->getEndpoint().distanceTo(expected) < 1e-8) found = true;
    }
    INFO(qPrintable(geometry.join("; ")));
    CHECK(found);
    graphic->setModified(false);
}

TEST_CASE("CAD grip feedback preserves endpoint editing undo and cancellation", "[.gui][dock-window][phase2-selection]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.activateCadWorkspace("cad-familiar-v1", true);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    auto* command = window.findChild<QG_CommandWidget*>("Command");
    REQUIRE(command);
    window.getAction("DrawLine")->trigger();
    command->leCommand->acceptCommand("0,100");
    command->leCommand->acceptCommand("100,100");
    window.slotKillAllActions();
    settle();
    auto* graphic = window.getActionContext()->getGraphic();
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    auto* viewport = view->getViewPort();
    auto draftedLine = [graphic]() -> RS_Line* {
        for (auto* entity : *graphic)
            if (!entity->isDeleted() && entity->rtti() == RS2::EntityLine && std::abs(entity->getEndpoint().y - 100) < 1e-6)
                return static_cast<RS_Line*>(entity);
        return nullptr;
    };
    REQUIRE(draftedLine());
    graphic->select(QList<RS_Entity*>{draftedLine()}, true);
    viewport->zoomAuto();
    view->setDefaultSnapMode(RS_SnapMode{});
    settle();
    auto send = [view](QEvent::Type type, const QPointF& pos, Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, QPointF(view->mapToGlobal(pos.toPoint())), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(view, &event);
    };
    auto ui = [viewport](const RS_Vector& point) {
        double x, y;
        viewport->toUI(point, x, y);
        return QPointF(x, y);
    };
    const QString output = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    auto capture = [&window, &output](const QString& name) {
        if (!output.isEmpty()) {
            REQUIRE(QDir().mkpath(output));
            REQUIRE(window.grab().save(QDir(output).filePath(name)));
        }
    };
    for (const QSize size : {QSize(1280, 800), QSize(1024, 640)}) {
        window.resize(size);
        settle();
        viewport->zoomAuto();
        viewport->zoomOut();
        viewport->zoomOut();
        settle();
        const auto point = ui(draftedLine()->getStartpoint());
        send(QEvent::MouseMove, point, Qt::NoButton, Qt::NoButton);
        settle();
        CHECK(viewport->getOverlaysDrawablesContainer(RS2::ActionPreviewEntity)->first() != nullptr);
        CHECK(window.findChild<QLabel*>("cad_selection_count")->text().contains("1"));
        auto* ribbon = window.findChild<QToolBar*>("cad_ribbon");
        for (auto* control : ribbon->findChildren<QToolButton*>()) {
            if (!control->isVisible()) continue;
            CHECK(ribbon->rect().contains(QRect(control->mapTo(ribbon, QPoint()), control->size())));
        }
        capture(QString("selection-%1x%2-hover.png").arg(size.width()).arg(size.height()));
        send(QEvent::MouseMove, point + QPointF(25, 25), Qt::NoButton, Qt::NoButton);
        CHECK(viewport->getOverlaysDrawablesContainer(RS2::ActionPreviewEntity)->first() == nullptr);
    }
    const RS_Vector original = draftedLine()->getStartpoint();
    const auto start = ui(original);
    const QPointF destination = start + QPointF(60, 35);
    const RS_Vector expected = viewport->toWorldFromUi(destination.x(), destination.y());
    auto startDrag = [&] {
        send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        send(QEvent::MouseMove, start + QPointF(15, 15), Qt::NoButton, Qt::LeftButton);
        send(QEvent::MouseMove, destination, Qt::NoButton, Qt::LeftButton);
        settle();
        CHECK(viewport->getOverlaysDrawablesContainer(RS2::ActionPreviewEntity)->first() != nullptr);
    };
    startDrag();
    capture("selection-active-grip.png");
    send(QEvent::MouseButtonPress, destination, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, destination, Qt::LeftButton, Qt::NoButton);
    REQUIRE(draftedLine());
    CHECK(draftedLine()->getStartpoint().distanceTo(expected) < 1e-6);
    window.getAction("EditUndo")->trigger();
    REQUIRE(draftedLine());
    CHECK(draftedLine()->getStartpoint().distanceTo(original) < 1e-6);
    graphic->select(QList<RS_Entity*>{draftedLine()}, true);
    startDrag();
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(view, &escape);
    CHECK(draftedLine()->getStartpoint().distanceTo(original) < 1e-6);
    CHECK(viewport->getOverlaysDrawablesContainer(RS2::ActionPreviewEntity)->first() == nullptr);
    graphic->setModified(false);
}

TEST_CASE("Click grip placement accepts dimensional input and Tab without committing early", "[.gui][dock-window][grip-input]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.activateCadWorkspace("cad-familiar-v1", true);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    auto* command = window.findChild<QG_CommandWidget*>("Command");
    window.getAction("DrawLine")->trigger();
    command->leCommand->acceptCommand("0,100");
    command->leCommand->acceptCommand("100,100");
    window.slotKillAllActions();
    settle();
    auto* graphic = window.getActionContext()->getGraphic();
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    auto* viewport = view->getViewPort();
    graphic->setAnglesCounterClockwise(true);
    graphic->setAnglesBase(0);
    viewport->getFormatter()->updateByGraphic(graphic);
    auto line = [graphic]() -> RS_Line* {
        for (auto* entity : *graphic)
            if (!entity->isDeleted() && entity->rtti() == RS2::EntityLine && std::abs(entity->getEndpoint().y - 100) < 1e-6)
                return static_cast<RS_Line*>(entity);
        return nullptr;
    };
    REQUIRE(line());
    RS_Entity* stationary = nullptr;
    for (auto* entity : *graphic) if (entity->rtti() == RS2::EntityCircle) { stationary = entity; break; }
    REQUIRE(stationary);
    const auto center = stationary->getCenter();
    graphic->select(QList<RS_Entity*>{line(), stationary}, true);
    viewport->zoomAuto();
    viewport->zoomOut();
    view->setDefaultSnapMode(RS_SnapMode{});
    settle();
    auto ui = [viewport](const RS_Vector& point) {
        double x, y; viewport->toUI(point, x, y); return QPointF(x, y);
    };
    auto click = [view](const QPointF& position, bool doubleClick = false) {
        for (const auto type : {doubleClick ? QEvent::MouseButtonDblClick : QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
            QMouseEvent event(type, position, QPointF(view->mapToGlobal(position.toPoint())), Qt::LeftButton,
                              type != QEvent::MouseButtonRelease ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(view, &event);
        }
    };
    auto key = [](QWidget* target, int code, const QString& text = {}, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, code, modifiers, text);
        QApplication::sendEvent(target, &event);
    };
    auto type = [&](QLineEdit* field, const QString& text) {
        field->selectAll();
        for (const auto& letter : text) key(field, letter.unicode(), QString(letter));
    };
    const RS_Vector original = line()->getStartpoint();
    click(ui(original));
    auto* input = view->findChild<LC_GripInputWidget*>("cad_grip_input");
    REQUIRE(input);
    CHECK(input->isVisible());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-9); // first click/release only picks up
    QEvent leaveToEditor(QEvent::Leave);
    QApplication::sendEvent(view, &leaveToEditor);
    CHECK(viewport->getOverlaysDrawablesContainer(RS2::ActionPreviewEntity)->first() != nullptr);
    auto* length = input->findChild<QLineEdit*>("cad_grip_length");
    auto* angle = input->findChild<QLineEdit*>("cad_grip_angle");
    REQUIRE(length);
    REQUIRE(angle);
    type(length, "150");
    key(length, Qt::Key_Tab);
    CHECK(angle->hasFocus());
    type(angle, "0");
    key(angle, Qt::Key_Tab);
    CHECK(length->hasFocus());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-9);
    settle();
    const QString output = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    if (!output.isEmpty()) {
        REQUIRE(QDir().mkpath(output));
        REQUIRE(window.grab().save(QDir(output).filePath("grip-length-angle.png")));
    }
    click(ui(original) + QPointF(40, 25), true);
    REQUIRE(line());
    CHECK(line()->getLength() == Catch::Approx(150));
    CHECK(stationary->getCenter() == center);
    CHECK(stationary->isSelected());
    CHECK(line()->getStartpoint().distanceTo({-50, 100}) < 1e-8);
    CHECK(input->isHidden());
    window.getAction("EditUndo")->trigger();
    REQUIRE(line());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-9);
    graphic->select(QList<RS_Entity*>{line()}, true);
    viewport->createUCS({10, 20}, M_PI / 6);
    click(ui(original));
    type(length, "80+20");
    key(length, Qt::Key_Tab);
    type(angle, "30");
    key(angle, Qt::Key_Return);
    const auto expected = RS_Vector(100, 100) - RS_Vector::polar(100, M_PI / 3);
    CHECK(line()->getStartpoint().distanceTo(expected) < 1e-8);
    CHECK(input->isHidden());
    window.getAction("EditUndo")->trigger();
    graphic->select(QList<RS_Entity*>{line()}, true);
    click(ui(original));
    type(length, "-1");
    key(length, Qt::Key_Return);
    CHECK(input->isVisible());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-8);
    type(length, "0");
    key(length, Qt::Key_Return);
    CHECK(input->isVisible());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-8);
    key(length, Qt::Key_Escape);
    CHECK(input->isHidden());
    CHECK(line()->getStartpoint().distanceTo(original) < 1e-8);
    auto* unchanged = line();
    graphic->setModified(false);
    click(ui(original));
    type(length, "100");
    key(length, Qt::Key_Tab);
    type(angle, QString::number(viewport->getFormatter()->toUCSBasisAngleDegrees(line()->getAngle1()), 'g', 12));
    INFO("unchanged grip length=" << qPrintable(length->text()) << " angle=" << qPrintable(angle->text()));
    key(angle, Qt::Key_Return);
    CHECK(input->isHidden());
    CHECK(line() == unchanged);
    CHECK_FALSE(graphic->isModified());
    click(ui(original));
    window.getAction("DrawLine")->trigger();
    CHECK(input->isHidden()); // changing command aborts the grip preview
    window.slotKillAllActions();
    QList<RS_Entity*> selected;
    graphic->collectSelected(selected);
    graphic->select(selected, false);
    auto* polyline = new RS_Polyline(graphic, RS_PolylineData());
    polyline->addVertex({150, 50});
    polyline->addVertex({200, 50});
    polyline->addVertex({200, 90});
    polyline->setLayer(graphic->getActiveLayer());
    graphic->addEntity(polyline);
    graphic->select(QList<RS_Entity*>{polyline}, true);
    viewport->zoomAuto();
    viewport->zoomOut();
    settle();
    click(ui(polyline->getStartpoint()));
    CHECK(input->isVisible());
    type(length, "40");
    key(length, Qt::Key_Tab);
    type(angle, QString::number(viewport->getFormatter()->toUCSBasisAngleDegrees(0), 'g', 12));
    key(angle, Qt::Key_Return);
    RS_Polyline* editedPolyline = nullptr;
    for (auto* entity : *graphic)
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityPolyline) editedPolyline = static_cast<RS_Polyline*>(entity);
    REQUIRE(editedPolyline);
    CHECK(editedPolyline->getStartpoint().distanceTo({160, 50}) < 1e-8);
    graphic->collectSelected(selected);
    graphic->select(selected, false);
    graphic->select(QList<RS_Entity*>{stationary}, true);
    const auto* circle = static_cast<RS_Circle*>(stationary);
    viewport->zoomAuto();
    viewport->zoomOut();
    settle();
    click(ui(center + RS_Vector(circle->getRadius(), 0)));
    CHECK(input->isVisible());
    type(length, "15");
    key(length, Qt::Key_Tab);
    type(angle, QString::number(viewport->getFormatter()->toUCSBasisAngleDegrees(0), 'g', 12));
    key(angle, Qt::Key_Return);
    RS_Circle* editedCircle = nullptr;
    for (auto* entity : *graphic)
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityCircle) editedCircle = static_cast<RS_Circle*>(entity);
    REQUIRE(editedCircle);
    CHECK(editedCircle->getCenter() == center);
    CHECK(editedCircle->getRadius() == Catch::Approx(15));
    graphic->setModified(false);
}

TEST_CASE("Press drag release draws a freehand lasso and Escape clears it", "[.gui][dock-window][lasso-input]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.activateCadWorkspace("cad-familiar-v1", true);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle();
    auto* graphic = window.getActionContext()->getGraphic();
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    auto* viewport = view->getViewPort();
    viewport->zoomAuto();
    auto send = [view](QEvent::Type type, const RS_Vector& point, Qt::MouseButton button, Qt::MouseButtons buttons) {
        double x, y; view->getViewPort()->toUI(point, x, y);
        QMouseEvent event(type, QPointF(x, y), QPointF(view->mapToGlobal(QPoint(qRound(x), qRound(y)))), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(view, &event);
    };
    RS_Entity* target = nullptr;
    for (auto* entity : *graphic) if (entity->rtti() == RS2::EntityCircle) {target = entity; break;}
    REQUIRE(target);
    const auto min = target->getMin() - RS_Vector(2, 2);
    const auto max = target->getMax() + RS_Vector(2, 2);
    const double w = max.x - min.x, h = max.y - min.y;
    const RS_Vector first(min.x, min.y + h * .3);
    const QList<RS_Vector> points{first, {min.x + w * .2, min.y}, {max.x - w * .2, min.y},
        {max.x, min.y + h * .25}, {max.x, max.y - h * .2}, {max.x - w * .2, max.y},
        {min.x + w * .1, max.y}, {min.x, max.y - h * .2}, first};
    auto draw = [&] {
        send(QEvent::MouseButtonPress, points.first(), Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i < points.size(); ++i) send(QEvent::MouseMove, points[i], Qt::NoButton, Qt::LeftButton);
    };
    draw();
    CHECK(viewport->getOverlaysDrawablesContainer(RS2::OverlayEffects)->first() != nullptr);
    const QString output = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    if (!output.isEmpty()) {
        REQUIRE(QDir().mkpath(output));
        REQUIRE(window.grab().save(QDir(output).filePath("lasso-preview.png")));
    }
    send(QEvent::MouseButtonRelease, points.last(), Qt::LeftButton, Qt::NoButton);
    CHECK(target->isSelected());
    CHECK(viewport->getOverlaysDrawablesContainer(RS2::OverlayEffects)->first() == nullptr);
    graphic->select(QList<RS_Entity*>{target}, false);
    draw();
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(view, &escape);
    CHECK_FALSE(target->isSelected());
    CHECK(viewport->getOverlaysDrawablesContainer(RS2::OverlayEffects)->first() == nullptr);
    graphic->setModified(false);
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

TEST_CASE("Bottom command suggestions preserve focus drafting and history", "[.gui][dock-window][phase2-input]") {
    DockFixture fixture;
    auto& window = fixture.window;
    window.resize(1280, 800);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    settle();
    auto* command = window.findChild<QG_CommandWidget*>("Command");
    REQUIRE(command);
    auto* popup = command->findChild<QTreeWidget*>("command_suggestions");
    REQUIRE(popup);
    auto* edit = command->leCommand;
    QStringList submissions;
    QObject::connect(edit, &QG_CommandEdit::command, &window, [&](const QString& text) {submissions << text;});
    const auto key = [&](int value, const QString& text = {}) {
        QKeyEvent event(QEvent::KeyPress, value, Qt::NoModifier, text);
        QApplication::sendEvent(edit, &event);
        settle(20);
    };
    const auto type = [&](const QString& text) {
        edit->clear();
        // Synthetic key delivery still requires a Qt-active owner. The terminal
        // or another application may own macOS focus while this harness runs.
        window.raise();
        window.QWidget::activateWindow();
        QApplication::setActiveWindow(&window);
        command->focusWidget();
        settle();
        QApplication::setActiveWindow(&window);
        edit->setFocus();
        for (const auto& letter : text) key(letter.unicode(), QString(letter));
    };
    type("li");
    INFO("input=" << qPrintable(edit->text()) << " focus=" << edit->hasFocus()
         << " eligible=" << edit->allowsCompletion() << " active action=" << window.getActionContext()->getGraphicView()->hasAction()
         << " default status=" << window.getActionContext()->getGraphicView()->getDefaultAction()->getStatus()
         << " rows=" << popup->topLevelItemCount());
    REQUIRE(popup->isVisible());
    CHECK(edit->hasFocus());
    CHECK(popup->currentItem()->data(0, Qt::UserRole).toString() == "li");
    key(Qt::Key_Down);
    CHECK(popup->indexOfTopLevelItem(popup->currentItem()) == 1);
    key(Qt::Key_Up);
    key(Qt::Key_Escape);
    CHECK_FALSE(popup->isVisible());
    CHECK(edit->text() == "li");
    key(Qt::Key_Tab);
    REQUIRE(popup->isVisible());
    key(Qt::Key_Tab);
    CHECK_FALSE(popup->isVisible());
    CHECK(submissions.isEmpty());
    key(Qt::Key_Return);
    REQUIRE(submissions.size() == 1);
    CHECK(submissions.last() == "li");
    CHECK(window.getActionContext()->getGraphicView()->getCurrentAction()->rtti() == RS2::ActionDrawLine);
    type("0,100");
    CHECK_FALSE(popup->isVisible());
    key(Qt::Key_Return);
    type("@100,0");
    CHECK_FALSE(popup->isVisible());
    key(Qt::Key_Return);
    type("close"); // active action options/arguments must not be global suggestions
    CHECK_FALSE(popup->isVisible());
    window.slotKillAllActions();
    edit->clear();
    key(Qt::Key_Up);
    CHECK(edit->text() == "@100,0");
    CHECK_FALSE(popup->isVisible());
    type("li");
    REQUIRE(popup->isVisible());
    const QRect item = popup->visualItemRect(popup->currentItem());
    for (const auto eventType : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
        QMouseEvent event(eventType, QPointF(item.center()), QPointF(popup->viewport()->mapToGlobal(item.center())),
                          Qt::LeftButton, eventType == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(popup->viewport(), &event);
    }
    CHECK_FALSE(popup->isVisible());
    CHECK(edit->hasFocus());
    CHECK(submissions.size() == 3); // mouse inserts a candidate; Enter performs execution
    type("li");
    REQUIRE(popup->isVisible());
    key(Qt::Key_Return);
    CHECK(submissions.size() == 4);
    CHECK_FALSE(popup->isVisible());
    window.slotKillAllActions();
    type("li");
    window.getAction("DrawLine")->trigger();
    CHECK_FALSE(popup->isVisible());
    window.slotKillAllActions();
    QTemporaryDir aliases;
    REQUIRE(aliases.isValid());
    QFile aliasFile(aliases.filePath("librecad.alias"));
    REQUIRE(aliasFile.open(QIODevice::WriteOnly));
    REQUIRE(aliasFile.write("phase2circle\tcircle\nPhase2case\tline2p\nphase2CASE\tcircle\nphase2sequence;\tcircle\n") > 0);
    aliasFile.close();
    RS_SETTINGS->writeSingle("Paths", "OtherSettingsDir", aliases.path());
    RS_COMMANDS->updateAlias();
    type("phase2sequence");
    CHECK_FALSE(popup->isVisible());
    type("phase2circle");
    REQUIRE(popup->isVisible());
    CHECK(popup->currentItem()->text(0).contains("circle"));
    key(Qt::Key_Return);
    CHECK(window.getActionContext()->getGraphicView()->getCurrentAction()->rtti() == RS2::ActionDrawCircleCenterPoint);
    key(Qt::Key_Escape);
    CHECK_FALSE(window.getActionContext()->getGraphicView()->hasAction());
    type("phase2CASE");
    REQUIRE(popup->isVisible());
    CHECK(popup->currentItem()->data(0, Qt::UserRole).toString() == "phase2CASE");
    key(Qt::Key_Return);
    CHECK(submissions.last() == "phase2CASE");
    CHECK(window.getActionContext()->getGraphicView()->getCurrentAction()->rtti() == RS2::ActionDrawCircleCenterPoint);
    key(Qt::Key_Escape);
    RS_SETTINGS->writeSingle("Paths", "OtherSettingsDir", lc::test::SettingsIsolation::directory());
    type("li");
    REQUIRE(popup->isVisible());
    const int beforeSpace = submissions.size();
    key(Qt::Key_Space, " ");
    CHECK(submissions.size() == beforeSpace + 1);
    CHECK(window.getActionContext()->getGraphicView()->getCurrentAction()->rtti() == RS2::ActionDrawLine);
    key(Qt::Key_Escape);
    type("li");
    REQUIRE(popup->isVisible());
    window.getActionContext()->getGraphic()->setModified(false);
    window.getAction("FileNew")->trigger();
    settle();
    CHECK_FALSE(popup->isVisible());
}

TEST_CASE("Capture Phase 2 cursor and command input at laptop sizes", "[.baseline][phase2-capture]") {
    const QString output = qEnvironmentVariable("LIBRECAD_BASELINE_OUTPUT");
    REQUIRE_FALSE(output.isEmpty());
    REQUIRE(QDir().mkpath(output));
    DockFixture fixture;
    auto& window = fixture.window;
    window.activateCadWorkspace("cad-familiar-v1", true);
    window.openFile(QString(LIBRECAD_SOURCE_DIR) + "/tests/fixtures/cad-familiar/model-basics-r2018.dxf");
    auto* command = window.findChild<QG_CommandWidget*>("Command");
    REQUIRE(command);
    auto* popup = command->findChild<QTreeWidget*>("command_suggestions");
    REQUIRE(popup);
    auto* view = dynamic_cast<QG_GraphicView*>(window.getActionContext()->getGraphicView());
    REQUIRE(view);
    QJsonArray captures;
    for (const QSize size : {QSize(1280, 800), QSize(1024, 640)}) {
        window.resize(size);
        settle();
        CHECK(window.size() == size);
        CHECK(command->isVisible());
        CHECK(command->leCommand->width() >= 120);
        const QPoint pos(view->getWidth() * 2 / 3, view->getHeight() / 2);
        QMouseEvent move(QEvent::MouseMove, QPointF(pos), QPointF(view->mapToGlobal(pos)), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view, &move);
        const QString prefix = QString("input-%1x%2-").arg(size.width()).arg(size.height());
        command->leCommand->clear();
        for (const QString& state : {QString("idle"), QString("point"), QString("selection")}) {
            window.slotKillAllActions();
            if (state == "point") window.getAction("DrawLine")->trigger();
            if (state == "selection") window.getAction("SelectSingle")->trigger();
            settle();
            REQUIRE(view->getViewPort()->getOverlaysDrawablesContainer(RS2::DrawingCursor)->first());
            REQUIRE(window.grab().save(QDir(output).filePath(prefix + state + ".png")));
        }
        window.slotKillAllActions();
        command->leCommand->clear();
        window.raise();
        window.QWidget::activateWindow();
        QApplication::setActiveWindow(&window);
        command->focusWidget();
        settle();
        QApplication::setActiveWindow(&window);
        command->leCommand->setFocus();
        for (const auto& text : {QString("l"), QString("i")}) {
            QKeyEvent key(QEvent::KeyPress, text[0].unicode(), Qt::NoModifier, text);
            QApplication::sendEvent(command->leCommand, &key);
        }
        settle();
        INFO("capture input=" << qPrintable(command->leCommand->text()) << " focus=" << command->leCommand->hasFocus()
             << " eligible=" << command->leCommand->allowsCompletion() << " action=" << view->hasAction()
             << " default status=" << view->getDefaultAction()->getStatus() << " rows=" << popup->topLevelItemCount());
        REQUIRE(popup->isVisible());
        CHECK(command->leCommand->hasFocus());
        CHECK(popup->screen()->availableGeometry().contains(popup->geometry()));
        auto image = window.grab();
        // Capture both actual native Qt surfaces; top-level tool windows are not included by QWidget::grab().
        QPainter painter(&image);
        painter.drawPixmap(window.mapFromGlobal(popup->pos()), popup->grab());
        painter.end();
        REQUIRE(image.save(QDir(output).filePath(prefix + "suggestions.png")));
        captures.append(QJsonObject{{"width", size.width()}, {"height", size.height()},
            {"canvas_width", view->getWidth()}, {"canvas_height", view->getHeight()}, {"dpr", view->devicePixelRatioF()},
            {"input_width", command->leCommand->width()}, {"suggestions", popup->topLevelItemCount()}});
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(command->leCommand, &escape);
    }
    QFile metadata(QDir(output).filePath("input-captures.json"));
    REQUIRE(metadata.open(QIODevice::WriteOnly));
    REQUIRE(metadata.write(QJsonDocument(captures).toJson()) > 0);
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
    auto* tabs = ribbon->findChild<QTabWidget*>("cad_ribbon_tabs");
    REQUIRE(tabs);
    CHECK(tabs->count() == 5);
    tabs->setCurrentIndex(1);
    CHECK(ribbon->findChild<QToolButton*>("cad_action_BlocksCreate")->isVisible());
    CHECK(ribbon->findChild<QToolButton*>("cad_action_BlocksCreate")->defaultAction() == window.getAction("BlocksCreate"));
    tabs->setCurrentIndex(2);
    CHECK(ribbon->findChild<QToolButton*>("cad_action_DimRadial")->isVisible());
    tabs->setCurrentIndex(0);
    CHECK(quick->actions().contains(window.getAction("FileSave")));
    auto* strip = window.findChild<QWidget*>("cad_status_strip");
    REQUIRE(strip);
    CHECK(strip->isVisible());
    CHECK(window.statusBar()->height() <= 36);
    CHECK(window.findChild<QToolBar*>("snap_toolbar")->isHidden());
    auto* ortho = strip->findChild<QToolButton*>("cad_action_RestrictOrthogonal");
    REQUIRE(ortho);
    CHECK(ortho->defaultAction() == window.getAction("RestrictOrthogonal"));
    auto* snaps = strip->findChild<QToolButton*>("cad_object_snaps");
    REQUIRE(snaps);
    CHECK(snaps->menu()->actions().contains(window.getAction("SnapEnd")));
    window.findChild<QG_SelectionWidget*>("selections")->setNumber(3);
    CHECK(strip->findChild<QLabel*>("cad_selection_count")->text().contains("3"));
    window.findChild<QG_CoordinateWidget*>("coordinates")->clearContent();
    CHECK(strip->findChild<QLabel*>("cad_coordinates")->text() == "0 , 0");
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
    CHECK(strip->isHidden());
    window.activateCadWorkspace("cad-familiar-v1");
    settle();
    CHECK(properties->isHidden());
    CHECK(strip->isVisible());
    CHECK(window.findChild<QToolBar*>("snap_toolbar")->isHidden());
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
    if (!captureOutput.isEmpty()) REQUIRE(QDir().mkpath(captureOutput));
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
    window.getAction("EditRedo")->trigger();
    window.getAction("EditRedo")->trigger();
    settle();
    CHECK(draftedLine()->getLength() == Catch::Approx(120));
    CHECK(draftedLine()->getLayer()->getName() == "ANNOTATION");
    window.getAction("EditUndo")->trigger();
    window.getAction("EditUndo")->trigger();
    settle();
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
