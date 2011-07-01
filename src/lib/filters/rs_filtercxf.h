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


#ifndef RS_FILTERCXF_H
#define RS_FILTERCXF_H

#include <fstream>

#include "rs_filterinterface.h"

/**
 * This format filter class can import and export CXF (CAM Expert Font) files.
 *
 * @author Andrew Mustun
 */
class RS_FilterCXF : public RS_FilterInterface {
public:
    RS_FilterCXF();
    ~RS_FilterCXF() {}
	
	/**
	 * @return RS2::FormatCXF.
	 */
	//RS2::FormatType rtti() {
	//	return RS2::FormatCXF;
	//}
	
    /*virtual bool canImport(RS2::FormatType t) {
		return (t==RS2::FormatCXF);
	}
	
    virtual bool canExport(RS2::FormatType t) {
		return (t==RS2::FormatCXF);
	}*/

    virtual bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    virtual bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    void stream(std::ofstream& fs, double value);
};

#endif
