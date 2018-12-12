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
    virtual bool canImport(const QString& /*fileName*/, RS2::FormatType t) const {
		return (t==RS2::FormatCXF);
	}
	
    virtual bool canExport(const QString& /*fileName*/, RS2::FormatType t) const {
		return (t==RS2::FormatCXF);
    }

    virtual bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    virtual bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    void stream(std::ofstream& fs, double value);

    static RS_FilterInterface* createFilter(){return new RS_FilterCXF();}
};

#endif
