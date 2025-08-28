/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2018 Simon Wells <simonrwells@gmail.com>
** Copyright (C) 2020 Nikita Letov <letovnn@gmail.com>
** Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <clocale>

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QSplashScreen>

#include "console_dxf2pdf.h"
#include "console_dxf2png.h"
#include "lc_application.h"
#include "main.h"

#include <QDir>
#include <QPushButton>
#include <QTimer>
#include <QToolBar>

#include "lc_iconcolorsoptions.h"
#include "qc_applicationwindow.h"
#include "qg_dlginitial.h"
#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"


// fixme - sand - files - complete refactoring
namespace
{
// update splash for alpha/beta names)
    void updateSplash(const std::unique_ptr<QSplashScreen>& splash);
}

void showFirstLoadSetupDialog(bool first_load) {
    LC_GROUP_GUARD("Defaults");
    {
        QString unit = LC_GET_STR("Unit", "Invalid");
        // show initial config dialog:
        if (first_load){
            RS_DEBUG->print("main: show initial config dialog..");
            QG_DlgInitial di(nullptr);
            QPixmap pxm(":/images/intro_librecad.png");
            di.setPixmap(pxm);
            if (di.exec()) {
                unit = LC_GET_STR("Unit", "None");
            }
            RS_DEBUG->print("main: show initial config dialog: OK");
        }
    }
}

int showHelpMessage() {
    qDebug()<<"Usage: librecad [command] <options> <dxf file>";
    qDebug()<<"";
    qDebug()<<"Commands:";
    qDebug()<<"";
    qDebug()<<"  dxf2pdf\tRun librecad as console dxf2pdf tool. Use -h for help.";
    qDebug()<<"  dxf2png\tRun librecad as console dxf2png tool. Use -h for help.";
    qDebug()<<"  dxf2svg\tRun librecad as console dxf2svg tool. Use -h for help.";
    qDebug()<<"";
    qDebug()<<"Options:";
    qDebug()<<"";
    qDebug()<<"  -h, --help\tdisplay this message";
    qDebug()<<"  -d, --debug <level>";
    qDebug()<<"";
    RS_DEBUG->print( RS_Debug::D_NOTHING, "possible debug levels:");
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
    exit(0);
}

void showDebugSetupHelpMessage() {
    RS_DEBUG->print(RS_Debug::D_NOTHING, "possible debug levels:");
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
}

void initFontList() {
    RS_DEBUG->print("main: init fontlist..");
    RS_FONTLIST->init();
    RS_DEBUG->print("main: init fontlist: OK");
}

void initPatternList() {
    RS_DEBUG->print("main: init patternlist..");
    RS_PATTERNLIST->init();
    RS_DEBUG->print("main: init patternlist: OK");
}

void loadTranslations() {
    RS_DEBUG->print("main: loading translation..");

    LC_GROUP("Appearance");
    QString lang = LC_GET_STR("Language", "en");
    QString langCmd = LC_GET_STR("LanguageCmd", "en");
    LC_GROUP_END();

    RS_SYSTEM->loadTranslation(lang, langCmd);
    RS_DEBUG->print("main: loading translation: OK");
}

void initSystem(char** argv, LC_Application& app) {
    RS_DEBUG->print("param 0: %s", argv[0]);

    QFileInfo prgInfo( QFile::decodeName(argv[0]) );
    QString prgDir(prgInfo.absolutePath());

    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(), XSTR(QC_APPDIR), prgDir);
}

void loadFilesOnStartup(QSplashScreen *splash, QC_ApplicationWindow& appWin, [[maybe_unused]]LC_Application& app, QStringList fileList) {
    RS_DEBUG->print("main: loading files..");
#ifdef Q_OS_MAC
    // get the file list from LC_Application
    fileList << app.fileList();
#endif

    // reopen files that we open during last close of application
    // we'll reopen them if no explicit files to open are provided in command line

    appWin.openFilesOnStartup(fileList, splash);
    RS_DEBUG->print("main: loading files: OK");
}

