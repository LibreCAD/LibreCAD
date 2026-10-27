// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>
#include <QtWidgets>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_exitdialog.h"
#include "qg_graphicview.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

// Closing drawings, print previews and block editors. doClose() takes a window out of
// the MDI area; with a style sheet, Qt showed a maximized or minimized window again as
// a window of its own, which kept the application from quitting (#2004, #1810). The
// windows are deleted now, so these tests also check that nothing uses them afterwards.
class LC_MdiCloseTests : public QObject {
    Q_OBJECT

    QTemporaryDir settingsDirectory;
    QTemporaryDir files;
    QC_ApplicationWindow* window {nullptr};
    std::unique_ptr<QC_ApplicationWindow>* instance {nullptr};

    QList<QMdiSubWindow*> subWindows() const
    {
        return window->getMdiArea()->subWindowList();
    }

    // Drawing windows that left the MDI area and are shown as windows of their own.
    static QList<QC_MDIWindow*> detachedShown()
    {
        QList<QC_MDIWindow*> shown;
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            auto* mdi = qobject_cast<QC_MDIWindow*>(widget);
            if (mdi != nullptr && mdi->isVisible())
                shown << mdi;
        }
        return shown;
    }

    static QList<QC_MDIWindow*> liveWindows()
    {
        QList<QC_MDIWindow*> live;
        for (QWidget* widget : QApplication::allWidgets()) {
            if (auto* mdi = qobject_cast<QC_MDIWindow*>(widget))
                live << mdi;
        }
        return live;
    }

    static void deleteClosedWindows()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    QC_MDIWindow* addBlockEditor(QC_MDIWindow* drawing)
    {
        RS_Graphic* graphic = drawing->getGraphic();
        auto* block = new RS_Block(graphic, RS_BlockData("mdi_close_block", RS_Vector(0., 0.), false));
        graphic->addBlock(block);
        graphic->getBlockList()->activate(block);
        RS_DIALOGFACTORY->requestEditBlockWindow(graphic->getBlockList());
        return window->getWindowWithDoc(block);
    }

    QString writeFile(const QString& name, const QByteArray& content)
    {
        const QString path = files.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
            return QString();
        return path;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(settingsDirectory.isValid());
        QVERIFY(files.isValid());
        qApp->setQuitOnLastWindowClosed(false);
        // The tests answer dialogs from a timer, which cannot reach a native one.
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDirectory.path());
        QCoreApplication::setOrganizationName(QString("LibreCAD-MdiCloseTests-%1").arg(qApp->applicationPid()));
        QCoreApplication::setApplicationName("mdi_close");
        QCoreApplication::setApplicationVersion("2.2.1");
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        RS_SYSTEM->init("LibreCAD", "2.2.1", "librecad", QString(LIBRECAD_TEST_SOURCE_DIR) + "/unix");
        RS_FONTLIST->init();
        RS_PATTERNLIST->init();
        // Any style sheet makes the reparenting in QMdiArea::removeSubWindow() send a
        // StyleChange, which is what showed the closed windows again.
        qApp->setStyleSheet("QMenu { font-size: 24px; }");
    }

    void cleanupTestCase()
    {
        qApp->setStyleSheet(QString());
        // RS_Settings writes the native settings of this organization name, and every main
        // window writes librecad.alias to its application data directory: remove both. On
        // macOS the preferences daemon may still leave an empty preferences file.
        QSettings native(qApp->organizationName(), qApp->applicationName());
        const QString nativeFile = native.fileName();
        native.clear();
        native.sync();
        if (QFileInfo(nativeFile).isFile()) {
            QFile::remove(nativeFile);
            QDir().rmdir(QFileInfo(nativeFile).path());
        }
        const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir(data).removeRecursively();
        QDir().rmdir(QFileInfo(data).path());
    }

    void init()
    {
        QSettings settings;
        settings.setValue("Startup/FirstLoad", false);
        settings.setValue("Defaults/AutoBackupDocument", false);
        if (!instance) instance = &QC_ApplicationWindow::getAppWindow();
        else instance->reset(new QC_ApplicationWindow);
        window = instance->get();
        window->prepareWindowForShow();
        window->resize(700, 500);
        window->show();
        QTest::qWait(100);
    }

    void cleanup()
    {
        deleteClosedWindows();
        // Only a failing test leaves closed windows behind: delete print previews and
        // block editors before the drawings they show.
        QList<QPointer<QC_MDIWindow>> leftovers;
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* mdi = qobject_cast<QC_MDIWindow*>(widget))
                leftovers << mdi;
        }
        for (bool children : {true, false}) {
            for (const QPointer<QC_MDIWindow>& mdi : leftovers) {
                if (!mdi.isNull() && (mdi->getParentWindow() != nullptr) == children)
                    delete mdi.data();
            }
        }
        instance->reset();
        window = nullptr;
        deleteClosedWindows();
    }

    void closeAllDeletesTheDrawings()
    {
        QPointer<QC_MDIWindow> first = window->slotFileNew();
        QPointer<QC_MDIWindow> second = window->slotFileNew();
        second->showMaximized();
        QVERIFY(window->slotFileCloseAll());
        QVERIFY(subWindows().isEmpty());
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(first.isNull());
        QVERIFY(second.isNull());
    }

    void closeAllDoesNotActivateAMinimizedDrawing()
    {
        QPointer<QC_MDIWindow> minimized = window->slotFileNew();
        QPointer<QC_MDIWindow> maximized = window->slotFileNew();
        window->getMdiArea()->setActiveSubWindow(minimized);
        minimized->showMinimized();
        QTest::qWait(50);
        window->getMdiArea()->setActiveSubWindow(maximized);
        maximized->showMaximized();
        QTest::qWait(50);
        QVERIFY(minimized->isMinimized());
        QVERIFY(maximized->isMaximized());
        QCOMPARE(window->getMdiArea()->activeSubWindow(), maximized.data());
        QMdiSubWindow* const closing = minimized.data();
        int activations = 0;
        QObject connectionScope; // destroyed before `activations`, on every return path
        connect(window->getMdiArea(), &QMdiArea::subWindowActivated, &connectionScope,
                [closing, &activations](QMdiSubWindow* activated) { activations += activated == closing; });
        QVERIFY(window->slotFileCloseAll());
        QCOMPARE(activations, 0);
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(minimized.isNull());
        QVERIFY(maximized.isNull());
    }

    void printPreviewOffHandsTheViewBack()
    {
        QC_MDIWindow* drawing = window->slotFileNew();
        window->slotFilePrintPreview(true);
        QPointer<QC_MDIWindow> preview = window->getMDIWindow();
        QVERIFY(preview != nullptr);
        QCOMPARE(preview->getParentWindow(), drawing);
        window->slotFilePrintPreview(false);
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(preview.isNull());
        // The preview's view made itself the drawing's view. A block editor saves through
        // the drawing, and the DXF writer reads that view.
        QCOMPARE(drawing->getGraphic()->getGraphicView(),
                 static_cast<RS_GraphicView*>(drawing->getGraphicView()));
    }

    void closingADrawingDeletesItsPreviewAndBlockEditor()
    {
        QPointer<QC_MDIWindow> drawing = window->slotFileNew();
        QPointer<QC_MDIWindow> editor = addBlockEditor(drawing);
        QVERIFY(editor != nullptr);
        window->getMdiArea()->setActiveSubWindow(drawing);
        window->slotFilePrintPreview(true);
        QPointer<QC_MDIWindow> preview = window->getMDIWindow();
        QVERIFY(preview != nullptr && preview != drawing);
        window->getMdiArea()->setActiveSubWindow(preview);
        preview->showMinimized();
        QTest::qWait(50);
        window->getMdiArea()->setActiveSubWindow(drawing);
        drawing->showMaximized();
        QTest::qWait(50);
        QVERIFY(preview->isMinimized());
        drawing->getGraphic()->setModified(false);
        QVERIFY(drawing->close()); // File > Close
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(drawing.isNull());
        QVERIFY(editor.isNull());
        QVERIFY(preview.isNull());
    }

    void failedOpenDeletesTheWindow()
    {
        const QString path = writeFile("drawing.unknown", "not a drawing");
        QVERIFY(!path.isEmpty());
        window->slotFileOpen(path, RS2::FormatUnknown);
        QVERIFY(subWindows().isEmpty());
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(liveWindows().isEmpty());
    }

    // Close All delivered by the events that slotFileOpen() processes while it sets up the
    // new window: the window is deleted before slotFileOpen() goes on. Without the guard
    // this is a use-after-free, which AddressSanitizer reports.
    void closeAllWhileAFileOpens()
    {
        const QString path = QString(LIBRECAD_TEST_SOURCE_DIR) + "/librecad/support/library/misc/a3.dxf";
        QVERIFY(QFileInfo::exists(path));
        QTimer::singleShot(0, window, [this]() { window->slotFileCloseAll(); });
        window->slotFileOpen(path, RS2::FormatUnknown);
        QVERIFY(subWindows().isEmpty());
        QVERIFY(QApplication::overrideCursor() == nullptr);
        deleteClosedWindows();
        QVERIFY(detachedShown().isEmpty());
        QVERIFY(liveWindows().isEmpty());
    }

    // The same when QC_MDIWindow::slotFileOpen() processes the events, while the drawing
    // window loads the file: without its guard, the deleted window opens the file.
    void closeAllWhileADrawingWindowLoadsAFile()
    {
        const QString path = QString(LIBRECAD_TEST_SOURCE_DIR) + "/librecad/support/library/misc/a3.dxf";
        QVERIFY(QFileInfo::exists(path));
        QPointer<QC_MDIWindow> drawing = window->slotFileNew();
        QTimer::singleShot(0, window, [this]() { window->slotFileCloseAll(); });
        QVERIFY(!drawing->slotFileOpen(path, RS2::FormatUnknown));
        QVERIFY(subWindows().isEmpty());
        deleteClosedWindows();
        QVERIFY(drawing.isNull());
        QVERIFY(detachedShown().isEmpty());
    }

    // The same while File > New sets up the new drawing.
    void closeAllWhileANewDrawingIsSetUp()
    {
        QTimer::singleShot(0, window, [this]() { window->slotFileCloseAll(); });
        window->slotFileNewNew();
        QVERIFY(subWindows().isEmpty());
        QVERIFY(QApplication::overrideCursor() == nullptr);
        deleteClosedWindows();
        QVERIFY(detachedShown().isEmpty());
        QVERIFY(liveWindows().isEmpty());
    }

    // Save All in the close prompt, then Cancel in the Save As dialog of the unnamed
    // drawing: the drawing must stay open, and it is asked about once.
    void failedSaveAllKeepsTheDrawing()
    {
        QPointer<QC_MDIWindow> drawing = window->slotFileNew();
        QPointer<QC_MDIWindow> editor = addBlockEditor(drawing);
        QVERIFY(editor != nullptr);
        QPointer<QC_MDIWindow> other = window->slotFileNew();
        drawing->getGraphic()->setModified(true);

        int closePrompts = 0;
        int fileDialogs = 0;
        QTimer answer;
        answer.setInterval(20);
        connect(&answer, &QTimer::timeout, this, [&]() {
            QWidget* modal = QApplication::activeModalWidget();
            if (auto* prompt = qobject_cast<QG_ExitDialog*>(modal)) {
                ++closePrompts;
                prompt->done(closePrompts == 1 ? QG_ExitDialog::SaveAll : QG_ExitDialog::Cancel);
            } else if (auto* dialog = qobject_cast<QDialog*>(modal)) {
                if (qobject_cast<QFileDialog*>(modal) != nullptr)
                    ++fileDialogs;
                dialog->reject();
            }
        });
        answer.start();
        const bool closed = window->slotFileCloseAll();
        answer.stop();

        QVERIFY(!closed);
        QCOMPARE(closePrompts, 1);
        QCOMPARE(fileDialogs, 1);
        QVERIFY(detachedShown().isEmpty());
        deleteClosedWindows();
        QVERIFY(!drawing.isNull());
        QVERIFY(subWindows().contains(drawing.data()));
        QVERIFY(!drawing->isWindow());
        QVERIFY(drawing->getDocument()->isModified());
        drawing->getGraphic()->setModified(false);
    }

    // doClose() takes a drawing's block editors out of the window list with the drawing:
    // Close All must not reach them again.
    void closeAllClosesEachWindowOnce()
    {
        QPointer<QC_MDIWindow> drawing = window->slotFileNew();
        QPointer<QC_MDIWindow> editor = addBlockEditor(drawing); // listed after its drawing
        QVERIFY(editor != nullptr);
        QPointer<QC_MDIWindow> other = window->slotFileNew();
        drawing->getGraphic()->setModified(false);
        // QMdiArea::removeSubWindow() warns about a window it no longer holds.
        static QStringList warnings;
        static QtMessageHandler previous = nullptr;
        warnings.clear();
        previous = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
            if (type == QtWarningMsg)
                warnings << message;
            if (previous != nullptr)
                previous(type, context, message);
        });
        const bool closed = window->slotFileCloseAll();
        qInstallMessageHandler(previous);
        QVERIFY(closed);
        for (const QString& warning : qAsConst(warnings))
            QVERIFY2(!warning.contains("not inside workspace"), qPrintable(warning));
        QVERIFY(subWindows().isEmpty());
        deleteClosedWindows();
        QVERIFY(drawing.isNull());
        QVERIFY(editor.isNull());
        QVERIFY(other.isNull());
    }
};

QTEST_MAIN(LC_MdiCloseTests)
#include "mdi_close_tests.moc"
