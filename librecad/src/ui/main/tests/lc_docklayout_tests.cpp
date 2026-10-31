// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 librecad.org
// Copyright (C) 2026 Dongxu Li (github.com/dxli)

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>

#include "lc_actiontestsupport.h"
#include "lc_offsetcurve.h"
#include "lc_offsetimportsession.h"
#include "lc_settingguard.h"
#include "lc_testsettingsisolation.h"
#include "lc_workspacesmanager.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "qg_layerwidget.h"
#include "rs_debug.h"
#include "rs_layer.h"
#include "rs_system.h"
#include "qg_blockwidget.h"
#include "rs_block.h"
#include "rs_line.h"
#include "rs_spline.h"
#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <unistd.h>
#endif

struct LC_GuiImportTestAccess {
    static void onStarted(LC_OffsetImportSession& session, std::function<void(QProcess&)> hook) {
        session.m_worker.m_testStarted = std::move(hook);
    }
    static void afterRead(LC_OffsetImportSession& session, std::function<void()> hook) {
        session.m_testAfterRead = std::move(hook);
    }
};

struct LC_DockLayoutTestAccess {
    using Workspace = LC_WorkspacesManager::LC_Workspace;
    static QC_ApplicationWindow* createWindow() { return new QC_ApplicationWindow; }
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
    static QStringList collapsedGroups(QC_ApplicationWindow& window) { return window.m_autoCollapsedGroups; }
    static int surfaceBlocks(QC_ApplicationWindow& window, RS_Graphic& graphic) {
        lc::test::TestGraphicView view;
        view.setDocument(&graphic);
        window.m_blockWidget->setGraphicView(&view);
        const int count = window.maybeSurfaceBlocksDock(&graphic);
        window.m_blockWidget->setGraphicView(nullptr);
        return count;
    }
    static int drawingCount(QC_ApplicationWindow& window) { return window.findChildren<QC_MDIWindow*>().size(); }
    static int pendingImports(QC_ApplicationWindow& window) { return window.m_pendingImports.size() + window.m_queuedImports.size(); }
    static int activeImports(QC_ApplicationWindow& window) { return window.m_pendingImports.size(); }
    static int queuedImports(QC_ApplicationWindow& window) { return window.m_queuedImports.size(); }
    static int maxActiveImports() { return QC_ApplicationWindow::MaxActiveImports; }
    static int maxQueuedImports() { return QC_ApplicationWindow::MaxQueuedImports; }
    static QString queuedFile(QC_ApplicationWindow& window, int index) { return window.m_queuedImports.at(index).fileName; }
    static bool queuedTemplate(QC_ApplicationWindow& window, int index) { return window.m_queuedImports.at(index).asTemplate; }
    static void cancelImports(QC_ApplicationWindow& window) { window.cancelPendingImports(); }
    static void closeDrawing(QC_ApplicationWindow& window, QC_MDIWindow* drawing) {
        drawing->setSaveOnClosePolicy(QC_MDIWindow::DONT_SAVE);
        window.doClose(drawing);
    }
    static bool newTemplate(QC_ApplicationWindow& window, const QString& file) { return window.newDrawingFromTemplate(file); }
    static bool tracksOpenedFile(QC_ApplicationWindow& window, const QString& file) { return window.m_openedFiles.contains(file); }
    static void closeDrawings(QC_ApplicationWindow& window) {
        const auto drawings = window.findChildren<QC_MDIWindow*>();
        for (auto* drawing : drawings) {
            drawing->setSaveOnClosePolicy(QC_MDIWindow::DONT_SAVE);
            window.doClose(drawing);
        }
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    static LC_OffsetImportSession* pendingImport(QC_ApplicationWindow& window) {
        return window.m_pendingImports.isEmpty() ? nullptr : window.m_pendingImports.first().data();
    }
    static LC_OffsetImportSession* lastPendingImport(QC_ApplicationWindow& window) {
        return window.m_pendingImports.isEmpty() ? nullptr : window.m_pendingImports.last().data();
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
        QSettings settings;
        settings.setValue("Startup/FirstLoad", true);
        settings.setValue("Defaults/AutoBackupDocument", false);
        settings.setValue("Paths/OtherSettingsDir", lc::test::SettingsIsolation::directory());
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

struct DrawingWindowCleanup {
    QC_ApplicationWindow& window;
    QC_MDIWindow* drawing;
    ~DrawingWindowCleanup() {
        if (drawing != nullptr) LC_DockLayoutTestAccess::closeDrawing(window, drawing);
    }
};

class PopupObserver final : public QObject {
public:
    bool shown {false};
    int actionCount {0};

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            if (auto* menu = qobject_cast<QMenu*>(watched)) {
                shown = true;
                actionCount = menu->actions().size();
                QTimer::singleShot(0, menu, &QMenu::close);
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

// A window as main() builds it on the first start, shown at a realistic size. The offscreen
// platform has an 800x600 screen unless given a larger one: run these tests with
// QT_QPA_PLATFORM=offscreen:configfile=librecad/src/ui/main/tests/lc_docklayout_screen.json
struct FirstStart {
    QC_ApplicationWindow& window {testWindow()};
    ~FirstStart() { window.hide(); }
    void show(QSize size, bool maximized = false) {
        window.prepareWindowForShow();
        window.resize(size);
        if (maximized) window.showMaximized();
        else window.show();
        settle();
    }
    QDockWidget* dock(const char* name) {
        auto* dock = window.findChild<QDockWidget*>(name);
        REQUIRE(dock != nullptr);
        return dock;
    }
    bool shown(QDockWidget* dock) const {
        return !dock->isHidden() && window.rect().intersects(dock->geometry());
    }
    int rightStrip() const { return window.width() - window.centralWidget()->geometry().right() - 1; }
};

void requireScreen(QSize size) {
    REQUIRE(lc::test::application() != nullptr);
    const QScreen* screen = QGuiApplication::primaryScreen();
    REQUIRE(screen != nullptr);
    const QSize available = screen->availableGeometry().size();
    if (available.width() < size.width() + 8 || available.height() < size.height() + 8)
        SKIP("needs a screen larger than " << size.width() << 'x' << size.height()
             << ", see lc_docklayout_screen.json");
}

void checkDefaultRightDocks(FirstStart& start) {
    auto& window = start.window;
    INFO("window=" << window.width() << 'x' << window.height()
         << " canvas=" << window.getMdiArea()->width() << 'x' << window.getMdiArea()->height()
         << " strip=" << start.rightStrip()
         << " collapsed=" << LC_DockLayoutTestAccess::collapsedGroups(window).join(',').toStdString());
    CHECK(start.shown(start.dock("layer_dockwidget")));
    CHECK(start.shown(start.dock("command_dockwidget")));
    CHECK(LC_DockLayoutTestAccess::collapsedGroups(window).isEmpty());
    CHECK(start.rightStrip() * 4 <= window.width());
    CHECK(window.getMdiArea()->width() >= 480);
    CHECK(window.getMdiArea()->height() >= 320);
}

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

TEST_CASE("Idle right-button release reaches graphic-view popup", "[.gui][dock-window][context-menu]") {
    DockFixture fixture;
    auto& window = fixture.window;
    auto* drawing = window.createNewDrawingWindow(nullptr, {});
    REQUIRE(drawing != nullptr);
    DrawingWindowCleanup cleanup{window, drawing};
    drawing->show();
    window.activateWindow(drawing);
    settle();

    auto* view = drawing->getGraphicView();
    REQUIRE(view != nullptr);
    const QPoint local(20, 20);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(local), QPointF(view->mapToGlobal(local)),
                        Qt::RightButton, Qt::NoButton, Qt::NoModifier);

    std::unique_ptr<QMenu> menu(window.createGraphicViewContentMenu(&release, view, nullptr, RS_Vector{}));
    REQUIRE(menu != nullptr);
    REQUIRE(menu->actions().size() > 1);
    menu.reset();

    PopupObserver observer;
    qApp->installEventFilter(&observer);
    QApplication::sendEvent(view, &release);
    qApp->removeEventFilter(&observer);

    CHECK(observer.shown);
    CHECK(observer.actionCount > 1);
}

TEST_CASE("Print does not inherit the print preview checked state", "[.gui][printing][issue3105]") {
    DockFixture fixture;
    REQUIRE(fixture.window.getCurrentMDIWindow() == nullptr);
    QAction* print = fixture.window.getAction("FilePrint");
    QAction* preview = fixture.window.getAction("FilePrintPreview");
    REQUIRE(print != nullptr);
    REQUIRE(preview != nullptr);
    print->setEnabled(true);

    QObject receiver;
    int triggered = 0;
    bool checked = true;
    QObject::connect(print, &QAction::triggered, &receiver, [&](bool value) {
        ++triggered;
        checked = value;
    });
    for (const bool active : {false, true, false, true}) {
        CAPTURE(active);
        fixture.window.printPreviewChanged(active);
        CHECK(preview->isChecked() == active);
        CHECK_FALSE(print->isCheckable());
        print->trigger();
        CHECK_FALSE(checked);
    }
    CHECK(triggered == 4);
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

TEST_CASE("Chinese UI translations preserve default keyboard shortcuts", "[gui][issue3135]") {
    REQUIRE(lc::test::application() != nullptr);
    const QVariant oldTranslations = RS_SETTINGS->getSettings()->value("/Paths/Translations");
    LC_SET_ONE("Paths", "Translations", QStringLiteral(LIBRECAD_TEST_TRANSLATION_DIR));

    const std::pair<const char*, QKeySequence> shortcuts[] = {
        {"DeselectAll", QKeySequence(Qt::CTRL | Qt::Key_K)},
        {"ViewGrid", QKeySequence(Qt::CTRL | Qt::Key_G)},
        {"ViewDraft", QKeySequence(Qt::CTRL | Qt::Key_D)},
        {"ViewStatusBar", QKeySequence(Qt::CTRL | Qt::Key_I)},
    };
    for (const QString& locale : {QStringLiteral("zh_CN"), QStringLiteral("zh_TW")}) {
        RS_SYSTEM->loadTranslation(locale, "en");
        DockFixture fixture;
        for (const auto& [actionName, expected] : shortcuts) {
            QAction* action = fixture.window.getAction(actionName);
            CHECK(action != nullptr);
            if (action != nullptr) {
                CHECK(action->shortcut() == expected);
            }
        }
    }

    RS_SYSTEM->loadTranslation("en", "en");
    const auto groupGuard = RS_SETTINGS->beginGroupGuard("Paths");
    if (oldTranslations.isValid()) {
        RS_SETTINGS->write("Translations", oldTranslations);
    }
    else {
        RS_SETTINGS->remove("Translations");
    }
}

// QObject::receivers() is protected; a member pointer formed through a subclass reaches it.
struct SignalReceivers : QObject {
    template <typename Signal>
    static int count(const QObject* sender, Signal signal) {
        const QByteArray signature = QByteArray::number(QSIGNAL_CODE) + QMetaMethod::fromSignal(signal).methodSignature();
        return (sender->*&SignalReceivers::receivers)(signature.constData());
    }
};

TEST_CASE("A replaced main window leaves no listeners on the settings", "[gui][settings-listeners]") {
    testWindow();
    const int options = SignalReceivers::count(RS_SETTINGS, &RS_Settings::optionsChanged);
    const int option = SignalReceivers::count(RS_SETTINGS, &RS_Settings::optionChanged);

    // destroys the window above and builds another
    testWindow();
    REQUIRE(SignalReceivers::count(RS_SETTINGS, &RS_Settings::optionsChanged) == options);
    REQUIRE(SignalReceivers::count(RS_SETTINGS, &RS_Settings::optionChanged) == option);
    // a listener left by the destroyed window crashed here
    RS_SETTINGS->emitOptionsChanged();
}

TEST_CASE("Opening a drawing keeps its saved current layer", "[.gui][dock-window][saved-current-layer]") {
    DockFixture fixture;
    auto& window = fixture.window;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    for (const int flags : {0, 4, 1, 5}) {
        CAPTURE(flags);
        const QString fileName = directory.filePath(QString("layer-%1.dxf").arg(flags));
        QFile file(fileName);
        REQUIRE(file.open(QIODevice::WriteOnly));
        const QByteArray dxf =
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CLAYER\n8\nSaved\n"
            "0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n2\n"
            "0\nLAYER\n5\n10\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "0\nLAYER\n5\n11\n2\nSaved\n70\n" + QByteArray::number(flags) +
            "\n62\n1\n6\nCONTINUOUS\n0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nLINE\n5\n20\n8\nSaved\n"
            "10\n0\n20\n0\n11\n10\n21\n0\n0\nENDSEC\n0\nEOF\n";
        REQUIRE(file.write(dxf) == dxf.size());
        file.close();

        window.openFile(fileName);
        QElapsedTimer elapsed; elapsed.start();
        while (LC_DockLayoutTestAccess::pendingImports(window)) {
            REQUIRE(elapsed.elapsed() < 35000); settle(10);
        }
        auto* mdi = window.getCurrentMDIWindow();
        REQUIRE(mdi != nullptr);
        auto* graphic = mdi->getGraphic();
        REQUIRE(graphic != nullptr);
        const QString expectedName = (flags & 1) ? "0" : "Saved";
        auto* expected = graphic->findLayer(expectedName);
        REQUIRE(expected != nullptr);
        auto* saved = graphic->findLayer("Saved");
        REQUIRE(saved != nullptr);
        CHECK(graphic->getActiveLayer() == expected);
        CHECK(saved->isLocked() == bool(flags & 4));
        CHECK(saved->isFrozen() == bool(flags & 1));
        CHECK_FALSE(graphic->getActiveLayer()->isFrozen());

        auto* label = window.findChild<QLabel*>("lActiveLayerName");
        REQUIRE(label != nullptr);
        CHECK(label->text() == expectedName);
        auto* widget = window.findChild<QG_LayerWidget*>();
        REQUIRE(widget != nullptr);
        auto* table = widget->findChild<QTableView*>();
        REQUIRE(table != nullptr);
        auto* model = dynamic_cast<QG_LayerModel*>(table->model());
        REQUIRE(model != nullptr);
        CHECK(model->getActiveLayer() == expected);
        CHECK(table->currentIndex().row() == model->getIndex(expected).row());
        CHECK_FALSE(graphic->isModified());
    }
}

TEST_CASE("GUI open publishes a validated native offset only after staging", "[.gui][dock-window][offset-import]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString root = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    QString fileName; bool expectNative = true;
    SECTION("binary DXF") { fileName = root + "dxf-binary-R2018-native-degree3.dxf"; }
    SECTION("ODA DWG") { fileName = root + "oda-dwg-R2018-native-degree3.dwg"; }
    SECTION("DWG counted-NUL marker") { fileName = root + "dwg-R2018-native-degree3.dwg"; expectNative = false; }
    REQUIRE(QFileInfo::exists(fileName));
    window.openFile(fileName);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    REQUIRE(LC_DockLayoutTestAccess::pendingImports(window) == 1);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) {
        REQUIRE(elapsed.elapsed() < 35000); settle(10);
    }
    REQUIRE(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); auto* graphic = mdi->getGraphic(); REQUIRE(graphic);
    CHECK(graphic->getFilename() == fileName); CHECK_FALSE(graphic->isModified());
    bool found = false, spline = false;
    for (const auto* entity : *graphic) {
        found |= dynamic_cast<const LC_OffsetCurve*>(entity) != nullptr;
        spline |= dynamic_cast<const RS_Spline*>(entity) != nullptr;
    }
    CHECK(spline); CHECK(found == expectNative);
}

TEST_CASE("An MDI window owns and destroys a transferred graphic", "[.gui][dock-window][offset-import][lifetime]") {
    struct TrackedGraphic final : RS_Graphic {
        explicit TrackedGraphic(int& count) : destroyed(count) {}
        ~TrackedGraphic() override { ++destroyed; }
        int& destroyed;
    };
    DockFixture fixture; auto& window = fixture.window; int destroyed = 0;
    auto graphic = std::make_unique<TrackedGraphic>(destroyed); graphic->initForNewDocument();
    auto* original = graphic.get();
    QPointer<QC_MDIWindow> mdi(window.createOwnedDrawingWindow(std::move(graphic), QStringLiteral("owned.dxf")));
    REQUIRE(mdi); CHECK(mdi->getGraphic() == original); CHECK(destroyed == 0);
    window.doClose(mdi); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK_FALSE(mdi); CHECK(destroyed == 1);
}

TEST_CASE("Cancelling a pending GUI import publishes no drawing", "[.gui][dock-window][offset-import]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(fileName)); window.openFile(fileName);
    REQUIRE(LC_DockLayoutTestAccess::pendingImports(window) == 1);
    LC_DockLayoutTestAccess::cancelImports(window); settle(25);
    CHECK(LC_DockLayoutTestAccess::pendingImports(window) == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
}

TEST_CASE("Startup restores the active file after asynchronous opens", "[.gui][dock-window][offset-import][startup]") {
    DockFixture fixture; auto& window = fixture.window;
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString active = root + "dxf-binary-R2018-line-control.dxf";
    const QString other = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(active)); REQUIRE(QFileInfo::exists(other));
    window.openFile(active); window.openFile(other);
    window.restoreActiveFileAfterPendingImports(active);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(window.getCurrentMDIWindow()); CHECK(window.getCurrentMDIWindow()->getFileName() == active);
    window.openFile(other);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(window.getCurrentMDIWindow()); CHECK(window.getCurrentMDIWindow()->getFileName() == other);
}

#ifdef Q_OS_UNIX
TEST_CASE("Startup imports preserve a later drawing selection", "[.gui][dock-window][offset-import][startup]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString active = directory.filePath("active.dxf"), selected = directory.filePath("selected.dxf");
    const QString pending = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFile::copy(line, active)); REQUIRE(QFile::copy(line, selected));
    window.openFile(active); window.openFile(selected);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    QC_MDIWindow* chosen = nullptr;
    for (auto* mdi : window.findChildren<QC_MDIWindow*>()) if (mdi->getFileName() == selected) chosen = mdi;
    REQUIRE(chosen);
    window.openFile(pending);
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool started = false; pid_t pid = 0; int suspended = -1;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) {
        pid = static_cast<pid_t>(process.processId()); suspended = ::kill(pid, SIGSTOP); started = true;
    });
    while (!started) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(pid > 0); REQUIRE(suspended == 0);
    window.openFile(active);
    window.restoreActiveFileAfterPendingImports(active);
    REQUIRE(window.getCurrentMDIWindow()); REQUIRE(window.getCurrentMDIWindow()->getFileName() == active);
    const auto verifySelection = [&] {
        REQUIRE(window.getCurrentMDIWindow() == chosen);
        SECTION("selection survives multiple publications") {
            while (LC_DockLayoutTestAccess::pendingImports(window) > 1) {
                REQUIRE(elapsed.elapsed() < 35000); settle(10);
            }
            CHECK(window.getCurrentMDIWindow() == chosen);
        }
        SECTION("closing the selected window preserves its replacement") {
            LC_DockLayoutTestAccess::closeDrawing(window, chosen);
            chosen = window.getCurrentMDIWindow();
            REQUIRE(chosen); REQUIRE(chosen->getFileName() == active);
        }
        REQUIRE(::kill(pid, SIGCONT) == 0);
        while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
        CHECK(window.getCurrentMDIWindow() == chosen);
        LC_DockLayoutTestAccess::closeDrawings(window);
    };
    SECTION("window command") {
        window.activateWindow(chosen);
        verifySelection();
    }
    SECTION("MDI activation") {
        REQUIRE(chosen->mdiArea());
        chosen->mdiArea()->setActiveSubWindow(chosen);
        verifySelection();
    }
}

TEST_CASE("New imports take precedence over older startup imports", "[.gui][dock-window][offset-import][startup]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString active = directory.filePath("active.dxf"), requested = directory.filePath("requested.dxf");
    const QString pending = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFile::copy(line, active)); REQUIRE(QFile::copy(line, requested));
    window.openFile(active);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    window.openFile(pending);
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool started = false; pid_t pid = 0; int suspended = -1;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) {
        pid = static_cast<pid_t>(process.processId()); suspended = ::kill(pid, SIGSTOP); started = true;
    });
    while (!started) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(pid > 0); REQUIRE(suspended == 0);
    window.restoreActiveFileAfterPendingImports(active);
    const int before = LC_DockLayoutTestAccess::drawingCount(window);
    bool asTemplate = false;
    SECTION("drawing") { window.openFile(requested); }
    SECTION("template") { asTemplate = true; REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, requested)); }
    while (LC_DockLayoutTestAccess::drawingCount(window) == before) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    auto* chosen = window.getCurrentMDIWindow(); REQUIRE(chosen);
    CHECK(chosen->getFileName() == (asTemplate ? QString{} : requested));
    REQUIRE(::kill(pid, SIGCONT) == 0);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    CHECK(window.getCurrentMDIWindow() == chosen);
    LC_DockLayoutTestAccess::closeDrawings(window);
}

