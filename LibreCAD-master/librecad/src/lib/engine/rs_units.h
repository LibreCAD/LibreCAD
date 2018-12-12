/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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


#ifndef RS_UNITS_H
#define RS_UNITS_H

#include "rs.h"

class RS_Vector;
class QString;

/**
 * Conversion methods for units
 *
 * @author Andrew Mustun
 */
class RS_Units {
public:

    //static char* unit2sign(RS2::Unit unit);

    //static RS2::Unit string2unit(const char* str, bool* ok=0);
    //static char* unit2string(RS2::Unit unit);

    static RS2::Unit dxfint2unit(int dxfint);

    static QString unitToString(RS2::Unit u, bool t = true);
    static RS2::Unit stringToUnit(const QString& u);

	static bool isMetric(RS2::Unit u);
	static double getFactorToMM(RS2::Unit u);
	static double convert(double val, RS2::Unit src, RS2::Unit dest);
	static RS_Vector convert(const RS_Vector& val, RS2::Unit src, RS2::Unit dest);
	
    static QString unitToSign(RS2::Unit u);

    static QString formatLinear(double length, RS2::Unit unit,
                                  RS2::LinearFormat format,
                                  int prec, bool showUnit=false);
    static QString formatScientific(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);
    static QString formatDecimal(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);
    static QString formatEngineering(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);
    static QString formatArchitectural(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);
    static QString formatFractional(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);
    static QString formatArchitecturalMetric(double length, RS2::Unit unit,
                                  int prec, bool showUnit=false);

    static QString formatAngle(double angle, RS2::AngleFormat format,
                                 int prec);
    static RS2::AngleFormat numberToAngleFormat(int num);

	static RS_Vector paperFormatToSize(RS2::PaperFormat p);
	static RS2::PaperFormat paperSizeToFormat(const RS_Vector& s);

	static QString paperFormatToString(RS2::PaperFormat p);
	static RS2::PaperFormat stringToPaperFormat(const QString& p);

	static void test();
    static double dpiToScale(double dpi, RS2::Unit unit);
    static double scaleToDpi(double scale, RS2::Unit unit);

};


#endif
