/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2012 R. van Twisk (librecad@rvt.dds.nl)
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

typedef  RS_FilterInterface* (*createFilter)();

//RLZ: TODO destructor for clear filterList
/**
 * API Class for importing files. 
 *
 * @author Andrew Mustun
 */
class RS_FileIO {
private:
    //singleton
    RS_FileIO();
    RS_FileIO(RS_FileIO&) = delete;
    RS_FileIO& operator = (RS_FileIO&) = delete;
	
public:
    /**
     * @return Instance to the unique import object.
     */
    static RS_FileIO* instance() {
         static RS_FileIO* uniqueInstance=NULL;
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_FileIO();
        }
        return uniqueInstance;
    }

    /**
     * Registers a new import filter.
     */
    void registerFilter(createFilter f) {
        filters.append(f);
    }

	/**
	 * @return Filter which can import the given file type.
	 */
	RS_FilterInterface* getImportFilter(const QString &fileName, RS2::FormatType t) {
        for (int i = 0; i < filters.size(); ++i) {
            RS_FilterInterface *filter=(* (filters.at(i)))();
            if (filter!=NULL) {
                if (filter->canImport(fileName, t))
                    return filter;
                delete filter;
            }
        }
		return NULL;
	}
	
	/**
	 * @return Filter which can export the given file type.
	 */
	RS_FilterInterface* getExportFilter(const QString &fileName, RS2::FormatType t) {
        for (int i = 0; i < filters.size(); ++i) {
            RS_FilterInterface *filter=(* (filters.at(i)))();
            if (filter!=NULL) {
                if (filter->canExport(fileName, t))
                    return filter;
                delete filter;
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

/** a list of pointers to static functions to create file filters **/
    QList<createFilter> filters;
};


#endif
