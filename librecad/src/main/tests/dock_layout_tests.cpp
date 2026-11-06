// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>
#include <QtWidgets>

#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

class LC_DockLayoutTests : public QObject {
    Q_OBJECT

    QTemporaryDir settingsDirectory;
    QC_ApplicationWindow* window {nullptr};
    std::unique_ptr<QC_ApplicationWindow>* instance {nullptr};

    QDockWidget* addDock(const QString& name, const QSize& minimum = QSize(150, 60))
    {
        auto* dock = new QDockWidget(name, window);
        dock->setObjectName(name);
        auto* content = new QWidget(dock);
        content->setMinimumSize(minimum);
        dock->setWidget(content);
        window->addDockWidget(Qt::RightDockWidgetArea, dock);
        window->requestedDockVisibility[name] = true;
        return dock;
    }

    void isolatedLayout()
    {
        window->collapsedDockGroups.clear();
        for (auto it = window->requestedDockVisibility.begin(); it != window->requestedDockVisibility.end(); ++it)
            it.value() = false;
        window->applyDockVisibility();
        for (auto* toolbar : window->findChildren<QToolBar*>()) toolbar->hide();
        window->statusBar()->hide();
        window->requestedDockAreas[Qt::RightDockWidgetArea] = true;
        window->fittedDockState.clear();
    }