void loadIconsStylingOptions() {
    LC_IconColorsOptions iconColorsOptions;
    iconColorsOptions.loadSettings();
    iconColorsOptions.applyOptions();
}

int execApplication(LC_Application& app) {
    RS_DEBUG->print("main: entering Qt event loop");
    QCoreApplication::processEvents();

    int return_code = app.exec();

    RS_DEBUG->print("main: exited Qt event loop");

    // Destroy the singleton
    QC_ApplicationWindow::getAppWindow().reset();
    return return_code;
}

//
bool setupDebugLevel(char level) {
    switch(level){
        case '?' : {
            showDebugSetupHelpMessage();
            return false;
        }
        case '0' + RS_Debug::D_NOTHING : {
            RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
            break;
        }
        case '0' + RS_Debug::D_CRITICAL : {
            RS_DEBUG->setLevel(RS_Debug::D_CRITICAL);
            break;
        }
        case '0' + RS_Debug::D_ERROR : {
            RS_DEBUG->setLevel(RS_Debug::D_ERROR);
            break;
        }
        case '0' + RS_Debug::D_WARNING : {
            RS_DEBUG->setLevel(RS_Debug::D_WARNING);
            break;
        }
        case '0' + RS_Debug::D_NOTICE : {
            RS_DEBUG->setLevel(RS_Debug::D_NOTICE);
            break;
        }
        case '0' + RS_Debug::D_INFORMATIONAL : {
            RS_DEBUG->setLevel(RS_Debug::D_INFORMATIONAL);
            break;
        }
        case '0' + RS_Debug::D_DEBUGGING : {
            RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
            break;
        }
        default : {
            RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
            break;
        }
    }
    return true;
}

