/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

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

void LC_LastOpenFilesOpener::collectFilesList(const QList<QC_MDIWindow*>& windowList, const QMdiSubWindow* activWindow) {
    const bool rememberOpenedFiles = LC_GET_ONE_BOOL("Startup", "OpenLastOpenedFiles", false);
    m_activeFile.clear();
    m_openedFiles.clear();
    if (rememberOpenedFiles) {
        for (const auto w: windowList) {
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

void LC_LastOpenFilesOpener::saveSettings() const {
    LC_GROUP_GUARD("Startup"); {
        const bool rememberOpenedFiles = LC_GET_BOOL("OpenLastOpenedFiles", false);
        if (rememberOpenedFiles) {
            LC_SET("LastOpenFilesList", m_openedFiles);
            LC_SET("LastOpenFilesActive", m_activeFile);
        }
    }
}


void LC_LastOpenFilesOpener::openLastOpenFiles(QStringList &fileList,  QSplashScreen* splash) const {
    LC_GROUP("Startup"); // fixme - sand - files - move saved files opening to the appwindow or util class out of there
    {
        bool files_loaded = false;
        const QString lastFiles = LC_GET_STR("LastOpenFilesList", "");
        const bool reopenLastFiles = LC_GET_BOOL("OpenLastOpenedFiles");
        if (reopenLastFiles) {
            foreach(const QString &filename, lastFiles.split(";")) {
                if (!filename.isEmpty() && QFileInfo::exists(filename)) {
                    fileList << filename;
                }
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
                const QString activeFile = LC_GET_STR("LastOpenFilesActive", "");
                m_appWindow->activateWindowWithFile(activeFile);
            }
        }

        RS_DEBUG->print("main: loading files: OK");

        if (!files_loaded) {
            m_appWindow->slotFileNewFromDefaultTemplate();
        }
    }
}
