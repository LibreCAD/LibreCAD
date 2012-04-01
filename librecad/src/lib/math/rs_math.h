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

#include <cmath>
#include <complex>
#include <errno.h>

// RVT port abs issue on latest compiler?
#include <cstdlib>

#include <QRegExp>
#include <QVector>
#include <muParser.h>

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
    static RS_Vector pow(RS_Vector x, double y);

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
//swap of two variables
    template <class T>
    static void swap( T &a, T &b) {
        const T ttmp(a);
        a=b;
        b=ttmp;
    }

    static double eval(const QString& expr, bool* ok);

    static std::vector<double> quadraticSolver(const std::vector<double>& ce);
    static std::vector<double> cubicSolver(const std::vector<double>& ce);
    static std::vector<double> quarticSolver(const std::vector<double>& ce);
    //solver for linear equation set
    static bool linearSolver(const QVector<QVector<double> >& m, QVector<double>& dn);

    /** solver quadratic simultaneous equations of a set of two **/
    /* solve the following quadratic simultaneous equations,
      *  ma000 x^2 + ma011 y^2 - 1 =0
      * ma100 x^2 + 2 ma101 xy + ma111 y^2 + mb10 x + mb11 y +mc1 =0
      *
      *@m, a vector of size 8 contains coefficients in the strict order of:
      ma000 ma011 ma100 ma101 ma111 mb10 mb11 mc1
      *@return a RS_VectorSolutions contains real roots (x,y)
      */
    static RS_VectorSolutions simultaneusQuadraticSolver(const std::vector<double>& m);

    /** wrapper for elliptic integral **/
    /**
     * wrapper of elliptic integral of the second type, Legendre form
     *@k the elliptic modulus or eccentricity
     *@phi elliptic angle, must be within range of [0, M_PI]
     *
     *Author: Dongxu Li
     */
    static double ellipticIntegral_2(const double& k, const double& phi);

    static QString doubleToString(double value, double prec);
    static QString doubleToString(double value, int prec);

    static void test();
    };

#endif
