// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

#include "qg_recentfiles.h"

#include <QActionGroup>
#include <QFileInfo>
#include <QMenu>

#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_settings.h"

/**
* Constructor
* @param number Number of files that can be stored in the list at maximum
*/
QG_RecentFiles::QG_RecentFiles(QObject* parent, int number)
    : QObject(parent)
      , m_maxEntries(number) {
    if (number <= 0)
        assert(!"maximum number of RecentFiles must be larger than zero");
}

QG_RecentFiles::~QG_RecentFiles() {
    try {
        saveToSettings();
    }
    catch (...) {
        RS_DEBUG->print(
            "QG_RecentFiles::~QG_RecentFiles(): saving to settings caused an exception.");
    }
}

void QG_RecentFiles::saveToSettings() const {
    RS_SETTINGS->beginGroup("/RecentFiles");
    for (int i = 0; i < count(); ++i) {
        RS_SETTINGS->writeEntry(QString("/File") + QString::number(i + 1), get(i));
    }
    RS_SETTINGS->endGroup();
}

/**
 * Adds a file to the list of recently loaded m_files if
 * it's not already in the list.
 */
void QG_RecentFiles::add(const QString& filename) {
    RS_DEBUG->print("QG_RecentFiles::add");
    if (filename.size() > 2048) {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_RecentFiles::add filename too long at %zu\n",
                        filename.size());
        return;
    }

    // Already on top
    if (!m_files.empty() && filename == m_files.back())
        return;

    // is the file already in the list? remove it, as it will be added to the top
    m_files.removeAll(filename);

    // prepend
    m_files.append(filename);
    while (m_files.size() > m_maxEntries)
        m_files.pop_front();
    if (hasMenuEntries())
        updateRecentFilesMenu();
    RS_DEBUG->print("QG_RecentFiles::add: OK");
}

void QG_RecentFiles::addIfAbsent(const QString& filename) {
    if (!contains(filename)) {
        add(filename);
    }
}

QString QG_RecentFiles::get(int i) const {
    if (i >= 0 && i < m_files.size()) {
        return m_files[i];
    }
    else {
        return QString("");
    }
}

int QG_RecentFiles::count() const {
    return m_files.count();
}

/** @return m_maxEntries of files that can be stored in the list at maximum */
int QG_RecentFiles::getNumber() const{
    return m_maxEntries;
}

int QG_RecentFiles::indexOf(const QString &filename) const{
    return m_files.indexOf(filename);
}

void QG_RecentFiles::addFiles(QMenu *file_menu){
    RS_DEBUG->print("QG_RecentFiles::addFiles()");

    LC_GROUP("RecentFiles");
    for (int i = 0; i < m_maxEntries; ++i) {
        QString filename = LC_GET_STR(QString("File") + QString::number(i + 1));
        if (QFileInfo::exists(filename))
            add(filename);
    }
    LC_GROUP_END();

    auto *a_group = new QActionGroup(this);
    QC_ApplicationWindow *context = static_cast<QC_ApplicationWindow *>(parent());
    connect(a_group, &QActionGroup::triggered, context, &QC_ApplicationWindow::slotFileOpenRecent);

    for (int i = 0; i < m_maxEntries; ++i) {
        m_recentFilesActions.push_back(new QAction(a_group));
        QAction *a = m_recentFilesActions.back();
        a->setVisible(false);
        file_menu->addAction(a);
    }
    if (count() > 0) {
        updateRecentFilesMenu();
    }
    RS_DEBUG->print("QG_RecentFiles::addFiles(): OK");
}

void QG_RecentFiles::updateRecentFilesMenu(){
    RS_DEBUG->print("QG_RecentFiles::updateRecentFilesMenu(): begin\n");

    RS_DEBUG->print("Updating recent file menu...");

    QStringList validateFiles;
    std::copy_if(m_files.cbegin(),
                 m_files.cend(),
                 std::back_inserter(validateFiles),
                 [](const QString &file) { return QFileInfo::exists(file); });
    if (validateFiles.size() < m_files.size())
        m_files = validateFiles;

    while (m_files.size() > m_recentFilesActions.size()){
        m_files.pop_front();
    }

    foreach (auto *action, m_recentFilesActions){
            action->setVisible(false);
    }
    auto itAction = m_recentFilesActions.begin();
    // most recent file in the back of files
    for (auto itFile = m_files.rbegin(); itFile != m_files.rend(); itFile++) {
        //oldest on top
        //        QString text = tr("&%1 %2").arg(i + 1).arg(recentFiles->get(i));
        //newest on top

        auto file_path = *itFile;
        if (file_path.length() > 128)
            file_path = "..." + file_path.right(128);
        const QString text = tr("&%1 %2").arg(itFile - m_files.rbegin() + 1).arg(file_path);

        auto *action = *itAction++;
        action->setText(text);
        //newest on top
        action->setData(*itFile);
        action->setVisible(true);
    }
    saveToSettings();
    RS_DEBUG->print("QG_RecentFiles::updateRecentFilesMenu(): OK\n");
}
