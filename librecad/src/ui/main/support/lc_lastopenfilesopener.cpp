#include "lc_lastopenfilesopener.h"

#include <QDir>
#include <QFileInfo>
#include <QSplashScreen>
#include <qcoreapplication.h>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "rs_debug.h"
#include "rs_settings.h"

LC_LastOpenFilesOpener::LC_LastOpenFilesOpener(QC_ApplicationWindow* appWin):m_appWindow{appWin} {
}

LC_LastOpenFilesOpener::~LC_LastOpenFilesOpener() = default;

void LC_LastOpenFilesOpener::collectFilesList(const QList<QC_MDIWindow*>& m_windowList, const QMdiSubWindow* activWindow) {
    bool rememberOpenedFiles = LC_GET_ONE_BOOL("Startup", "OpenLastOpenedFiles", false);
    m_activeFile = "";
    m_openedFiles = "";
    if (rememberOpenedFiles) {
        for (auto w: m_windowList) {
            QString fileName = w->getFileName();
            if (!fileName.isEmpty()) {
                if (activWindow != nullptr && activWindow == w) {
                    m_activeFile = fileName;
                }
                m_openedFiles += fileName;
                m_openedFiles += ";";
            }
        }
    }
}

void LC_LastOpenFilesOpener::saveSettings() {
    LC_GROUP_GUARD("Startup"); {
        bool rememberOpenedFiles = LC_GET_BOOL("OpenLastOpenedFiles", false);
        if (rememberOpenedFiles) {
            LC_SET("LastOpenFilesList", m_openedFiles);
            LC_SET("LastOpenFilesActive", m_activeFile);
        }
    }
}


void LC_LastOpenFilesOpener::openLastOpenFiles(QStringList &fileList,  QSplashScreen* splash) {
    bool files_loaded = false;
    LC_GROUP("Startup"); // fixme - sand - files - move saved files opening to the appwindow or util class out of there
    {
        QString lastFiles = LC_GET_STR("LastOpenFilesList", "");
        bool reopenLastFiles = LC_GET_BOOL("OpenLastOpenedFiles");
        if (reopenLastFiles) {
            foreach(const QString &filename, lastFiles.split(";")) {
                if (!filename.isEmpty() && QFileInfo::exists(filename))
                    fileList << filename;
            }
        }
        if (!fileList.isEmpty()) {
            for (auto & it : fileList) {
                if (splash != nullptr) {
                    auto message = QObject::tr("Loading File %1..").arg(QDir::toNativeSeparators(it));
                    splash->showMessage(message,Qt::AlignRight | Qt::AlignBottom, Qt::black);
                    qApp->processEvents();
                }
                m_appWindow->openFile(it);
                files_loaded = true;
            }
            if (reopenLastFiles) {
                QString activeFile = LC_GET_STR("LastOpenFilesActive", "");
                m_appWindow->activateWindowWithFile(activeFile);
            }
        }

        RS_DEBUG->print("main: loading files: OK");

        if (!files_loaded) {
            m_appWindow->slotFileNewFromDefaultTemplate();
        }
    }
}
