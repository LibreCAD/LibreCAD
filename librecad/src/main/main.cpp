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
#include "main.h"

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QSplashScreen>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>

#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "qg_dlginitial.h"

#include "lc_application.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"

#include "console_dxf2pdf.h"
#include "console_dxf2png.h"

namespace
{
void restoreWindowGeometry(QC_ApplicationWindow& appWin, QSettings& settings);
}
/**
 * Main. Creates Application window.
 */
int main(int argc, char** argv)
{
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    QGuiApplication::setDesktopFileName("librecad.desktop");
#endif

    QSettings settings;

    bool first_load = settings.value("Startup/FirstLoad", 1).toBool();

    const QString lpDebugSwitch0("-d"),lpDebugSwitch1("--debug") ;
    const QString help0("-h"), help1("--help");
    bool allowOptions=true;
    QList<int> argClean;
    for (int i=0; i<argc; i++)
    {
        QString argstr(argv[i]);
        if(allowOptions&&QString::compare("--", argstr)==0)
        {
            allowOptions=false;
            continue;
        }
        if (allowOptions && (help0.compare(argstr, Qt::CaseInsensitive)==0 ||
                             help1.compare(argstr, Qt::CaseInsensitive)==0 ))
        {
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
        if ( allowOptions&& (argstr.startsWith(lpDebugSwitch0, Qt::CaseInsensitive) ||
                             argstr.startsWith(lpDebugSwitch1, Qt::CaseInsensitive) ))
        {
            argClean<<i;

            // to control the level of debugging output use --debug with level 0-6, e.g. --debug3
            // for a list of debug levels use --debug?
            // if no level follows, the debugging level is set
            argstr.remove(QRegularExpression("^"+lpDebugSwitch0));
            argstr.remove(QRegularExpression("^"+lpDebugSwitch1));
            char level;
            if(argstr.size()==0)
            {
                if(i+1<argc)
                {
                    if(QRegularExpression(R"(\d*)").match(argv[i+1]).hasMatch())
                    {
                        ++i;
                        qDebug()<<"reading "<<argv[i]<<" as debugging level";
                        level=argv[i][0];
                        argClean<<i;
                    }
                    else
                        level='3';
                }
                else
                    level='3'; //default to D_WARNING
            }
            else
                level=argstr.toStdString()[0];

            switch(level)
            {
            case '?' :
                RS_DEBUG->print( RS_Debug::D_NOTHING, "possible debug levels:");
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
                RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
                return 0;

            case '0' + RS_Debug::D_NOTHING :
                RS_DEBUG->setLevel( RS_Debug::D_NOTHING);
                break;

            case '0' + RS_Debug::D_CRITICAL :
                RS_DEBUG->setLevel( RS_Debug::D_CRITICAL);
                break;

            case '0' + RS_Debug::D_ERROR :
                RS_DEBUG->setLevel( RS_Debug::D_ERROR);
                break;

            case '0' + RS_Debug::D_WARNING :
                RS_DEBUG->setLevel( RS_Debug::D_WARNING);
                break;

            case '0' + RS_Debug::D_NOTICE :
                RS_DEBUG->setLevel( RS_Debug::D_NOTICE);
                break;

            case '0' + RS_Debug::D_INFORMATIONAL :
                RS_DEBUG->setLevel( RS_Debug::D_INFORMATIONAL);
                break;

            case '0' + RS_Debug::D_DEBUGGING :
                RS_DEBUG->setLevel( RS_Debug::D_DEBUGGING);
                break;

            default :
                RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
                break;
            }
        }
    }
    RS_DEBUG->print("param 0: %s", argv[0]);

    QFileInfo prgInfo( QFile::decodeName(argv[0]) );
    QString prgDir(prgInfo.absolutePath());
    RS_SETTINGS->init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(), XSTR(QC_APPDIR), prgDir);

    // parse command line arguments that might not need a launched program:
    QStringList fileList = handleArgs(argc, argv, argClean);

    QString unit = settings.value("Defaults/Unit", "Invalid").toString();

    // show initial config dialog:
    if (first_load)
    {
        RS_DEBUG->print("main: show initial config dialog..");
        QG_DlgInitial di(nullptr);
        QPixmap pxm(":/main/intro_librecad.png");
        di.setPixmap(pxm);
        if (di.exec())
        {
            RS_SETTINGS->beginGroup("/Defaults");
            unit = RS_SETTINGS->readEntry("/Unit", "None");
            RS_SETTINGS->endGroup();
        }
        RS_DEBUG->print("main: show initial config dialog: OK");
    }

    auto splash = new QSplashScreen;

    bool show_splash = settings.value("Startup/ShowSplash", 1).toBool();

    if (show_splash)
    {
        QPixmap pixmap(":/main/splash_librecad.png");
        splash->setPixmap(pixmap);
        splash->setAttribute(Qt::WA_DeleteOnClose);
        splash->show();
        splash->showMessage(QObject::tr("Loading.."),
                            Qt::AlignRight|Qt::AlignBottom, Qt::black);
        app.processEvents();
        RS_DEBUG->print("main: splashscreen: OK");
    }

    RS_DEBUG->print("main: init fontlist..");
    RS_FONTLIST->init();
    RS_DEBUG->print("main: init fontlist: OK");

    RS_DEBUG->print("main: init patternlist..");
    RS_PATTERNLIST->init();
    RS_DEBUG->print("main: init patternlist: OK");

    RS_DEBUG->print("main: loading translation..");

    settings.beginGroup("Appearance");
    QString lang = settings.value("Language", "en").toString();
    QString langCmd = settings.value("LanguageCmd", "en").toString();
    settings.endGroup();

    RS_SYSTEM->loadTranslation(lang, langCmd);
    RS_DEBUG->print("main: loading translation: OK");

    RS_DEBUG->print("main: creating main window..");
    QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow();
#ifdef Q_OS_MAC
    app.installEventFilter(&appWin);
#endif
    RS_DEBUG->print("main: setting caption");
    appWin.setWindowTitle(app.applicationName());

    RS_DEBUG->print("main: show main window");

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

    if (!first_load)
        restoreWindowGeometry(appWin, settings);

    bool maximize = settings.value("Startup/Maximize", 0).toBool();

    if (maximize || first_load)
        appWin.showMaximized();
    else
        appWin.show();

    RS_DEBUG->print("main: set focus");
    appWin.setFocus();
    RS_DEBUG->print("main: creating main window: OK");

    if (show_splash)
    {
        RS_DEBUG->print("main: updating splash");
        splash->raise();
        splash->showMessage(QObject::tr("Loading..."),
                Qt::AlignRight|Qt::AlignBottom, Qt::black);
        RS_DEBUG->print("main: processing events");
        qApp->processEvents();
        RS_DEBUG->print("main: updating splash: OK");
    }

    // Set LC_NUMERIC so that entering numeric values uses . as the decimal separator
    setlocale(LC_NUMERIC, "C");

    RS_DEBUG->print("main: loading files..");
#ifdef Q_OS_MAC
    // get the file list from LC_Application
    fileList << app.fileList();
#endif

    // reopen files that we open during last close of application
    // we'll reopen them if no explicit files to open are provided in command line
    RS_SETTINGS->beginGroup("/Startup");
    bool reopenLastFiles = RS_SETTINGS->readNumEntry("/OpenLastOpenedFiles", 0) == 1;
    QString lastFiles = RS_SETTINGS->readEntry("/LastOpenFilesList", "");
    QString activeFile = RS_SETTINGS->readEntry("/LastOpenFilesActive", "");
    RS_SETTINGS->endGroup();

    if (reopenLastFiles && fileList.isEmpty() && !lastFiles.isEmpty()){
        foreach(const QString& filename, lastFiles.split(";")) {
            if (!filename.isEmpty() && QFileInfo::exists(filename))
                fileList << filename;
        }
    }

    bool files_loaded = false;
    for (QStringList::Iterator it = fileList.begin(); it != fileList.end(); ++it )
    {
        if (show_splash)
        {
            splash->showMessage(QObject::tr("Loading File %1..")
                    .arg(QDir::toNativeSeparators(*it)),
            Qt::AlignRight|Qt::AlignBottom, Qt::black);
            qApp->processEvents();
        }
        appWin.slotFileOpen(*it);
        files_loaded = true;
    }

    if (reopenLastFiles){
        appWin.activateWindowWithFile(activeFile);
    }
    RS_DEBUG->print("main: loading files: OK");

    if (!files_loaded)
    {
        appWin.slotFileNewNew();
    }

    if (show_splash)
        splash->finish(&appWin);
    else
        delete splash;

    if (first_load)
        settings.setValue("Startup/FirstLoad", 0);

    RS_DEBUG->print("main: entering Qt event loop");

    int return_code = app.exec();

    RS_DEBUG->print("main: exited Qt event loop");

    // Destroy the singleton
    QC_ApplicationWindow::getAppWindow().reset();

    return return_code;
}