    void settle()
    {
        QTest::qWait(300);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(settingsDirectory.isValid());
        qApp->setQuitOnLastWindowClosed(false);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDirectory.path());
        QCoreApplication::setOrganizationName(QString("LibreCAD-DockLayoutTests-%1").arg(qApp->applicationPid()));
        QCoreApplication::setApplicationName("initialization");
        QCoreApplication::setApplicationVersion("2.2.1");
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        RS_SYSTEM->init("LibreCAD", "2.2.1", "librecad", QString(LIBRECAD_TEST_SOURCE_DIR) + "/unix");
        RS_FONTLIST->init();
        RS_PATTERNLIST->init();
    }

    void init()
    {
        QCoreApplication::setApplicationName(QString::fromLatin1(QTest::currentTestFunction()));
        QSettings settings;
        settings.setValue("Startup/FirstLoad", false);
        settings.setValue("Defaults/AutoBackupDocument", false);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        if (!instance) instance = &QC_ApplicationWindow::getAppWindow();
        else instance->reset(new QC_ApplicationWindow);
        window = instance->get();
        window->prepareWindowForShow();
        window->resize(700, 500);
        window->show();
        settle();
    }

    void cleanup()
    {
        instance->reset();
        window = nullptr;
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    void cleanupTestCase()
    {
        QSettings(QSettings::NativeFormat, QSettings::UserScope, qApp->organizationName(),
                  "savedDockSizesSurviveRestart").clear();
    }

    void movingCommandEnablesDestination()
    {
        auto* command = window->findChild<QDockWidget*>("command_dockwidget");
        window->requestDockVisible(command);
        window->addDockWidget(Qt::BottomDockWidgetArea, command);
        QVERIFY(window->requestedDockAreas.value(Qt::BottomDockWidgetArea));
        window->slotFileNew();
        QVERIFY(window->requestedDockVisibility.value(command->objectName()));
        QVERIFY(!command->isHidden());
    }

    void fullscreenDoesNotHideDocks()
    {
        isolatedLayout();
        auto* dock = addDock("fullscreen_panel");
        window->resize(700, 500);
        settle();
        QVERIFY(!dock->isHidden());
        window->showFullScreen();
        QTRY_VERIFY(window->isFullScreen());
        QTest::qWait(1500);
        QVERIFY(!dock->isHidden());
        window->showNormal();
        QTRY_VERIFY(!window->isFullScreen());
        QTest::qWait(1500);
        QVERIFY(!dock->isHidden());
    }

    void splitGroupsReclaimSharedWidth()
    {
        isolatedLayout();
        auto* first = addDock("split_first");
        auto* second = addDock("split_second");
        window->resize(420, 500);
        settle();
        QVERIFY(first->isHidden());
        QVERIFY(second->isHidden());
        QVERIFY(window->mdiAreaCAD->width() >= 320);
    }

    void protectedSplitGroupsCannotExceedScreen()
    {
        isolatedLayout();
        const QRect available = qApp->primaryScreen()->availableGeometry();
        auto* first = addDock("oversized_first", QSize(available.width() + 100, 60));
        auto* second = addDock("oversized_second", QSize(available.width() + 100, 60));
        window->requestDockVisible(second);
        window->fitDockLayout();
        QVERIFY(first->isHidden());
        QVERIFY(second->isHidden());
        QVERIFY(available.contains(window->frameGeometry()));
        QVERIFY(window->requestedDockVisibility.value(second->objectName()));
    }

    void toolbarClosureRestoresDocksWithoutWindowGrowth()
    {
        isolatedLayout();
        auto* first = addDock("restore_first");
        auto* second = addDock("restore_second");
        auto* toolbar = window->findChild<QToolBar*>("file_toolbar");
        QVERIFY(toolbar);
        toolbar->clear();
        auto* content = new QWidget(toolbar);
        content->setFixedSize(150, 100);
        toolbar->addWidget(content);
        window->addToolBar(Qt::LeftToolBarArea, toolbar);
        toolbar->show();
        window->resize(796, 500);
        settle();
        QVERIFY(first->isHidden());
        QVERIFY(second->isHidden());
        const QSize size = window->size();
        toolbar->hide();
        settle();
        QCOMPARE(window->size(), size);
        QVERIFY(!first->isHidden());
        QVERIFY(!second->isHidden());
        QVERIFY(window->mdiAreaCAD->width() >= 504);
    }

    void savingDoesNotMutateEffectiveLayout()
    {
        isolatedLayout();
        auto* first = addDock("save_first", QSize(150, 400));
        addDock("save_second", QSize(150, 400));
        auto collapsed = window->captureDockGroup(first);
        collapsed.pressure = Qt::Vertical;
        window->collapsedDockGroups.append(collapsed);
        window->applyDockVisibility();
        window->resize(700, 500);
        settle();
        const QByteArray state = window->saveState();
        const QRect geometry = window->geometry();
        const auto requested = window->requestedDockVisibility;
        const QByteArray intended = window->dockStateForSaving();
        settle();
        QCOMPARE(window->geometry(), geometry);
        QCOMPARE(window->saveState(), state);
        QCOMPARE(window->requestedDockVisibility, requested);
        QVERIFY(!intended.isEmpty());
    }

    void areaTogglePreservesIndividualChoices()
    {
        isolatedLayout();
        auto* first = addDock("area_first");
        auto* second = addDock("area_second");
        window->requestedDockVisibility[second->objectName()] = false;
        window->getAction("RightDockAreaToggle")->setChecked(true);
        window->getAction("RightDockAreaToggle")->trigger();
        QVERIFY(first->isHidden());
        QVERIFY(second->isHidden());
        window->getAction("RightDockAreaToggle")->trigger();
        QVERIFY(!first->isHidden());
        QVERIFY(second->isHidden());
    }

    void leftAreaTogglePreservesDockSizes()
    {
        isolatedLayout();
        auto* line = window->findChild<QDockWidget*>("dock_line");
        auto* circle = window->findChild<QDockWidget*>("dock_circle");
        QVERIFY(line && circle);
        window->requestedDockAreas[Qt::RightDockWidgetArea] = false;
        window->requestedDockAreas[Qt::LeftDockWidgetArea] = true;
        window->requestedDockVisibility[line->objectName()] = true;
        window->requestedDockVisibility[circle->objectName()] = true;
        window->applyDockVisibility();
        window->resize(796, 550);
        settle();
        window->resizeDocks({line}, {260}, Qt::Horizontal);
        window->resizeDocks({line, circle}, {140, 360}, Qt::Vertical);
        settle();
        const QSize lineSize = line->size();
        const QSize circleSize = circle->size();
        for (bool viaDock : {false, true}) {
            window->toggleLeftDockArea(false);
            settle();
            if (viaDock) window->requestDockVisible(line);
            else window->toggleLeftDockArea(true);
            settle();
            QCOMPARE(line->size(), lineSize);
            QCOMPARE(circle->size(), circleSize);
        }
    }

    void constrainedDockSizesSurviveRepeatedToggle()
    {
        isolatedLayout();
        auto* line = window->findChild<QDockWidget*>("dock_line");
        auto* circle = window->findChild<QDockWidget*>("dock_circle");
        QVERIFY(line && circle);
        window->requestedDockAreas[Qt::RightDockWidgetArea] = false;
        window->requestedDockAreas[Qt::LeftDockWidgetArea] = true;
        window->requestedDockVisibility[line->objectName()] = true;
        window->requestedDockVisibility[circle->objectName()] = true;
        window->applyDockVisibility();
        window->resize(796, 550);
        settle();
        window->resizeDocks({line}, {260}, Qt::Horizontal);
        window->resizeDocks({line, circle}, {140, 360}, Qt::Vertical);
        settle();
        const QSize lineSize = line->size(), circleSize = circle->size();
        window->toggleLeftDockArea(false);
        settle();
        const QVariantMap saved = window->dockAreaSizes;
        QVERIFY(!saved.isEmpty());
        window->resize(420, 400);
        settle();
        window->toggleLeftDockArea(true);
        settle();
        QCOMPARE(window->dockAreaSizes, saved);
        window->toggleLeftDockArea(false);
        settle();
        QCOMPARE(window->dockAreaSizes, saved);
        window->resize(796, 550);
        settle();
        window->toggleLeftDockArea(true);
        settle();
        QCOMPARE(line->size(), lineSize);
        QCOMPARE(circle->size(), circleSize);
        QVERIFY(window->dockAreaSizes.isEmpty());
    }

    void restoringAreaKeepsOtherDockChanges()
    {
        isolatedLayout();
        auto* line = window->findChild<QDockWidget*>("dock_line");
        QVERIFY(line);
        window->requestedDockAreas[Qt::LeftDockWidgetArea] = true;
        window->requestedDockVisibility[line->objectName()] = true;
        window->applyDockVisibility();
        auto* right = addDock("independent_right", QSize(100, 60));
        window->resize(796, 550);
        settle();
        window->resizeDocks({line}, {150}, Qt::Horizontal);
        settle();
        window->toggleLeftDockArea(false);
        settle();
        window->resizeDocks({right}, {120}, Qt::Horizontal);
        settle();
        const int width = right->width();
        window->toggleLeftDockArea(true);
        settle();
        QCOMPARE(right->width(), width);
    }

    void savedDockSizesSurviveRestart_data()
    {
        QTest::addColumn<bool>("reopenConstrained");
        QTest::newRow("area hidden") << false;
        QTest::newRow("area open in smaller window") << true;
    }

    void savedDockSizesSurviveRestart()
    {
        QFETCH(bool, reopenConstrained);
        isolatedLayout();
        auto* line = window->findChild<QDockWidget*>("dock_line");
        auto* circle = window->findChild<QDockWidget*>("dock_circle");
        QVERIFY(line && circle);
        window->requestedDockAreas[Qt::RightDockWidgetArea] = false;
        window->requestedDockAreas[Qt::LeftDockWidgetArea] = true;
        window->requestedDockVisibility[line->objectName()] = true;
        window->requestedDockVisibility[circle->objectName()] = true;
        window->applyDockVisibility();
        window->resize(796, 550);
        settle();
        window->resizeDocks({line}, {260}, Qt::Horizontal);
        window->resizeDocks({line, circle}, {140, 360}, Qt::Vertical);
        settle();
        const QSize lineSize = line->size(), circleSize = circle->size();
        window->toggleLeftDockArea(false);
        settle();
        if (reopenConstrained) {
            window->resize(420, 400);
            settle();
            window->toggleLeftDockArea(true);
            settle();
        }
        const QVariantMap saved = window->dockAreaSizes;
        QVERIFY(!saved.isEmpty());
        struct NativeSettingsGuard {
            QSettings::Format previous {QSettings::defaultFormat()};
            NativeSettingsGuard() { QSettings::setDefaultFormat(QSettings::NativeFormat); }
            ~NativeSettingsGuard() { QSettings::setDefaultFormat(previous); }
        } nativeSettings;
        window->storeSettings();
        QCOMPARE(QSettings().value("Geometry/DockAreaSizes").toMap(), saved);
        instance->reset();
        instance->reset(new QC_ApplicationWindow);
        window = instance->get();
        window->prepareWindowForShow();
        for (auto* toolbar : window->findChildren<QToolBar*>()) toolbar->hide();
        window->statusBar()->hide();
        window->show();
        settle();
        QCOMPARE(window->dockAreaSizes, saved);
        window->resize(796, 550);
        settle();
        if (!reopenConstrained) window->toggleLeftDockArea(true);
        settle();
        line = window->findChild<QDockWidget*>("dock_line");
        circle = window->findChild<QDockWidget*>("dock_circle");
        QVERIFY(line && circle);
        QCOMPARE(line->size(), lineSize);
        QCOMPARE(circle->size(), circleSize);
        QVERIFY(window->dockAreaSizes.isEmpty());
    }

    void explicitPenWizardChoiceSurvivesActivationAndRestore()
    {
        auto* pen = window->findChild<QDockWidget*>("pen_wiz");
        window->requestDockVisible(pen);
        window->slotFileNew();
        QVERIFY(window->requestedDockVisibility.value("pen_wiz"));
        QSettings settings;
        settings.setValue("Geometry/StateOfWidgets", window->dockStateForSaving());
        QVariantMap requested;
        for (auto it = window->requestedDockVisibility.cbegin(); it != window->requestedDockVisibility.cend(); ++it)
            requested.insert(it.key(), it.value());
        settings.setValue("Geometry/RequestedDockVisibility", requested);
        window->initSettings();
        QVERIFY(window->requestedDockVisibility.value("pen_wiz"));
    }

    void collapsedTabGroupRetainsSelection()
    {
        isolatedLayout();
        auto* first = addDock("tab_first");
        auto* second = addDock("tab_second");
        window->tabifyDockWidget(first, second);
        second->raise();
        addDock("tab_split");
        window->resize(796, 500);
        settle();
        const auto intended = window->captureDockGroup(second);
        window->resize(420, 500);
        settle();
        QVERIFY(first->isHidden());
        QVERIFY(second->isHidden());
        window->resize(796, 500);
        settle();
        QVERIFY(!first->isHidden());
        QVERIFY(!second->isHidden());
        const auto restored = window->captureDockGroup(second);
        QCOMPARE(restored.names, intended.names);
        QCOMPARE(restored.selected, intended.selected);
    }

    void userHiddenDockStaysHidden()
    {
        auto* dock = window->findChild<QDockWidget*>("layer_dockwidget");
        window->requestDockVisible(dock);
        dock->close();
        QVERIFY(!window->requestedDockVisibility.value(dock->objectName()));
        window->resize(1800, 1400);
        settle();
        QVERIFY(dock->isHidden());
    }

    void commandFocusSurvivesDeferredFit()
    {
        window->slotFocusCommandLine();
        settle();
        QVERIFY(!window->findChild<QDockWidget*>("command_dockwidget")->isHidden());
    }

    void hidingPriorityTabReleasesItsGroup()
    {
        isolatedLayout();
        auto* command = window->findChild<QDockWidget*>("command_dockwidget");
        auto* layers = window->findChild<QDockWidget*>("layer_dockwidget");
        window->tabifyDockWidget(layers, command);
        window->resize(420, 500);
        window->requestDockVisible(layers);
        window->requestDockVisible(command);
        settle();
        QVERIFY(!command->isHidden());
        window->requestDockVisible(command);
        window->fitDockLayout();
        QCOMPARE(window->priorityDockName, command->objectName());
        command->toggleViewAction()->trigger();
        QVERIFY(window->priorityDockName.isEmpty());
        settle();
        QVERIFY(!window->requestedDockVisibility.value(command->objectName()));
        QVERIFY(window->requestedDockVisibility.value(layers->objectName()));
        QVERIFY(layers->isHidden());
        QVERIFY(window->mdiAreaCAD->width() >= 320);
    }

    void floatingCommandIsClampedToScreen()
    {
        auto* dock = window->findChild<QDockWidget*>("command_dockwidget");
        dock->setFloating(true);
        window->requestDockVisible(dock);
        dock->move(-10000, -10000);
        settle();
        QVERIFY(qApp->primaryScreen()->availableGeometry().contains(dock->frameGeometry()));
    }

    void invalidStateUsesFactoryVisibility()
    {
        QSettings settings;
        settings.setValue("Geometry/StateOfWidgets", QByteArray("invalid"));
        settings.remove("Geometry/RequestedDockVisibility");
        window->initSettings();
        QCOMPARE(window->requestedDockVisibility, window->factoryDockVisibility);
    }

    void initialRightDockWidth_data()
    {
        QTest::addColumn<QString>("state");
        QTest::newRow("missing") << QString();
        QTest::newRow("invalid") << QString("invalid");
        QTest::newRow("saved") << QString("saved");
    }

    void initialRightDockWidth()
    {
        QFETCH(QString, state);
        window->hide();
        window->resize(1280, 900);
        auto* command = window->findChild<QDockWidget*>("command_dockwidget");
        QVERIFY(command);
        window->requestDockVisible(command);
        window->resizeDocks({command}, {600}, Qt::Horizontal);
        window->layout()->activate();
        const int savedWidth = command->width();
        QVERIFY(savedWidth > 390);
        QSettings settings;
        if (state == "saved") settings.setValue("Geometry/StateOfWidgets", window->saveState());
        else if (state.isEmpty()) settings.remove("Geometry/StateOfWidgets");
        else settings.setValue("Geometry/StateOfWidgets", state.toLatin1());
        window->initSettings();
        window->layout()->activate();
        QCOMPARE(command->width(), state == "saved" ? savedWidth : 390);
        window->resizeDocks({command}, {600}, Qt::Horizontal);
        window->layout()->activate();
        QCOMPARE(command->width(), savedWidth);
    }

    void firstRunMaximizeFitsScreen()
    {
        QSettings().setValue("Startup/FirstLoad", true);
        instance->reset(new QC_ApplicationWindow);
        window = instance->get();
        window->prepareWindowForShow();
        window->showMaximized();
        settle();
        const QSize available = qApp->primaryScreen()->availableGeometry().size();
        QVERIFY(window->width() <= available.width());
        QVERIFY(window->height() <= available.height());
        QVERIFY(window->mdiAreaCAD->width() >= qMin(480, qMax(200, window->width() - 100)));
        QVERIFY(window->mdiAreaCAD->height() >= qMin(320, qMax(160, window->height() - 100)));
    }

    void oversizedRestoredGeometryFitsScreen()
    {
        window->setGeometry(-200, -100, 1800, 1400);
        window->prepareWindowForShow();
        settle();
        QVERIFY(qApp->primaryScreen()->availableGeometry().contains(window->frameGeometry()));
    }
};

QTEST_MAIN(LC_DockLayoutTests)
#include "dock_layout_tests.moc"