TEST_CASE("Later startup import choices win over slow earlier requests", "[.gui][dock-window][offset-import][startup]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString active = directory.filePath("active.dxf"), selected = directory.filePath("selected.dxf");
    const QString pending = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFile::copy(line, active)); REQUIRE(QFile::copy(line, selected));
    window.openFile(active); window.openFile(selected);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    QC_MDIWindow* selectedWindow = nullptr;
    for (auto* mdi : window.findChildren<QC_MDIWindow*>()) if (mdi->getFileName() == selected) selectedWindow = mdi;
    REQUIRE(selectedWindow);
    window.openFile(line);
    window.restoreActiveFileAfterPendingImports(active);
    window.openFile(pending);
    auto* slow = LC_DockLayoutTestAccess::lastPendingImport(window); REQUIRE(slow);
    bool secondStarted = false; pid_t secondPid = 0; int secondStopped = -1;
    LC_GuiImportTestAccess::onStarted(*slow, [&](QProcess& process) {
        secondPid = static_cast<pid_t>(process.processId()); secondStopped = ::kill(secondPid, SIGSTOP); secondStarted = true;
    });
    while (!secondStarted) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(secondPid > 0); REQUIRE(secondStopped == 0);
    QC_MDIWindow* chosen = nullptr;
    SECTION("later open finishes first") {
        const int before = LC_DockLayoutTestAccess::drawingCount(window);
        window.openFile(selected);
        while (LC_DockLayoutTestAccess::drawingCount(window) == before) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
        chosen = window.getCurrentMDIWindow(); REQUIRE(chosen); CHECK(chosen->getFileName() == selected);
        CHECK(chosen != selectedWindow);
    }
    SECTION("later window switch") {
        window.activateWindow(selectedWindow);
        chosen = selectedWindow; REQUIRE(window.getCurrentMDIWindow() == chosen);
    }
    REQUIRE(::kill(secondPid, SIGCONT) == 0);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    CHECK(window.getCurrentMDIWindow() == chosen);
    bool native = false;
    for (auto* mdi : window.findChildren<QC_MDIWindow*>()) {
        if (mdi->getFileName() != pending || !mdi->getGraphic()) continue;
        for (auto* entity : *mdi->getGraphic()) native |= dynamic_cast<LC_OffsetCurve*>(entity) != nullptr;
    }
    CHECK(native);
    LC_DockLayoutTestAccess::closeDrawings(window);
}

