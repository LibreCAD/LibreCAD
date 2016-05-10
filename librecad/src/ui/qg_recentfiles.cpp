/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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
#include <QFileInfo>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include "qg_recentfiles.h"

#include "rs_debug.h"
#include "rs_settings.h"

/**
 * Constructor
 * @param number Number of files that can be stored in the list at maximum
 */
QG_RecentFiles::QG_RecentFiles(QObject* parent, int number)
    : QObject(parent)
    , number(number)
{}

QG_RecentFiles::~QG_RecentFiles()
{
	RS_SETTINGS->beginGroup("/RecentFiles");
	for (int i=0; i<count(); ++i) {
		RS_SETTINGS->writeEntry(QString("/File") + QString::number(i+1), get(i));
	}
	RS_SETTINGS->endGroup();
}

/**
 * Adds a file to the list of recently loaded files if
 * it's not already in the list.
 */
void QG_RecentFiles::add(const QString& filename) {
    RS_DEBUG->print("QG_RecentFiles::add");
	if(filename.size()>2048){
		RS_DEBUG->print(RS_Debug::D_ERROR, "QG_RecentFiles::add filename too long at %d\n", filename.size());
		return;
	}

    // is the file already in the list?
    int i0=files.indexOf(filename);
    if (i0>=0) {
		if (i0+1==files.size()) return; //do nothing, file already being the last in list
        //move the i0 to the last
		files.erase(files.begin() + i0);
		files.push_back(filename);
        return;
    }

    // append
    //files.push_back(filename);
    files.append(filename);
	if(files.size() > number)
		files.erase(files.begin(), files.begin() + files.size() - number);
	RS_DEBUG->print("QG_RecentFiles::add: OK");
}


QString QG_RecentFiles::get(int i) const{
	if (i<files.size()) {
		return files[i];
	} else {
		return QString("");
	}
}

int QG_RecentFiles::count() const {
	return files.count();
}

/** @return number of files that can be stored in the list at maximum */
int QG_RecentFiles::getNumber() const {
	return number;
}

int QG_RecentFiles::indexOf(const QString& filename) const{
	return files.indexOf(filename) ;
}

void QG_RecentFiles::addFiles(QMenu* file_menu)
{
    RS_DEBUG->print("QG_RecentFiles::addFiles()");

    RS_SETTINGS->beginGroup("/RecentFiles");
    for (int i=0; i<number; ++i)
    {
        QString filename = RS_SETTINGS->readEntry(QString("/File") +
                           QString::number(i+1));
        if (QFileInfo(filename).exists()) add(filename);
    }
    RS_SETTINGS->endGroup();

    QActionGroup* a_group = new QActionGroup(this);
    connect(a_group, SIGNAL(triggered(QAction*)),
            parent(), SLOT(slotFileOpenRecent(QAction*)));

    for (int i = 0; i < number; ++i)
    {
        recentFilesAction.push_back(new QAction(a_group));
        QAction* a=recentFilesAction.back();
        a->setVisible(false);
        file_menu->addAction(a);
    }
    if (count()>0) {
        updateRecentFilesMenu();
    }
}


void QG_RecentFiles::updateRecentFilesMenu() {
	RS_DEBUG->print("QG_RecentFiles::updateRecentFilesMenu(): begin\n");

	RS_DEBUG->print("Updating recent file menu...");
	int numRecentFiles = std::min(count(), getNumber());

	for (int i = 0; i < numRecentFiles; ++i) {
		//oldest on top
//        QString text = tr("&%1 %2").arg(i + 1).arg(recentFiles->get(i));
		//newest on top

        auto file_path = get(numRecentFiles-i-1);
        if (file_path.length() > 128)
            file_path = "..." + file_path.right(128);
        QString const& text = tr("&%1 %2").arg(i + 1).arg(file_path);

		recentFilesAction[i]->setText(text);
		//newest on top
		recentFilesAction[i]->setData(get(numRecentFiles-i-1));
		recentFilesAction[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < getNumber(); ++j)
		recentFilesAction[j]->setVisible(false);
	RS_DEBUG->print("QG_RecentFiles::updateRecentFilesMenu(): ok\n");
}


