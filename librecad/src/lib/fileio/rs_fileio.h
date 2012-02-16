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

#ifndef RS_FILEIO_H
#define RS_FILEIO_H

#include <QList>
#include "rs_filterinterface.h"

#define RS_FILEIO RS_FileIO::instance()

//RLZ: TODO destructor for clear filterList
/**
 * API Class for importing files. 
 *
 * @author Andrew Mustun
 */
class RS_FileIO {
private:
    RS_FileIO() {}
    RS_FileIO(RS_FileIO&) = delete;
	
public:
    /**
     * @return Instance to the unique import object.
     */
    static RS_FileIO* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_FileIO();
        }
        return uniqueInstance;
    }
	
    /**
     * Registers a new import filter.
     */
	void registerFilter(RS_FilterInterface* f) {
		filterList.append(f);
	}

    /**
     * @return List of registered filters.
     */
        QList<RS_FilterInterface*> getFilterList() {
		return filterList;
	}

	/**
	 * @return Filter which can import the given file type.
	 */
	RS_FilterInterface* getImportFilter(RS2::FormatType t) {
                for (int i = 0; i < filterList.size(); ++i) {
                    if (filterList.at(i)->canImport(t)) {
                                return filterList.at(i);
                        }
                }

		return NULL;
	}
	
	/**
	 * @return Filter which can export the given file type.
	 */
	RS_FilterInterface* getExportFilter(RS2::FormatType t) {
                for (int i = 0; i < filterList.size(); ++i) {
                    if (filterList.at(i)->canExport(t)) {
                                return filterList.at(i);
                        }
                }

		return NULL;
	}

    bool fileImport(RS_Graphic& graphic, const QString& file,
		RS2::FormatType type = RS2::FormatUnknown);
		
    bool fileExport(RS_Graphic& graphic, const QString& file,
		RS2::FormatType type = RS2::FormatUnknown);

        RS2::FormatType detectFormat(const QString& file);

protected:
    static RS_FileIO* uniqueInstance;
    QList<RS_FilterInterface*> filterList;
}
;

#endif