/**
 * Handles command line arguments that might not require a GUI.
 *
 * @return list of files to load on startup.
 */
QStringList handleArgs(int argc, char** argv, const QList<int>& argClean)
{
    RS_DEBUG->print("main: handling args..");
    QStringList ret;

    bool doexit = false;

    for (int i=1; i<argc; i++)
    {
        if(argClean.indexOf(i)>=0) continue;
        if (!QString(argv[i]).startsWith("-"))
        {
            QString fname = QDir::toNativeSeparators(
            QFileInfo(QFile::decodeName(argv[i])).absoluteFilePath());
            ret.append(fname);
        }
        else if (QString(argv[i])=="--exit")
        {
            doexit = true;
        }
    }
    if (doexit)
    {
        exit(0);
    }
    RS_DEBUG->print("main: handling args: OK");
    return ret;
}

namespace {
void restoreWindowGeometry(QC_ApplicationWindow& appWin, QSettings& settings)
{
    settings.beginGroup("Geometry");
    auto geometryB64 = settings.value("/WindowGeometry").toString().toUtf8();
    auto geometry = QByteArray::fromBase64(geometryB64, QByteArray::Base64Encoding);
    if (!geometry.isEmpty()) {
        appWin.restoreGeometry(geometry);
    } else {
        // fallback
        int windowWidth = settings.value("WindowWidth", 1024).toInt();
        int windowHeight = settings.value("WindowHeight", 1024).toInt();
        int windowX = settings.value("WindowX", 32).toInt();
        int windowY = settings.value("WindowY", 32).toInt();
        appWin.resize(windowWidth, windowHeight);
        appWin.move(windowX, windowY);
    }

    settings.endGroup();
}
}