// -------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        // Create menu bar
        QMenuBar* menuBar = new QMenuBar(this);
        QMenu* fileMenu = menuBar->addMenu("&File");
        QAction* exitAction = fileMenu->addAction("E&xit");
        connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

        QMenu* helpMenu = menuBar->addMenu("&Help");
        QAction* aboutAction = helpMenu->addAction("&About");
        connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

        setMenuBar(menuBar);

        // Create central widget and main layout
        QWidget* centralWidget = new QWidget(this);


        // Create left panel
        QToolBar* leftPanel = new QToolBar(this);
        // leftPanel->setFrameStyle(QFrame::Box);
        leftPanel->setMinimumWidth(100);
        leftPanel->setStyleSheet("background-color: #e0e0e0;");
        QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
        QPushButton* leftButton = new QPushButton("Left Button", leftPanel);
        leftLayout->addWidget(leftButton);
        connect(leftButton, &QPushButton::clicked, this, [](){ qDebug() << "Left button clicked"; });

        addToolBar(Qt::LeftToolBarArea, leftPanel);

        // Create center area (our original content)
        QWidget* centerArea = new QWidget(this);
        QVBoxLayout* centerLayout = new QVBoxLayout(centerArea);

        setCentralWidget(centerArea);

        // Create target widget (the one that will remain interactive)
        targetWidget = new QPushButton("Click Me (Target Widget)", centerArea);
        targetWidget->setMinimumSize(200, 50);
        connect(targetWidget, &QPushButton::clicked, this, &MainWindow::onTargetClicked);

        // Create other widgets that should be blocked
        otherWidget1 = new QPushButton("Other Button (Should Be Blocked)", centerArea);
        otherWidget1->setMinimumSize(200, 50);
        connect(otherWidget1, &QPushButton::clicked, this, [](){ qDebug() << "Other button clicked"; });

        otherWidget2 = new QLabel("This is a label (should be blocked too)", centerArea);
        otherWidget2->setAlignment(Qt::AlignCenter);
        otherWidget2->setMinimumSize(200, 50);
        otherWidget2->setFrameStyle(QFrame::Box);

        // Add widgets to center layout
        centerLayout->addWidget(targetWidget);
        centerLayout->addWidget(otherWidget1);
        centerLayout->addWidget(otherWidget2);

        // Create right panel
        QToolBar* rightPanel = new QToolBar(this);
        // rightPanel->setFrameStyle(QFrame::Box);
        rightPanel->setMinimumWidth(100);
        rightPanel->setStyleSheet("background-color: #e0e0e0;");
        QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
        QPushButton* rightButton = new QPushButton("Right Button", rightPanel);
        rightLayout->addWidget(rightButton);
        connect(rightButton, &QPushButton::clicked, this, [](){ qDebug() << "Right button clicked"; });

        addToolBar(Qt::RightToolBarArea, rightPanel);

        // Create top panel
        QToolBar* topPanel = new QToolBar(this);
        // topPanel->setFrameStyle(QFrame::Box);
        topPanel->setMinimumHeight(50);
        topPanel->setStyleSheet("background-color: #d0d0d0;");
        QHBoxLayout* topLayout = new QHBoxLayout(topPanel);
        QPushButton* topButton = new QPushButton("Top Button", topPanel);
        topLayout->addWidget(topButton);
        connect(topButton, &QPushButton::clicked, this, [](){ qDebug() << "Top button clicked"; });

        addToolBar(Qt::TopToolBarArea, topPanel);

        // Create bottom panel
        QToolBar* bottomPanel = new QToolBar(this);
        // bottomPanel->setFrameStyle(QFrame::Box);
        bottomPanel->setMinimumHeight(50);
        bottomPanel->setStyleSheet("background-color: #d0d0d0;");
        QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
        QPushButton* bottomButton = new QPushButton("Bottom Button", bottomPanel);
        bottomLayout->addWidget(bottomButton);
        connect(bottomButton, &QPushButton::clicked, this, [](){ qDebug() << "Bottom button clicked"; });
        addToolBar(Qt::BottomToolBarArea, bottomPanel);

        // Create a grid layout for the main content
        QGridLayout* contentLayout = new QGridLayout();
        // contentLayout->addWidget(topPanel, 0, 0, 1, 3);
        // contentLayout->addWidget(leftPanel, 1, 0);
        // contentLayout->addWidget(centerArea, 1, 1);
        // contentLayout->addWidget(rightPanel, 1, 2);
        // contentLayout->addWidget(bottomPanel, 2, 0, 1, 3);

        // Set the content layout to the central widget
        // centralWidget->setLayout(contentLayout);
        // setCentralWidget(centralWidget);


        resize(600, 400);

        // Create overlay (initially hidden)
        overlay = new OverlayWidget(centralWidget);
        overlay->hide();

        // Show the modal dialog initially
        QTimer::singleShot(100, this, &MainWindow::showModalDialog);
    }


    void MainWindow::resizeEvent(QResizeEvent* event)     {
        QMainWindow::resizeEvent(event);
        if (overlay) {
            overlay->setGeometry(centralWidget()->geometry());
        }
    }


    void MainWindow::showModalDialog()
    {
        QDialog dialog(this);
        dialog.setWindowTitle("Modal Dialog");
        dialog.setModal(true);

        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        QLabel* label = new QLabel("Click the button to proceed to target selection", &dialog);
        QPushButton* button = new QPushButton("Proceed", &dialog);

        layout->addWidget(label);
        layout->addWidget(button);

        connect(button, &QPushButton::clicked, &dialog, &QDialog::accept);

        if (dialog.exec() == QDialog::Accepted) {
            // Dialog accepted - enable only target widget
            enableOnlyTargetWidget();
        }
    }

    void MainWindow::onTargetClicked()
    {
        // Target widget was clicked - restore UI and show dialog again
        qDebug() << "Target widget clicked!";
        restoreUI();
        showModalDialog();
    }

    void MainWindow::showAboutDialog()
    {
        QDialog aboutDialog(this);
        aboutDialog.setWindowTitle("About");
        aboutDialog.setModal(true);

        QVBoxLayout* layout = new QVBoxLayout(&aboutDialog);
        QLabel* label = new QLabel("This is a demo application showing how to restrict\n"
                                  "interaction to a single widget after a modal dialog.", &aboutDialog);
        QPushButton* closeButton = new QPushButton("Close", &aboutDialog);

        layout->addWidget(label);
        layout->addWidget(closeButton);

        connect(closeButton, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

        aboutDialog.exec();
    }


    void MainWindow::enableOnlyTargetWidget()
    {
        // Disable all widgets in the central widget except the target
        QList<QWidget*> widgets = centralWidget()->findChildren<QWidget*>();
        for (QWidget* widget : widgets) {
            if (widget != targetWidget && widget != overlay) {
                widget->setEnabled(false);
            }
        }

        // Disable menu bar
        menuBar()->setEnabled(false);

        // Show overlay to dim the background
        overlay->raise();
        overlay->show();

        // Install event filter to block input to other widgets
        qApp->installEventFilter(this);
    }

    void MainWindow::restoreUI()
    {
        // Re-enable all widgets in the central widget
        QList<QWidget*> widgets = centralWidget()->findChildren<QWidget*>();
        for (QWidget* widget : widgets) {
            widget->setEnabled(true);
        }

        // Re-enable menu bar
        menuBar()->setEnabled(true);

        // Hide overlay
        overlay->hide();

        // Remove event filter
        qApp->removeEventFilter(this);
    }

    bool MainWindow::eventFilter(QObject* obj, QEvent* event)
    {
        // Block all mouse events for widgets other than the target
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonDblClick ||
            event->type() == QEvent::MouseButtonRelease) {

            QWidget* widget = qobject_cast<QWidget*>(obj);
            if (widget && widget != targetWidget &&
                widget != overlay && // Allow clicks on overlay (they'll be blocked by the overlay itself)
                !widget->isWindow()) { // Don't block window events

                // Check if the widget is a child of the target (we want to allow those too)
                bool isChildOfTarget = false;
                QWidget* parent = widget->parentWidget();
                while (parent) {
                    if (parent == targetWidget) {
                        isChildOfTarget = true;
                        break;
                    }
                    parent = parent->parentWidget();
                }

                if (!isChildOfTarget) {
                    // Block the event
                    return true;
                }
            }
        }

        // Also block menu events when in restricted mode
        if (event->type() == QEvent::ActionChanged ||
            event->type() == QEvent::ActionAdded ||
            event->type() == QEvent::ActionRemoved) {
            if (qobject_cast<QMenu*>(obj) || qobject_cast<QMenuBar*>(obj)) {
                return true;
            }
        }

        return QObject::eventFilter(obj, event);
    }
