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

#include "qg_recentfiles.h"

#include "rs_debug.h"

/**
 * Constructor
 * @param number Number of files that can be stored in the list at maximum
 */
QG_RecentFiles::QG_RecentFiles(int number):
	number(number)
{
}

/**
 * Adds a file to the list of recently loaded files if
 * it's not already in the list.
 */
void QG_RecentFiles::add(const QString& filename) {
    RS_DEBUG->print("QG_RecentFiles::add");

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
