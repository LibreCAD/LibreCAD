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


#ifndef RS_FILTERINTERFACE_H
#define RS_FILTERINTERFACE_H

#include "rs_graphic.h"

#include "rs_string.h"
#include "rs_valuelist.h"

#include "rs_debug.h"

/**
 * This is the interface that must be implemented for all 
 * format filter classes. The RS_FileIO class 
 * uses the methods defined in here to interact with the format
 * filter classes.
 *
 * @author Andrew Mustun
 */
class RS_FilterInterface {
public:
    /**
     * Constructor.
     */
    RS_FilterInterface() {
		//std::cout << "RS_FilterInterface\n";
        //graphic = NULL;
    }

    /**
     * Destructor.
     */
    virtual ~RS_FilterInterface() {}

    /**
     * Checks if this filter can import the given file type.
     *
     * @retval true if the filter can import the file type 
     * @retval false otherwise.
     */
    virtual bool canImport(RS2::FormatType t) {
        return !(importFormats.find(t)==importFormats.end());
    }

    /**
     * Checks if this filter can export the given file type.
     *
     * @return true if the filter can export the file type, 
     *         false otherwise.
     */
    virtual bool canExport(RS2::FormatType t) {
        return !(exportFormats.find(t)==exportFormats.end());
    }

    /**
     * The implementation of this method in a inherited format
     * class should read a file from disk and put the entities
     * into the current entity container.
     */
    virtual bool fileImport(RS_Graphic& g, const RS_String& file, RS2::FormatType type) = 0;

    /**
     * The implementation of this method in a inherited format
     * class should write the entities in the current entity container
     * to a file on the disk.
     */
    virtual bool fileExport(RS_Graphic& g, const RS_String& file, RS2::FormatType type) = 0;

protected:
    /**
     * Adds a file extension which can be imported by this filter.
     */
    void addImportFormat(RS2::FormatType type) {
        RS_DEBUG->print("Filter can import %d", (int)type);
        importFormats += type;
    }

    /**
     * Adds a file extension which can be exported by this filter.
     */
    void addExportFormat(RS2::FormatType type) {
        RS_DEBUG->print("Filter can export %d", (int)type);
        exportFormats += type;
    }

protected:
    //! Pointer to the graphic we currently operate on.
    //RS_Graphic* graphic;

    //! Vector of file extensions this filter can import.
    RS_ValueList<RS2::FormatType> importFormats;

    //! Vector of file extensions this filter can export.
    RS_ValueList<RS2::FormatType> exportFormats;
};

#endif