#endif

TEST_CASE("Queued new import wins over remaining startup imports", "[.gui][dock-window][offset-import][startup][queue]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString active = directory.filePath("active.dxf"), requested = directory.filePath("requested.dxf");
    const QString selected = directory.filePath("selected.dxf");
    REQUIRE(QFile::copy(line, active)); REQUIRE(QFile::copy(line, requested)); REQUIRE(QFile::copy(line, selected));
    window.openFile(active); window.openFile(selected);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    QC_MDIWindow* existing = nullptr;
    for (auto* mdi : window.findChildren<QC_MDIWindow*>()) if (mdi->getFileName() == selected) existing = mdi;
    REQUIRE(existing);
    for (int i = 0; i < LC_DockLayoutTestAccess::maxActiveImports(); ++i) {
        window.openFile(line);
    }
    REQUIRE(LC_DockLayoutTestAccess::activeImports(window) == LC_DockLayoutTestAccess::maxActiveImports());
    window.restoreActiveFileAfterPendingImports(active);
    window.openFile(requested);
    REQUIRE(LC_DockLayoutTestAccess::queuedImports(window) == 1);
    bool switched = false;
    SECTION("queued request wins") {}
    SECTION("later switch wins") { switched = true; window.activateWindow(existing); }
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    auto* chosen = window.getCurrentMDIWindow(); REQUIRE(chosen);
    if (switched) CHECK(chosen == existing);
    else CHECK(chosen->getFileName() == requested);
    LC_DockLayoutTestAccess::closeDrawings(window);
}

