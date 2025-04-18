/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_LASTOPENFILESOPENER_H
#define LC_LASTOPENFILESOPENER_H
#include <QList>

class QSplashScreen;
class QC_ApplicationWindow;
class QMdiSubWindow;
class QC_MDIWindow;

class LC_LastOpenFilesOpener{
public:
    LC_LastOpenFilesOpener(QC_ApplicationWindow* m_appWin);
    ~LC_LastOpenFilesOpener();
    void openLastOpenFiles(QStringList& fileList,  QSplashScreen* spash);
    void collectFilesList(const QList<QC_MDIWindow*> &m_windowList,  const QMdiSubWindow* activWindow);
    void saveSettings();
private:
    QString m_openedFiles;
    QString m_activeFile = "";
    QC_ApplicationWindow* m_appWindow;
};


#endif // LC_LASTOPENFILESOPENER_H
