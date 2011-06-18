/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_entitycontainer.h"
#include "rs_filterinterface.h"
#include "rs_ptrlist.h"

#define RS_FILEIO RS_FileIO::instance()

/**
 * API Class for importing files. 
 *
 * @author Andrew Mustun
 */
class RS_FileIO {
protected:
    RS_FileIO() {}
	
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
	RS_PtrList<RS_FilterInterface> getFilterList() {
		return filterList;
	}

	/**
	 * @return Filter which can import the given file type.
	 */
	RS_FilterInterface* getImportFilter(RS2::FormatType t) {
		for (RS_FilterInterface* f=filterList.first(); 
			f!=NULL; f=filterList.next()) {
		
			if (f->canImport(t)) {
				return f;
			}
		}

		return NULL;
	}
	
	/**
	 * @return Filter which can export the given file type.
	 */
	RS_FilterInterface* getExportFilter(RS2::FormatType t) {
		for (RS_FilterInterface* f=filterList.first(); 
			f!=NULL; f=filterList.next()) {
		
			if (f->canExport(t)) {
				return f;
			}
		}

		return NULL;
	}

    bool fileImport(RS_Graphic& graphic, const RS_String& file, 
		RS2::FormatType type = RS2::FormatUnknown);
		
    bool fileExport(RS_Graphic& graphic, const RS_String& file,
		RS2::FormatType type = RS2::FormatUnknown);

	RS2::FormatType detectFormat(const RS_String& file);

protected:
    static RS_FileIO* uniqueInstance;
    RS_PtrList<RS_FilterInterface> filterList;
}
;

#endif