TEST_CASE("Staged GUI imports are bounded and queued work completes", "[.gui][dock-window][offset-import][queue]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString native = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(line)); REQUIRE(QFileInfo::exists(native));
    for (int i = 0; i < LC_DockLayoutTestAccess::maxActiveImports(); ++i) window.openFile(line);
    window.openFile(native); REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, native));
    CHECK(LC_DockLayoutTestAccess::activeImports(window) == LC_DockLayoutTestAccess::maxActiveImports());
    CHECK(LC_DockLayoutTestAccess::queuedImports(window) == 2);
    CHECK(LC_DockLayoutTestAccess::queuedFile(window, 0) == native);
    CHECK_FALSE(LC_DockLayoutTestAccess::queuedTemplate(window, 0));
    CHECK(LC_DockLayoutTestAccess::queuedFile(window, 1) == native);
    CHECK(LC_DockLayoutTestAccess::queuedTemplate(window, 1));
    window.restoreActiveFileAfterPendingImports(line);
    int peak = LC_DockLayoutTestAccess::activeImports(window);
    QTimer monitor; monitor.setInterval(1);
    QObject::connect(&monitor, &QTimer::timeout, [&] { peak = std::max(peak, LC_DockLayoutTestAccess::activeImports(window)); });
    monitor.start(); QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 60000); settle(10); }
    monitor.stop();
    CHECK(peak <= LC_DockLayoutTestAccess::maxActiveImports());
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial + LC_DockLayoutTestAccess::maxActiveImports() + 2);
    REQUIRE(window.getCurrentMDIWindow()); CHECK(window.getCurrentMDIWindow()->getFileName() == line);
    LC_DockLayoutTestAccess::closeDrawings(window);
}

