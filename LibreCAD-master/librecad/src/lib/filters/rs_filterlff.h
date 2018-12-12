/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
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


#ifndef RS_FILTERLFF_H
#define RS_FILTERLFF_H

#include <fstream>

#include "rs_filterinterface.h"

/**
 * This format filter class can import and export LFF (LibreCAD Font File) files.
 *
 * @author Rallaz
 */
class RS_FilterLFF : public RS_FilterInterface {
public:
    RS_FilterLFF();
    ~RS_FilterLFF() {}
	
/**
* @return RS2::FormatLFF.
*/
RS2::FormatType rtti() const{
        return RS2::FormatLFF;
}
	
    virtual bool canImport(const QString& /*fileName*/, RS2::FormatType t) const {
        return (t==RS2::FormatLFF);
    }
	
    virtual bool canExport(const QString& /*fileName*/, RS2::FormatType t) const {
        return (t==RS2::FormatLFF);
    }

    virtual bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    virtual bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    void stream(std::ofstream& fs, double value);

    static RS_FilterInterface *createFilter() {return new RS_FilterLFF();}
};



#endif