// ----

#define QUICK_TEST_

/**
 * Main. Creates Application window.
 */
 // fixme - sand - refactor and split to several specialized functions
#ifndef BUILD_TESTS
int main(int argc, char** argv) {
#ifdef QUICK_TEST
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
#    else

    QT_REQUIRE_VERSION(argc, argv, "5.2.1");

    // Check first two arguments in order to decide if we want to run librecad
    // as console dxf2pdf or dxf2png tools. On Linux we can create a link to
    // librecad executable and  name it dxf2pdf. So, we can run either:
    //
    //     librecad dxf2pdf [options] ...
    //
    // or just:
    //
    //     dxf2pdf [options] ...
    //
    for (int i = 0; i < qMin(argc, 2); i++) {
        QString arg(argv[i]);
        if (i == 0) {
            arg = QFileInfo(QFile::decodeName(argv[i])).baseName();
        }
        if (arg.compare("dxf2pdf") == 0) {
            return console_dxf2pdf(argc, argv);
        }
        if (arg.compare("dxf2png") == 0 || arg == "dxf2svg") {
            return console_dxf2png(argc, argv);
        }
    }

    RS_DEBUG->setLevel(RS_Debug::D_WARNING);

    LC_Application app(argc, argv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    // fixme - sand - NEED TO CHECK WHERE lc_svgicons.so is located under linux and mac!!! That's tested for Windows
    auto appDir = app.applicationDirPath();
    auto inconEnginesDir = appDir + "/iconengines";
    app.addLibraryPath(inconEnginesDir);

    RS_Settings::init(app.organizationName(), app.applicationName());

    QGuiApplication::setDesktopFileName("librecad");

    loadIconsStylingOptions();

    bool first_load = LC_GET_ONE_BOOL("Startup", "FirstLoad", true);

    bool allowOptions=true;
    QList<int> argClean;

    for (int i=0; i<argc; i++)   {
        QString argstr(argv[i]);
        if(allowOptions&&QString::compare("--", argstr)==0){
            allowOptions=false;
            continue;
        }
        const QString help0("-h"), help1("--help");
        if (allowOptions && (help0.compare(argstr, Qt::CaseInsensitive)==0 ||
                             help1.compare(argstr, Qt::CaseInsensitive)==0 )) {
            return showHelpMessage();
        }
        const QString lpDebugSwitch0("-d"),lpDebugSwitch1("--debug") ;

        if (allowOptions&& (argstr.startsWith(lpDebugSwitch0, Qt::CaseInsensitive) ||
                             argstr.startsWith(lpDebugSwitch1, Qt::CaseInsensitive) )){
            argClean<<i;

            // to control the level of debugging output use --debug with level 0-6, e.g. --debug3
            // for a list of debug levels use --debug?
            // if no level follows, the debugging level is set
            argstr.remove(QRegularExpression("^"+lpDebugSwitch0));
            argstr.remove(QRegularExpression("^"+lpDebugSwitch1));
            char level = '3';
            if(argstr.size()==0){
                if(i+1<argc){
                    if(QRegularExpression(R"(\d*)").match(argv[i+1]).hasMatch()){
                        ++i;
                        qDebug()<<"reading "<<argv[i]<<" as debugging level";
                        level=argv[i][0];
                        argClean<<i;
                    }
                    else {
                        level = '3';
                    }
                }
                else {
                    level = '3'; //default to D_WARNING
                }
            }
            else {
                level = argstr.toStdString()[0];
            }

            if (!setupDebugLevel(level)) {
                return 0;
            }
        }
    }
    initSystem(argv, app);
    showFirstLoadSetupDialog(first_load);

    std::unique_ptr<QSplashScreen> splash;
    bool show_splash = LC_GET_ONE_BOOL("Startup","ShowSplash", true);

    if (show_splash){
        splash = std::make_unique<QSplashScreen>();
        updateSplash(splash);
        app.processEvents();
        RS_DEBUG->print("main: splashscreen: OK");
    }

    initFontList();
    initPatternList();
    loadTranslations();

    RS_DEBUG->print("main: creating main window..");
    QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow();
    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    if (appWindow != nullptr) {
        appWindow->fireIconsRefresh();
    }
#ifdef Q_OS_MAC
    app.installEventFilter(&appWin);
#endif
    RS_DEBUG->print("main: setting caption");
    appWin.setWindowTitle(app.applicationName());

    RS_DEBUG->print("main: show main window");

    QSettings settings; // fixme - direct invocation of settings
    settings.beginGroup("Defaults");
    if( !settings.contains("UseQtFileOpenDialog")) {
#ifdef Q_OS_LINUX
        // on Linux don't use native file dialog
        // because of case insensitive filters (issue #791)
        settings.setValue("UseQtFileOpenDialog", QVariant(1));
#else
        settings.setValue("UseQtFileOpenDialog", QVariant(0));
#endif
    }
    settings.endGroup();

    bool maximize = LC_GET_ONE_BOOL("Startup","Maximize", false);

    if (maximize || first_load) {
        appWin.showMaximized();
    }
    else {
        appWin.show();
    }

    RS_DEBUG->print("main: set focus");
    appWin.setFocus();
    RS_DEBUG->print("main: creating main window: OK");

    if (splash != nullptr){
        RS_DEBUG->print("main: updating splash");
        splash->raise();
        splash->showMessage(QObject::tr("Loading..."), Qt::AlignRight|Qt::AlignBottom, Qt::black);
        RS_DEBUG->print("main: processing events");
        qApp->processEvents();
        RS_DEBUG->print("main: updating splash: OK");
    }

    // Set LC_NUMERIC so that entering numeric values uses . as the decimal separator
    setlocale(LC_NUMERIC, "C");

    // parse command line arguments that might not need a launched program:
    // fixme - sand - add support of skipping of loading via cmdline flag
    QStringList fileList = handleArgs(argc, argv, argClean);
    loadFilesOnStartup(splash.get(), appWin, app, fileList);

    appWin.initCompleted();

    if (splash != nullptr) {
        splash->finish(&appWin);
        splash.release();
    }

    LC_GROUP("Startup");
    {
        // fixme - sand - files - add support of command line flag to suppress version check (may be useful for automation)!
        bool checkForNewVersion = LC_GET_BOOL("CheckForNewVersions", true);
        if (checkForNewVersion) {
            appWin.checkForNewVersion();
        }

        if (first_load){
            LC_SET("FirstLoad", false);
        }
    }
    LC_GROUP_END();

    return execApplication(app);
#    endif
}
#endif // BUILD_TESTS

/**
 * Handles command line arguments that might not require a GUI.
 *
 * @return list of files to load on startup.
 */
QStringList handleArgs(int argc, char** argv, const QList<int>& argClean){
    RS_DEBUG->print("main: handling args..");
    QStringList ret;

    bool doexit = false;
    for (int i = 1; i < argc; i++) {
        if (argClean.indexOf(i) >= 0) {
            continue;
        }
        auto localFileName = argv[i];
        if (!QString(localFileName).startsWith("-")) {
            auto decodedName = QFile::decodeName(localFileName);
            QFileInfo fileInfo(decodedName);
            auto absolutePath = fileInfo.absoluteFilePath();
            QString fname = QDir::toNativeSeparators(absolutePath);
            ret.append(fname);
        }
        else if (QString(localFileName) == "--exit") {
            doexit = true;
        }
    }
    if (doexit) {
        exit(0);
    }
    RS_DEBUG->print("main: handling args: OK");
    return ret;
}

QString LCReleaseLabel(){
    QString version{XSTR(LC_VERSION)};
    QString label;
    const std::map<QString, QString> labelMap = {
        {"rc", QObject::tr("Release Candidate")},
        {"beta", QObject::tr("BETA")},
        {"alpha", QObject::tr("ALPHA")}
    };
    for (const auto& [key, value]: labelMap) {
        if (version.contains(key, Qt::CaseInsensitive)) {
            label=value;
            break;
        }
    }
    return label;
}

namespace {

// Update Splash image to show "ALPHA", "BETA", and "Release Candidate"
QPixmap getSplashImage(const std::unique_ptr<QSplashScreen>& splash, const QString& label);
// Update Splash Screen
    void updateSplash(const std::unique_ptr<QSplashScreen>& splash)
    {
        if (splash == nullptr)
            return;

    QString label = LCReleaseLabel();
        if (label.isEmpty())
            return;

        QPixmap splashImage = getSplashImage(splash, label);
        splash->setPixmap(splashImage);
        splash->setAttribute(Qt::WA_DeleteOnClose);
        splash->show();
        splash->showMessage(QObject::tr("Loading.."),
                            Qt::AlignRight|Qt::AlignBottom, Qt::black);
    }

// Update Splash image to show "ALPHA", "BETA", and "Release Candidate"
    QPixmap getSplashImage(const std::unique_ptr<QSplashScreen>& splash, const QString& label)
    {
        if (splash == nullptr)
            return {};

        QPixmap pixmapSplash(":/images/splash_librecad.png");
        QPainter painter(&pixmapSplash);
        const double factorX = pixmapSplash.width()/542.;
        const double factorY = pixmapSplash.height()/337.;
        painter.setPen(QColor(255, 0, 0, 128));
        QRectF labelRect{QPointF{280.*factorX, 130.*factorY}, QPointF{480.*factorX, 170.*factorY}};
        QFont font;
        font.setPixelSize(int(labelRect.height()) - 2);
        painter.setFont(font);
        painter.drawText(labelRect,Qt::AlignRight, label);
        return pixmapSplash;
    }
}