TEST_CASE("Forced application destruction closes live imported drawings", "[.gui][dock-window][offset-import][forced-destruction]") {
    auto& window = testWindow(); window.resize(700, 500); window.prepareWindowForShow(); window.show(); settle();
    const QString root = QString(LIBRECAD_SOURCE_DIR) + "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    const QString line = root + "dxf-binary-R2018-line-control.dxf";
    const QString native = root + "dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(line)); REQUIRE(QFileInfo::exists(native));
    for (int i = 0; i < 4; ++i) window.openFile(line);
    window.openFile(native); REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, native));
    window.restoreActiveFileAfterPendingImports(line);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 60000); settle(10); }
    bool helperStarted = false; qint64 helperPid = 0; int suspended = -1;
    SECTION("live drawings") {}
#ifdef Q_OS_UNIX
    SECTION("live drawings and an active helper") {
        window.openFile(native); auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
        LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) {
            helperPid = process.processId(); suspended = ::kill(static_cast<pid_t>(helperPid), SIGSTOP); helperStarted = true;
        });
        while (!helperStarted) { REQUIRE(elapsed.elapsed() < 60000); settle(10); }
        REQUIRE(helperPid > 0); REQUIRE(suspended == 0);
    }
#endif
    QList<QPointer<QC_MDIWindow>> drawings;
    for (auto* drawing : window.findChildren<QC_MDIWindow*>()) drawings.append(drawing);
    REQUIRE(drawings.size() == 6); window.hide(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    REQUIRE(testWindowOwner); testWindowOwner->reset();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    for (const auto& drawing : drawings) CHECK_FALSE(drawing);
#ifdef Q_OS_UNIX
    if (helperStarted) { errno = 0; CHECK(::kill(static_cast<pid_t>(helperPid), 0) == -1); CHECK(errno == ESRCH); }
#endif
    auto& replacement = testWindow(); replacement.resize(700, 500); replacement.prepareWindowForShow(); replacement.show(); settle();
    CHECK(LC_DockLayoutTestAccess::drawingCount(replacement) == 0);
}

TEST_CASE("Staged GUI import queue reports overflow and cancellation clears it", "[.gui][dock-window][offset-import][queue]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(fileName));
    for (int i = 0; i < LC_DockLayoutTestAccess::maxActiveImports() + LC_DockLayoutTestAccess::maxQueuedImports() + 1; ++i)
        window.openFile(fileName);
    CHECK(LC_DockLayoutTestAccess::activeImports(window) == LC_DockLayoutTestAccess::maxActiveImports());
    CHECK(LC_DockLayoutTestAccess::queuedImports(window) == LC_DockLayoutTestAccess::maxQueuedImports());
    bool reported = false;
    for (const auto* history : window.findChildren<QTextEdit*>())
        reported |= history->toPlainText().contains("Too many pending imports");
    CHECK(reported);
    LC_DockLayoutTestAccess::cancelImports(window); settle(25);
    CHECK(LC_DockLayoutTestAccess::pendingImports(window) == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
}

TEST_CASE("A queued import failure does not strand later requests", "[.gui][dock-window][offset-import][queue]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString line = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-line-control.dxf";
    REQUIRE(QFileInfo::exists(line)); const QString broken = directory.filePath("broken.dxf"); QFile file(broken);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const QByteArray bytes = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "0\nLINE\n8\n0\n10\n0.0\n";
    REQUIRE(file.write(bytes) == bytes.size()); file.close();
    for (int i = 0; i < LC_DockLayoutTestAccess::maxActiveImports(); ++i) window.openFile(line);
    window.openFile(broken); window.openFile(line);
    REQUIRE(LC_DockLayoutTestAccess::queuedImports(window) == 2);
    QElapsedTimer elapsed; elapsed.start(); QPointer<QMessageBox> box;
    while (!box) {
        REQUIRE(elapsed.elapsed() < 30000); settle(10);
        for (auto* candidate : window.findChildren<QMessageBox*>()) if (candidate->isVisible()) { box = candidate; break; }
    }
    box->done(QMessageBox::Ok);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 60000); settle(10); }
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial + LC_DockLayoutTestAccess::maxActiveImports() + 1);
    LC_DockLayoutTestAccess::closeDrawings(window);
}

#ifdef Q_OS_UNIX
TEST_CASE("GUI import cancels and reaps its running helper before retry", "[.gui][dock-window][offset-import][worker]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(fileName)); window.openFile(fileName);
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool started = false; pid_t pid = 0; int suspended = -1;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) {
        pid = static_cast<pid_t>(process.processId()); suspended = ::kill(pid, SIGSTOP); started = true;
    });
    QElapsedTimer elapsed; elapsed.start();
    while (!started) { REQUIRE(elapsed.elapsed() < 10000); settle(10); }
    REQUIRE(pid > 0); REQUIRE(suspended == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    SECTION("cancel and retry") {
        LC_DockLayoutTestAccess::cancelImports(window);
        errno = 0; CHECK(::kill(pid, 0) == -1); CHECK(errno == ESRCH);
        CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
        window.openFile(fileName);
        while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
        CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
        auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); REQUIRE(mdi->getGraphic());
        bool found = false; for (const auto* entity : *mdi->getGraphic()) if (dynamic_cast<const LC_OffsetCurve*>(entity)) found = true;
        CHECK(found);
    }
    SECTION("application close") {
        REQUIRE(window.close()); settle(25);
        errno = 0; CHECK(::kill(pid, 0) == -1); CHECK(errno == ESRCH);
        CHECK(LC_DockLayoutTestAccess::pendingImports(window) == 0);
        CHECK(LC_DockLayoutTestAccess::drawingCount(window) == 0);
    }
}
#endif

TEST_CASE("GUI recovery import stays modified after automatic zoom", "[.gui][dock-window][offset-import]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString fileName = directory.filePath("recovery.dxf"); QFile file(fileName);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const QByteArray bytes = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n9\n$DWGCODEPAGE\n3\nANSI_1252\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n0\nENDSEC\n0\nEOF\n";
    REQUIRE(file.write(bytes) == bytes.size()); file.close();
    window.openFile(fileName, RS2::FormatDXFRW, RS2::DxfTextEncoding::LegacyLibreCadUtf8);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 5000); settle(10); }
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); auto* graphic = mdi->getGraphic(); REQUIRE(graphic);
    CHECK(graphic->isDxfRecoverySourceFile(fileName)); CHECK(graphic->isModified());
}

