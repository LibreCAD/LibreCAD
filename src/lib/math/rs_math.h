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

#ifndef RS_MATH_H
#define RS_MATH_H

// no idea why, but doesn't link without that under win32 / bcc55:
#ifndef _MT
#define _MT
#endif

#include <math.h>
#include <errno.h>

// RVT port abs issue on latest compiler?
#include <cstdlib>

#include <QRegExp>
#include "fparser.hh"

#include "rs.h"
#include "rs_vector.h"

//#ifdef __GNUC__
//#define min(x,y) (x<y ? x : y)
//#define max(x,y) (x>y ? x : y)
//#endif

#define ARAD 57.29577951308232
#define RS_TOLERANCE 1.0e-10
#define RS_TOLERANCE_ANGLE 1.0e-8

typedef unsigned int uint;

/**
 * Math functions.
 */
class RS_Math {
public:
    static int round(double v);
	static double pow(double x, double y);

    //static double abs(double v);
    //static int abs(int v);
    static double rad2deg(double a);
    static double deg2rad(double a);
    static double rad2gra(double a);
    static int findGCD(int a, int b);
    static bool isAngleBetween(double a,
                               double a1, double a2,
                               bool reversed);
    static double correctAngle(double a);
    static double getAngleDifference(double a1, double a2);
    static double makeAngleReadable(double angle, bool readable=true,
                                    bool* corrected=NULL);
    static bool isAngleReadable(double angle);
    static bool isSameDirection(double dir1, double dir2, double tol);
    static double eval(const QString& expr, double def=0.0);

    static bool cmpDouble(double v1, double v2, double tol=0.001);
  	
    /**
     * Evaluates a mathematical expression and returns the result.
     * If an error occured, ok will be set to false (if ok isn't NULL).
     */
	// Keep that in the header file for dynamic inclusion/exclusion.
    static double eval(const QString& expr, bool* ok) {
        if (expr.isEmpty()) {
            if (ok!=NULL) {
                *ok = false;
            }
            return 0.0;
        }

        FunctionParser fp;
        fp.AddConstant("pi", M_PI);

		// replace '14 3/4' with '14+3/4'
                QString s = expr;
		bool done;
		do {
			done = true;
                        int i = s.indexOf(QRegExp("[0-9]* [0-9]*/[0-9]*"));
			if (i!=-1) {
                                int i2 = s.indexOf(' ', i);
				if (i2!=-1) {
					s.replace(i2, 1, "+");
					done = false;
				}
			}
		} while (!done);

        int ret = fp.Parse(s.toLatin1().data(), "", true);

        if (ret>=0) {
            if (ok!=NULL) {
                *ok = false;
            }
            return 0.0;
        }

        if (ok!=NULL) {
            *ok = true;
        }

        return fp.Eval(NULL);
    }

    static QString doubleToString(double value, double prec);
    static QString doubleToString(double value, int prec);

    static void test();
};

#endif
