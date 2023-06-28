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

#include <vector>

class RS_Vector;
class RS_VectorSolutions;
class QString;
class QRegularExpressionMatch;

/**
 * Math functions.
 */
namespace RS_Math {
int round(double v);
double round(const double v, const double precision);
double pow(double x, double y);
RS_Vector pow(const RS_Vector &x, double y);
bool equal(const double d1, const double d2);

double rad2deg(double a);
double deg2rad(double a);
double rad2gra(double a);
double gra2rad(double a);
unsigned findGCD(unsigned a, unsigned b);
bool isAngleBetween(double a, double a1, double a2, bool reversed = false);
//! \brief correct angle to be within [0, +PI*2.0)
double correctAngle(double a);
//! \brief correct angle to be within [-PI, +PI)
double correctAngle2(double a);
//! \brief correct angle to be unsigned [0, +PI)
double correctAngleU(double a);

//! \brief angular difference
double getAngleDifference(double a1, double a2, bool reversed = false);
/**
	 * @brief getAngleDifferenceU abs of minimum angular difference, unsigned version of angular difference
	 * @param a1,a2 angles
	 * @return the minimum of angular difference a1-a2 and a2-a1
     */
double getAngleDifferenceU(double a1, double a2);
double makeAngleReadable(double angle, bool readable = true, bool *corrected = nullptr);
bool isAngleReadable(double angle);
bool isSameDirection(double dir1, double dir2, double tol);

//! \convert measurement strings with rationals or unit symbols to current unit
double convert_unit(const QRegularExpressionMatch &, const QString &, double, double);
QString derationalize(const QString &expr);

//! \{ \brief evaluate a math string
double eval(const QString &expr, double def = 0.0);
double eval(const QString &expr, bool *ok);
//! \}

std::vector<double> quadraticSolver(const std::vector<double> &ce);
std::vector<double> cubicSolver(const std::vector<double> &ce);
/** quartic solver
    * x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
    @ce, a vector of size 4 contains the coefficient in order
    @return, a vector contains real roots
    **/
std::vector<double> quarticSolver(const std::vector<double> &ce);
/** quartic solver
* ce[4] x^4 + ce[3] x^3 + ce[2] x^2 + ce[1] x + ce[0] = 0
    @ce, a vector of size 5 contains the coefficient in order
    @return, a vector contains real roots
    **/
std::vector<double> quarticSolverFull(const std::vector<double> &ce);
//solver for linear equation set
/**
      * Solve linear equation set
	  *@param mt holds the augmented matrix
	  *@param sn holds the solution
	  *@param return true, if the equation set has a unique solution, return false otherwise
      *
	  *@author: Dongxu Li
      */
bool linearSolver(const std::vector<std::vector<double> > &m, std::vector<double> &sn);

/** solver quadratic simultaneous equations of a set of two **/
/* solve the following quadratic simultaneous equations,
      *  ma000 x^2 + ma011 y^2 - 1 =0
      * ma100 x^2 + 2 ma101 xy + ma111 y^2 + mb10 x + mb11 y +mc1 =0
      *
      *@m, a vector of size 8 contains coefficients in the strict order of:
      ma000 ma011 ma100 ma101 ma111 mb10 mb11 mc1
      *@return a RS_VectorSolutions contains real roots (x,y)
      */
RS_VectorSolutions simultaneousQuadraticSolver(const std::vector<double> &m);

/** solver quadratic simultaneous equations of a set of two **/
/** solve the following quadratic simultaneous equations,
      * ma000 x^2 + ma001 xy + ma011 y^2 + mb00 x + mb01 y + mc0 =0
      * ma100 x^2 + ma101 xy + ma111 y^2 + mb10 x + mb11 y + mc1 =0
      *
  *@param m a vector of size 2 each contains a vector of size 6 coefficients in the strict order of:
  ma000 ma001 ma011 mb00 mb01 mc0
  ma100 ma101 ma111 mb10 mb11 mc1
      *@return a RS_VectorSolutions contains real roots (x,y)
      */
RS_VectorSolutions simultaneousQuadraticSolverFull(const std::vector<std::vector<double> > &m);
RS_VectorSolutions simultaneousQuadraticSolverMixed(const std::vector<std::vector<double> > &m);

/** \brief verify simultaneousQuadraticVerify a solution for simultaneousQuadratic
	  *@param m the coefficient matrix
	  *@param v a candidate to verify
      *@return true, for a valid solution
      **/
bool simultaneousQuadraticVerify(const std::vector<std::vector<double> > &m, RS_Vector &v);
/** wrapper for elliptic integral **/
/**
     * wrapper of elliptic integral of the second type, Legendre form
     *@k the elliptic modulus or eccentricity
     *@phi elliptic angle, must be within range of [0, M_PI]
     *
	 *@\author: Dongxu Li
     */
double ellipticIntegral_2(const double &k, const double &phi);

QString doubleToString(double value, double prec);
QString doubleToString(double value, int prec);

void test();
}; // namespace

#endif