TEST_CASE("A GUI import helper failure publishes an ordinary fallback", "[.gui][dock-window][offset-import][worker]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(QFileInfo::exists(fileName)); window.openFile(fileName);
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool started = false;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) { started = true; process.kill(); });
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(started); REQUIRE(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); REQUIRE(mdi->getGraphic());
    bool spline = false, native = false;
    for (const auto* entity : *mdi->getGraphic()) {
        spline |= dynamic_cast<const RS_Spline*>(entity) != nullptr;
        native |= dynamic_cast<const LC_OffsetCurve*>(entity) != nullptr;
    }
    CHECK(spline); CHECK_FALSE(native);
    bool reported = false;
    for (const auto* history : window.findChildren<QTextEdit*>())
        reported |= history->toPlainText().contains("could not be restored as editable");
    CHECK(reported);
}

TEST_CASE("A source changed during GUI validation cannot be saved in place", "[.gui][dock-window][offset-import][source-change]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString fixturePath = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    const QString fileName = directory.filePath("drawing.dxf"); REQUIRE(QFile::copy(fixturePath, fileName));
    REQUIRE(QFile::setPermissions(fileName, QFile::ReadOwner | QFile::WriteOwner));
    const QDateTime originalTime = QFileInfo(fileName).lastModified(); REQUIRE(originalTime.isValid());
    window.openFile(fileName); auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool changed = false;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess&) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite) && file.seek(file.size()) && file.write("changed") == 7 && file.flush())
            changed = file.setFileTime(originalTime.addSecs(60), QFileDevice::FileModificationTime);
    });
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(changed); auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); auto* graphic = mdi->getGraphic(); REQUIRE(graphic);
    CHECK(graphic->getFormatType() == RS2::FormatUnknown); CHECK(graphic->isModified());
    CHECK(graphic->getLastSaveTime() == originalTime); CHECK(QFileInfo(fileName).lastModified() != originalTime);
    bool reported = false; for (const auto* history : window.findChildren<QTextEdit*>())
        reported |= history->toPlainText().contains("use Save As to avoid overwriting it");
    CHECK(reported);
}

TEST_CASE("A source changed at the read boundary is not published", "[.gui][dock-window][offset-import][source-change]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fixturePath = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    const QString fileName = directory.filePath("drawing.dxf"); REQUIRE(QFile::copy(fixturePath, fileName));
    REQUIRE(QFile::setPermissions(fileName, QFile::ReadOwner | QFile::WriteOwner));
    const QDateTime originalTime = QFileInfo(fileName).lastModified(); REQUIRE(originalTime.isValid());
    window.openFile(fileName); auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool changed = false;
    LC_GuiImportTestAccess::afterRead(*session, [&] {
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite) && file.seek(file.size()) && file.write("changed") == 7 && file.flush())
            changed = file.setFileTime(originalTime.addSecs(60), QFileDevice::FileModificationTime);
    });
    QElapsedTimer elapsed; elapsed.start(); QPointer<QMessageBox> box;
    while (!box) {
        REQUIRE(elapsed.elapsed() < 10000); settle(10);
        for (auto* candidate : window.findChildren<QMessageBox*>()) if (candidate->isVisible()) { box = candidate; break; }
    }
    REQUIRE(changed); CHECK(box->text().contains("changed while it was being read"));
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial); box->done(QMessageBox::Ok);
    settle(10); CHECK(LC_DockLayoutTestAccess::pendingImports(window) == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
}

#ifdef Q_OS_UNIX
TEST_CASE("A same-size source edit with restored mtime is detected where metadata time is available",
          "[.gui][dock-window][offset-import][source-change]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const QString fixturePath = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    const QString fileName = directory.filePath("drawing.dxf"); REQUIRE(QFile::copy(fixturePath, fileName));
    REQUIRE(QFile::setPermissions(fileName, QFile::ReadOwner | QFile::WriteOwner));
    const QFileInfo original(fileName); const QDateTime originalTime = original.lastModified();
    REQUIRE(originalTime.isValid());
    if (!original.metadataChangeTime().isValid()) SKIP("Filesystem metadata-change time is unavailable");
    QFile before(fileName); REQUIRE(before.open(QIODevice::ReadOnly)); REQUIRE(before.seek(8));
    const QByteArray originalByte = before.read(1); REQUIRE(originalByte.size() == 1); before.close();
    const QByteArray replacement = originalByte == QByteArray("X") ? QByteArray("Y") : QByteArray("X");
    const qint64 originalSize = original.size(); window.openFile(fileName);
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session); bool changed = false;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess&) {
        QThread::msleep(30); QFile file(fileName);
        if (file.open(QIODevice::ReadWrite) && file.seek(8) && file.write(replacement) == 1 && file.flush())
            changed = file.setFileTime(originalTime, QFileDevice::FileModificationTime);
    });
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(changed); const QFileInfo updated(fileName); CHECK(updated.size() == originalSize);
    CHECK(updated.lastModified() == originalTime); CHECK(updated.metadataChangeTime() != original.metadataChangeTime());
    QFile after(fileName); REQUIRE(after.open(QIODevice::ReadOnly)); REQUIRE(after.seek(8)); CHECK(after.read(1) == replacement);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); REQUIRE(mdi->getGraphic());
    CHECK(mdi->getGraphic()->getFormatType() == RS2::FormatUnknown); CHECK(mdi->getGraphic()->isModified());
}
#endif

TEST_CASE("A malformed DXF is reported without publishing a GUI drawing", "[.gui][dock-window][offset-import]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = directory.filePath("broken.dxf"); QFile file(fileName);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const QByteArray bytes = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "0\nLINE\n8\n0\n10\n0.0\n";
    REQUIRE(file.write(bytes) == bytes.size()); file.close(); window.openFile(fileName);
    QElapsedTimer elapsed; elapsed.start(); QPointer<QMessageBox> box;
    while (!box) {
        REQUIRE(elapsed.elapsed() < 5000); settle(10);
        for (auto* candidate : window.findChildren<QMessageBox*>()) if (candidate->isVisible()) { box = candidate; break; }
    }
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    box->done(QMessageBox::Ok);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 5000); settle(10); }
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
}

TEST_CASE("GUI templates publish validated untitled native drawings", "[.gui][dock-window][offset-template]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString root = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/";
    QString fileName;
    SECTION("binary DXF") { fileName = root + "dxf-binary-R2018-native-degree3.dxf"; }
    SECTION("DWG") { fileName = root + "oda-dwg-R2018-native-degree3.dwg"; }
    QFile source(fileName); REQUIRE(source.open(QIODevice::ReadOnly)); const auto original = source.readAll(); source.close();
    REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, fileName));
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    REQUIRE(LC_DockLayoutTestAccess::pendingImports(window) == 1);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); auto* graphic = mdi->getGraphic(); REQUIRE(graphic);
    CHECK(graphic->getFilename().isEmpty()); CHECK(graphic->getFormatType() == RS2::FormatDXFRW);
    CHECK_FALSE(graphic->getAutoSaveFileName().isEmpty()); CHECK(graphic->getAutoSaveFileName() != fileName);
    CHECK_FALSE(LC_DockLayoutTestAccess::tracksOpenedFile(window, fileName));
    bool native = false; for (const auto* entity : *graphic) native |= dynamic_cast<const LC_OffsetCurve*>(entity) != nullptr;
    CHECK(native); REQUIRE(source.open(QIODevice::ReadOnly)); CHECK(source.readAll() == original);
}

TEST_CASE("Staged imports retain Qt resource support", "[.gui][dock-window][offset-import][resource]") {
    DockFixture fixture;
    const QString path = QStringLiteral(":/dxf/dim_sample.dxf"); REQUIRE(QFile::exists(path));
    LC_OffsetImportSession session; std::optional<LC_OffsetImportSession::Outcome> outcome;
    REQUIRE(session.start(path, RS2::FormatDXFRW, RS2::DxfTextEncoding::Declared, false,
        [&](LC_OffsetImportSession::Outcome result) { outcome.emplace(std::move(result)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(outcome->status == LC_OffsetImportSession::Status::Complete); REQUIRE(outcome->graphic);
    CHECK_FALSE(outcome->graphic->isEmpty());
}

TEST_CASE("Blank new drawings remain immediate", "[.gui][dock-window][offset-template]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, QString{}));
    CHECK(LC_DockLayoutTestAccess::pendingImports(window) == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    REQUIRE(window.getCurrentMDIWindow()); REQUIRE(window.getCurrentMDIWindow()->getGraphic());
    CHECK(window.getCurrentMDIWindow()->getGraphic()->getFilename().isEmpty());
}

TEST_CASE("Default-template action stages its drawing before publication", "[.gui][dock-window][offset-template]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    lc::test::SettingGuard setting(RS_SETTINGS, "Paths", "Template"); setting.set(fileName);
    window.slotFileNewFromDefaultTemplate();
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    REQUIRE(LC_DockLayoutTestAccess::pendingImports(window) == 1);
    QElapsedTimer elapsed; elapsed.start();
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); REQUIRE(mdi->getGraphic());
    bool native = false; for (const auto* entity : *mdi->getGraphic()) native |= dynamic_cast<const LC_OffsetCurve*>(entity) != nullptr;
    CHECK(native); CHECK(mdi->getGraphic()->getFilename().isEmpty());
}

TEST_CASE("Malformed GUI template reports failure without a new drawing", "[.gui][dock-window][offset-template]") {
    DockFixture fixture; auto& window = fixture.window; QTemporaryDir directory; REQUIRE(directory.isValid());
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = directory.filePath("broken.dxf"); QFile file(fileName);
    REQUIRE(file.open(QIODevice::WriteOnly));
    const QByteArray bytes = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "0\nLINE\n8\n0\n10\n0.0\n";
    REQUIRE(file.write(bytes) == bytes.size()); file.close();
    REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, fileName));
    QElapsedTimer elapsed; elapsed.start(); QPointer<QMessageBox> box;
    while (!box) {
        REQUIRE(elapsed.elapsed() < 5000); settle(10);
        for (auto* candidate : window.findChildren<QMessageBox*>()) if (candidate->isVisible()) { box = candidate; break; }
    }
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial); box->done(QMessageBox::Ok);
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 5000); settle(10); }
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
}

#ifdef Q_OS_UNIX
TEST_CASE("Cancelling a GUI template reaps its active helper without publication", "[.gui][dock-window][offset-template][worker]") {
    DockFixture fixture; auto& window = fixture.window;
    const int initial = LC_DockLayoutTestAccess::drawingCount(window);
    const QString fileName = QString(LIBRECAD_SOURCE_DIR) +
        "/librecad/src/lib/filters/tests/testdata/offsetcurve_independent/dxf-binary-R2018-native-degree3.dxf";
    REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, fileName));
    auto* session = LC_DockLayoutTestAccess::pendingImport(window); REQUIRE(session);
    bool started = false; pid_t pid = 0; int suspended = -1;
    LC_GuiImportTestAccess::onStarted(*session, [&](QProcess& process) {
        pid = static_cast<pid_t>(process.processId()); suspended = ::kill(pid, SIGSTOP); started = true;
    });
    QElapsedTimer elapsed; elapsed.start();
    while (!started) { REQUIRE(elapsed.elapsed() < 10000); settle(10); }
    REQUIRE(pid > 0); REQUIRE(suspended == 0);
    CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    LC_DockLayoutTestAccess::cancelImports(window);
    errno = 0; CHECK(::kill(pid, 0) == -1); CHECK(errno == ESRCH);
    settle(25); CHECK(LC_DockLayoutTestAccess::drawingCount(window) == initial);
    REQUIRE(LC_DockLayoutTestAccess::newTemplate(window, fileName));
    while (LC_DockLayoutTestAccess::pendingImports(window)) { REQUIRE(elapsed.elapsed() < 35000); settle(10); }
    REQUIRE(LC_DockLayoutTestAccess::drawingCount(window) == initial + 1);
    auto* mdi = window.getCurrentMDIWindow(); REQUIRE(mdi); REQUIRE(mdi->getGraphic());
    bool native = false; for (const auto* entity : *mdi->getGraphic()) native |= dynamic_cast<const LC_OffsetCurve*>(entity) != nullptr;
    CHECK(native); CHECK(mdi->getGraphic()->getFilename().isEmpty());
}
#endif

TEST_CASE("Legacy UTF-8 recovery has a separate File menu action", "[.gui][legacy-utf8-ui]") {
    DockFixture fixture;
    auto& window = fixture.window;
    auto* ordinary = window.getAction("FileOpen");
    auto* recovery = window.getAction("FileOpenLegacyLibreCadUtf8");
    REQUIRE(ordinary != nullptr);
    REQUIRE(recovery != nullptr);
    CHECK(ordinary != recovery);
    CHECK(ordinary->shortcuts().contains(QKeySequence(QKeySequence::Open)));
    CHECK(recovery->shortcuts().isEmpty());
    CHECK_FALSE(recovery->isCheckable());
    bool besideOrdinaryOpen = false;
    for (auto* topLevel : window.menuBar()->actions()) {
        if (auto* menu = topLevel->menu()) {
            const auto actions = menu->actions();
            const int index = actions.indexOf(ordinary);
            if (index >= 0)
                besideOrdinaryOpen = actions.value(index + 1) == recovery;
        }
    }
    CHECK(besideOrdinaryOpen);
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
    const auto named = LC_DockLayoutTestAccess::fileRoundTrip(workspace, false);
    CHECK(named.hasDockVisibility);
    CHECK(named.dockVisibility == workspace.dockVisibility);
    CHECK(named.windowWidth == 777);
    CHECK(named.windowHeight == 444);
    CHECK(named.docAreaFloatingActive);
    CHECK_FALSE(named.dockAreaBottomActive);
    const auto legacy = LC_DockLayoutTestAccess::fileRoundTrip(workspace, true);
    CHECK_FALSE(legacy.hasDockVisibility);
    CHECK(legacy.windowWidth == 777);
    CHECK(legacy.windowHeight == 444);
    {
        auto paths = RS_SETTINGS->beginGroupGuard("Paths");
        RS_SETTINGS->writeEntry("OtherSettingsDir", lc::test::SettingsIsolation::directory());
    }
}

TEST_CASE("A first start shows the Layer List and the Command Line beside the drawing",
          "[.gui][dock-window][first-start]") {
    for (const QSize size : {QSize(1280, 720), QSize(1366, 768), QSize(1920, 1080)}) {
        CAPTURE(size.width(), size.height());
        requireScreen(size);
        FirstStart start;
        start.show(size);
        checkDefaultRightDocks(start);
    }
    requireScreen({1280, 720});
    FirstStart start;
    start.show({1280, 720}, true);
    CHECK(start.window.isMaximized());
    checkDefaultRightDocks(start);
}

TEST_CASE("A saved window geometry does not skip the first-start dock widths", "[.gui][dock-window][first-start]") {
    for (const QSize size : {QSize(1366, 768), QSize(1920, 1080)}) {
        CAPTURE(size.width(), size.height());
        requireScreen(size);
        // As in a profile copied from 2.2.x: the geometry is there, the dock layout is not restored.
        LC_SET_ONE("Geometry", "WindowGeometry", QString("AA=="));
        FirstStart start;
        {
            const auto geometry = RS_SETTINGS->beginGroupGuard("Geometry");
            RS_SETTINGS->remove("WindowGeometry");
        }
        for (QDockWidget* dock : start.window.findChildren<QDockWidget*>()) {
            if (start.window.dockWidgetArea(dock) == Qt::RightDockWidgetArea) {
                CAPTURE(dock->objectName().toStdString());
                CHECK(dock->width() <= 390);
            }
        }
        start.show(size);
        checkDefaultRightDocks(start);
    }
}

TEST_CASE("First-start dock widths follow the window until the user sizes the docks",
          "[.gui][dock-window][first-start]") {
    requireScreen({1920, 1080});
    FirstStart start;
    auto& window = start.window;
    start.show({1280, 720});
    const int small = start.rightStrip();
    window.resize(1920, 1080);
    settle();
    INFO("strips " << small << " then " << start.rightStrip());
    CHECK(start.rightStrip() > small);
    checkDefaultRightDocks(start);

    window.resizeDocks({start.dock("layer_dockwidget")}, {500}, Qt::Horizontal);
    settle();
    const int chosen = start.rightStrip();
    CHECK(chosen > 480);
    window.resize(1600, 900);
    settle();
    CHECK(start.rightStrip() == chosen);
}

TEST_CASE("Narrowing the window shrinks the right docks by the tab on screen", "[.gui][dock-window][first-start]") {
    requireScreen({1920, 1080});
    FirstStart start;
    auto& window = start.window;
    start.show({1920, 1080});
    start.dock("command_dockwidget")->close();
    // The Pens Palette is the first of its group and keeps the width it had when it was last on screen.
    auto* palette = start.dock("pen_palette_dockwidget");
    auto* layers = start.dock("layer_dockwidget");
    palette->raise();
    settle();
    window.resizeDocks({palette}, {1100}, Qt::Horizontal);
    settle();
    layers->raise();
    settle();
    window.resizeDocks({layers}, {360}, Qt::Horizontal);
    settle();
    INFO("palette=" << palette->width() << " layers=" << layers->width());
    REQUIRE_FALSE(start.shown(palette));
    REQUIRE(palette->width() > 900);
    REQUIRE(layers->width() - layers->minimumSizeHint().width() > 20);
    // 10 pixels too narrow for the drawing area beside the docks as they are
    window.resize(window.width() - window.getMdiArea()->width() + 480 - 10, window.height());
    settle();
    INFO("canvas=" << window.getMdiArea()->width() << " strip=" << start.rightStrip()
         << " collapsed=" << LC_DockLayoutTestAccess::collapsedGroups(window).join(',').toStdString());
    CHECK(start.shown(layers));
    CHECK(LC_DockLayoutTestAccess::collapsedGroups(window).isEmpty());
    CHECK(window.getMdiArea()->width() >= 480);
}

TEST_CASE("A short window gives up the Command Line before the Layer List", "[.gui][dock-window][first-start]") {
    requireScreen({1366, 768});
    FirstStart start;
    start.show({1366, 460});
    INFO("collapsed=" << LC_DockLayoutTestAccess::collapsedGroups(start.window).join(',').toStdString());
    CHECK(start.shown(start.dock("layer_dockwidget")));
    CHECK_FALSE(start.shown(start.dock("command_dockwidget")));
    CHECK(start.window.layout()->minimumSize().height() <= start.window.height());
}

TEST_CASE("Docks hidden by a narrow first start come back at the first-start width",
          "[.gui][dock-window][first-start]") {
    requireScreen({1920, 1080});
    int expected = 0;
    {
        FirstStart start;
        start.show({1920, 1080});
        expected = start.rightStrip();
    }
    FirstStart start;
    start.show({700, 700});
    CHECK_FALSE(start.shown(start.dock("layer_dockwidget")));
    start.window.resize(1920, 1080);
    settle();
    checkDefaultRightDocks(start);
    CHECK(start.rightStrip() == expected);
}

TEST_CASE("A restored layout gets the first-start width only if its right docks are over-wide",
          "[.gui][dock-window][first-start]") {
    requireScreen({1920, 1080});
    const QHash<int, bool> areas{{Qt::RightDockWidgetArea, true}};
    for (const int saved : {1100, 560}) {
        CAPTURE(saved);
        QByteArray state;
        int savedStrip = 0;
        {
            FirstStart start;
            start.show({1920, 1080});
            start.window.resizeDocks({start.dock("layer_dockwidget")}, {saved}, Qt::Horizontal);
            settle();
            savedStrip = start.rightStrip();
            state = start.window.saveState();
        }
        FirstStart start;
        start.window.restoreDockLayout({}, false, areas, state);
        start.show({1920, 1080});
        INFO("saved strip=" << savedStrip << " restored strip=" << start.rightStrip());
        CHECK(start.shown(start.dock("layer_dockwidget")));
        if (5 * savedStrip > 2 * start.window.width()) CHECK(start.rightStrip() * 4 <= start.window.width());
        else CHECK(start.rightStrip() == savedStrip);
    }
}
