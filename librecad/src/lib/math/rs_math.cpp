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
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions/ellint_2.hpp>

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <muParser.h>
#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

#include "rs.h"
#include "rs_math.h"
#include "rs_vector.h"
#include "rs_debug.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif


namespace {
    constexpr double m_piX2 = M_PI*2; //2*PI
    constexpr double m_halfPI = M_PI/2; //PI/2
    const QRegularExpression unitreg(
        R"((?P<sign>^-?))"
        R"((?:(?:(?:(?P<degrees>\d+\.?\d*)(?:degree[s]?|deg|[Dd]|°)))"  // DMS
            R"((?:(?P<minutes>\d+\.?\d*)(?:minute[s]?|min|[Mm]|'))?)"
            R"((?:(?P<seconds>\d+\.?\d*)(?:second[s]?|sec|[Ss]|"))?$)|)"
        R"((?:(?:(?P<meters>\d+\.?\d*)(?:meter[s]?|m(?![m])))?)"        // Metric
            R"((?:(?P<centis>\d+\.?\d*)(?:centimeter[s]?|centi|cm))?)"
            R"((?:(?P<millis>\d+\.?\d*)(?:millimeter[s]?|mm))?$)|)"
        R"((?:(?:(?P<yards>\d+\.?\d*)(?:yards|yard|yd))?)"              // Imperial
            R"((?:(?P<feet>\d+\.?\d*)(?:feet|foot|ft|'))?)"
            R"((?:(?P<inches>\d+\.?\d*)[-+]?)"
                R"((?:(?P<numer>\d+)\/(?P<denom>\d+))?)"                // rational inches
            R"((?:inches|inch|in|"))?$)))"
	);
}

/**
 * Rounds the given double to the closest int.
 */
int RS_Math::round(double v) {
    return (int) std::lrint(v);
}

double RS_Math::round(const double v, const double precision)
{
    return precision * std::llround(v / precision);
}

/**
 * Save pow function
 */
double RS_Math::pow(double x, double y) {
    errno = 0;
    double ret = std::pow(x, y);
    if (errno==EDOM) {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "RS_Math::pow: EDOM in pow");
        ret = 0.0;
    }
    else if (errno==ERANGE) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Math::pow: ERANGE in pow");
        ret = 0.0;
    }
    return ret;
}

/* pow of vector components */
RS_Vector RS_Math::pow(const RS_Vector& vp, double y) {
    return RS_Vector(std::pow(vp.x, y), std::pow(vp.y, y));
}

/**
 * Save equal function for real types
 */
bool RS_Math::equal(const double d1, const double d2)
{
    return std::abs(d1 - d2) < RS_TOLERANCE;
}

/**
 * Converts radians to degrees.
 */
double RS_Math::rad2deg(double a) {
    return 180. / M_PI * a;
}

/**
 * Converts degrees to radians.
 */
double RS_Math::deg2rad(double a) {
    return M_PI / 180.0 * a;
}

/**
 * Converts radians to gradians.
 */
double RS_Math::rad2gra(double a) {
    return 200. / M_PI * a;
}

double RS_Math::gra2rad(double a) {
    return M_PI / 200. * a;
}


/**
 * Finds greatest common divider using Euclid's algorithm.
 */
unsigned RS_Math::findGCD(unsigned a, unsigned b) {
    if (b > a)
        std::swap(a, b);

    while (b) {
        unsigned rem = a % b;
        a = b;
        b = rem;
    }

    return a;
}



/**
 * Tests if angle a is between a1 and a2. a, a1 and a2 must be in the
 * range between 0 and 2*PI.
 * All angles in rad.
 *
 * @param reversed true for clockwise testing. false for ccw testing.
 * @return true if the angle a is between a1 and a2.
 */
bool RS_Math::isAngleBetween(double a,
                             double a1, double a2,
                             bool reversed) {

	if (reversed) std::swap(a1,a2);
	if(getAngleDifferenceU(a2, a1 ) < RS_TOLERANCE_ANGLE) return true;
	const double tol=0.5*RS_TOLERANCE_ANGLE;
	const double diff0=correctAngle(a2 -a1) + tol;

	return diff0 >= correctAngle(a - a1) || diff0 >= correctAngle(a2 - a);
}

/**
 * Corrects the given angle to the range of 0 to +PI*2.0.
 */
double RS_Math::correctAngle(double a) {
    return std::fmod(M_PI + std::remainder(a - M_PI, m_piX2), m_piX2);
}

/**
 * Corrects the given angle to the range of -PI to +PI.
 */
double RS_Math::correctAngle2(double a) {
    return std::remainder(a, m_piX2);
}

/**
 * Returns the given angle as an Unsigned Angle in the range of 0 to +PI.
 */
double RS_Math::correctAngleU(double a) {
    return std::abs(std::remainder(a, m_piX2));
}

/**
 * Returns the given angle as an Unsigned Angle in the range of 0 to +PI.
 */
double RS_Math::correctAngle3(double a) {
    return std::remainder(a, m_halfPI);
}



/**
 * @return The angle that needs to be added to a1 to reach a2.
 *         Always positive and less than 2*pi.
 */
double RS_Math::getAngleDifference(double a1, double a2, bool reversed) {
	if(reversed) std::swap(a1, a2);
	return correctAngle(a2 - a1);
}

double RS_Math::getAngleDifferenceU(double a1, double a2)
{
	return correctAngleU(a1 - a2);
}


/**
* Makes a text constructed with the given angle readable. Used
* for dimension texts and for mirroring texts.
*
* @param readable true: make angle readable, false: unreadable
* @param corrected Will point to true if the given angle was
*   corrected, false otherwise.
*
 * @return The given angle or the given angle+PI, depending which on
 * is readable from the bottom or right.
 */
double RS_Math::makeAngleReadable(double angle, bool readable,
                                  bool* corrected) {

    double ret=correctAngle(angle);

    bool cor = isAngleReadable(ret) ^ readable;

    // quadrant 1 & 4
    if (cor) {
        //        ret = angle;
        //    }
        // quadrant 2 & 3
        //    else {
        ret = correctAngle(angle+M_PI);
    }

    if (corrected) {
        *corrected = cor;
    }

    return ret;
}


/**
 * @return true: if the given angle is in a range that is readable
 * for texts created with that angle.
 */
bool RS_Math::isAngleReadable(double angle) {
	const double tolerance=0.001;
    if (angle>M_PI_2)
        return std::abs(std::remainder(angle, m_piX2)) < (M_PI_2 - tolerance);
    else
        return std::abs(std::remainder(angle, m_piX2)) < (M_PI_2 + tolerance);
}

/**
 * @param tol Tolerance in rad.
 * @retval true The two angles point in the same direction.
 */
bool RS_Math::isSameDirection(double dir1, double dir2, double tol) {
	return getAngleDifferenceU(dir1, dir2) < tol;
}

/**
 * Evaluates a mathematical expression and returns the result.
 * If an error occurred, the given default value 'def' will be returned.
 */
double RS_Math::eval(const QString& expr, double def) {

    bool ok = false;
    double res = RS_Math::eval(expr, &ok);

    if (!ok) {
        //std::cerr << "RS_Math::evaluate: Parse error at col "
        //<< ret << ": " << fp.ErrorMsg() << "\n";
        return def;
    }

    return res;
}
/**
 * Helper function for derationalize; convert one unit to base unit using
 * provided regex match, named group, conversion factor, and default value.
 */
double RS_Math::convert_unit(const QRegularExpressionMatch& match, const QString& name, double factor, double defval) {
    if (!match.captured(name).isNull())
        LC_ERR <<"name="<<name<<": "<<  match.captured(name);
    QString input = (!match.captured(name).isNull()) ? match.captured(name) : QString::number(defval, 'g', 16);
    return input.toDouble() * factor;
}

/**
 * Convert a string containing rational numbers (fractions) and / or
 * various unit symbols into the current user unit (cm or inch).
 * Note: only the gui cares about units, so all matched symbols are used naively.
 */
QString RS_Math::derationalize(const QString& expr) {
	RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Math::derationalize: expr = '%s'", expr.toLatin1().data());

	QRegularExpressionMatch match = unitreg.match(expr);
	if (match.hasMatch()){
		RS_DEBUG->print(RS_Debug::D_DEBUGGING,
			"RS_Math::derationalize: matches = '%s'", match.capturedTexts().join(", ").toLatin1().data());
		double total = 0.0;
		int sign = (match.captured("sign").isNull() || match.captured("sign") == "") ? 1 : -1;

        // convert_unit(<match obj ref>, <regex group name>, <unit->base conversion factor>, <default value>)
        total += convert_unit(match, "degrees", 1.0, 0.0);
        total += convert_unit(match, "minutes", 1/60.0, 0.0);
        total += convert_unit(match, "seconds", 1/3600.0, 0.0);
        total += convert_unit(match, "meters", 100.0, 0.0);
        total += convert_unit(match, "centis", 1.0, 0.0);
        total += convert_unit(match, "millis", 0.1, 0.0);
        total += convert_unit(match, "yards", 36.0, 0.0);
        total += convert_unit(match, "feet", 12.0, 0.0);
        total += convert_unit(match, "inches", 1.0, 0.0);
        total += convert_unit(match, "numer", 1.0, 0.0) / convert_unit(match, "denom", 1.0, 1.0);
		total *= sign;

		RS_DEBUG->print("RS_Math::derationalize: total = '%f'", total);
        return QString::number(total, 'g', 16);
	}
	else {
		return expr;
	}
}

/**
 * Evaluates a mathematical expression and returns the result.
 * If an error occurred, ok will be set to false (if ok isn't NULL).
 */
double RS_Math::eval(const QString& expr, bool* ok) {
    bool okTmp(false);
	if(!ok) ok=&okTmp;
    if (expr.isEmpty()) {
        *ok = false;
        return 0.0;
    }

    QString derationalized = derationalize(expr);

    double ret(0.);
    try{
        mu::Parser p;
        p.DefineConst(_T("pi"),M_PI);
#ifdef _UNICODE
        p.SetExpr(derationalized.toStdWString());
#else
        p.SetExpr(derationalized.toStdString());
#endif
        ret=p.Eval();
        *ok=true;
    }
    catch (mu::Parser::exception_type &e)
    {
        mu::console() << e.GetMsg() << std::endl;
        *ok=false;
    }
    return ret;
}


/**
 * Converts a double into a string which is as short as possible
 *
 * @param value The double value
 * @param prec Precision e.g. a precision of 1 would mean that a
 *     value of 2.12030 will be converted to "2.1". 2.000 is always just "2").
 */
QString RS_Math::doubleToString(double value, double prec) {
    if (prec< RS_TOLERANCE ) {
		RS_DEBUG->print(RS_Debug::D_ERROR,
						"RS_Math::doubleToString: invalid precision");
		return QString().setNum(value, prec);
    }

	double const num = RS_Math::round(value / prec)*prec;

	QString exaStr = RS_Math::doubleToString(1./prec, 10);
	int const dotPos = exaStr.indexOf('.');

    if (dotPos==-1) {
		//big numbers for the precision
		return QString().setNum(RS_Math::round(num));
    } else {
		//number of digits after the point
		int digits = dotPos - 1;
		return RS_Math::doubleToString(num, digits);
    }
}

/**
 * Converts a double into a string which is as short as possible.
 *
 * @param value The double value
 * @param prec Precision
 */
QString RS_Math::doubleToString(double value, int prec) {
    QString valStr;

    valStr.setNum(value, 'f', prec);

    if(valStr.contains('.')) {
        // Remove tailing point and zeros:
//        valStr.replace(QRegExp("0*$"), "");
//        valStr.replace(QRegExp(R"(\.$)"), "");
//        while (valStr.at(valStr.length()-1)=='0') {
//            valStr.truncate(valStr.length()-1);
//        }

        if(valStr.at(valStr.length()-1)=='.') {
            valStr.truncate(valStr.length()-1);
        }

    }

    return valStr;
}



/**
 * Performs some testing for the math class.
 */
void RS_Math::test() {
	{
		std::cout<<"testing quadratic solver"<<std::endl;
		//equations x^2 + v[0] x + v[1] = 0
		std::vector<std::vector<double>> const eqns{
			{-1., -1.},
			{-101., -1.},
			{-1., -100.},
			{2., 1.},
			{-2., 1.}
		};
		//expected roots
		std::vector<std::vector<double>> roots{
			{-0.6180339887498948, 1.6180339887498948},
			{-0.0099000196991084878, 101.009900019699108},
			{-9.5124921972503929, 10.5124921972503929},
			{-1.},
			{1.}
		};

		for(size_t i=0; i < eqns.size(); i++) {
			std::cout<<"Test quadratic solver, test case: x^2 + ("
					<<eqns[i].front()<<") x + ("
				   <<eqns[i].back()<<") = 0"<<std::endl;
			auto sol = quadraticSolver(eqns[i]);
			assert(sol.size()==roots[i].size());
			if (sol.front() > sol.back())
				std::swap(sol[0], sol[1]);
			auto expected=roots[i];
			if (expected.front() > expected.back())
				std::swap(expected[0], expected[1]);
			for (size_t j=0; j < sol.size(); j++) {
				double x0 = sol[j];
				double x1 = expected[j];
                double const prec = (x0 - x1)/(std::abs(x0 + x1) + RS_TOLERANCE2);
				std::cout<<"root "<<j<<" : precision level = "<<prec<<std::endl;
				std::cout<<std::setprecision(17)<<"found: "<<x0<<"\texpected: "<<x1<<std::endl;
				assert(prec < RS_TOLERANCE);
			}
			std::cout<<std::endl;
		}
		return;
	}
	QString s;
    double v;

    std::cout << "RS_Math::test: doubleToString:\n";

    v = 0.1;
    s = RS_Math::doubleToString(v, 0.1);
	assert(s=="0.1");
    s = RS_Math::doubleToString(v, 0.01);
	assert(s=="0.10");

    v = 0.01;
    s = RS_Math::doubleToString(v, 0.1);
	assert(s=="0.0");
    s = RS_Math::doubleToString(v, 0.01);
	assert(s=="0.01");
	s = RS_Math::doubleToString(v, 0.001);
	assert(s=="0.010");

    v = 0.001;
    s = RS_Math::doubleToString(v, 0.1);
	assert(s=="0.0");
    s = RS_Math::doubleToString(v, 0.01);
	assert(s=="0.00");
    s = RS_Math::doubleToString(v, 0.001);
	assert(s=="0.001");

	std::cout << "RS_Math::test: complete"<<std::endl;
}



//Equation solvers

// quadratic, cubic, and quartic equation solver
// @ ce[] contains coefficient of the cubic equation:
// @ returns a vector contains real roots
//
// solvers assume arguments are valid, and there's no attempt to verify validity of the argument pointers
//
// @author Dongxu Li <dongxuli2011@gmail.com>
std::vector<double> RS_Math::quadraticSolver(const std::vector<double>& ce)
//quadratic solver for
// x^2 + ce[0] x + ce[1] =0
{
    std::vector<double> ans(0,0.);
	if (ce.size() != 2) return ans;
	using LDouble = long double;
	LDouble const b = -0.5L * ce[0];
	LDouble const c = ce[1];
	// x^2 -2 b x + c=0
	// (x - b)^2 = b^2 - c
    // b^2 >= std::abs(c)
	// x = b \pm b sqrt(1. - c/(b^2))
	LDouble const b2= b * b;
	LDouble const discriminant= b2 - c;
	LDouble const fc = std::abs(c);

	//TODO, fine tune to tolerance level
	LDouble const TOL = 1e-24L;

	if (discriminant < 0.L)
		//negative discriminant, no real root
		return ans;

	//find the radical
	LDouble r;

	// given |p| >= |q|
	// sqrt(p^2 \pm q^2) = p sqrt(1 \pm q^2/p^2)
	if (b2 >= fc)
		r = std::abs(b) * std::sqrt(1.L - c/b2);
	else
		// c is negative, because b2 - c is non-negative
		r = std::sqrt(fc) * std::sqrt(1.L + b2/fc);

	if (r >= TOL*std::abs(b)) {
		//two roots
		if (b >= 0.L)
			//since both (b,r)>=0, avoid (b - r) loss of significance
			ans.push_back(b + r);
		else
			//since b<0, r>=0, avoid (b + r) loss of significance
			ans.push_back(b - r);

		//Vieta's formulas for the second root
		ans.push_back(c/ans.front());
	} else
		//multiple roots
		ans.push_back(b);
	return ans;
}


std::vector<double> RS_Math::cubicSolver(const std::vector<double>& ce)
//cubic equation solver
// x^3 + ce[0] x^2 + ce[1] x + ce[2] = 0
{
//    std::cout<<"x^3 + ("<<ce[0]<<")*x^2+("<<ce[1]<<")*x+("<<ce[2]<<")==0"<<std::endl;
    std::vector<double> ans(0,0.);
	if (ce.size() != 3) return ans;
    // depressed cubic, Tschirnhaus transformation, x= t - b/(3a)
    // t^3 + p t +q =0
    double shift=(1./3)*ce[0];
    double p=ce[1] -shift*ce[0];
    double q=ce[0]*( (2./27)*ce[0]*ce[0]-(1./3)*ce[1])+ce[2];
    //Cardano's method,
    //	t=u+v
    //	u^3 + v^3 + ( 3 uv + p ) (u+v) + q =0
    //	select 3uv + p =0, then,
    //	u^3 + v^3 = -q
    //	u^3 v^3 = - p^3/27
    //	so, u^3 and v^3 are roots of equation,
    //	z^2 + q z - p^3/27 = 0
    //	and u^3,v^3 are,
    //		-q/2 \pm sqrt(q^2/4 + p^3/27)
    //	discriminant= q^2/4 + p^3/27
    //std::cout<<"p="<<p<<"\tq="<<q<<std::endl;
    double discriminant= (1./27)*p*p*p+(1./4)*q*q;
    if ( std::abs(p)< 1.0e-75) {
        ans.push_back(std::cbrt(q));
        ans[0] -= shift;
//        DEBUG_HEADER
//        std::cout<<"cubic: one root: "<<ans[0]<<std::endl;
        return ans;
    }
    //std::cout<<"discriminant="<<discriminant<<std::endl;
    if(discriminant>0) {
        std::vector<double> ce2(2,0.);
        ce2[0]=q;
        ce2[1]=-1./27*p*p*p;
		auto r=quadraticSolver(ce2);
        if ( r.empty() ) { //should not happen
            std::cerr<<__FILE__<<" : "<<__func__<<" : line"<<__LINE__<<" :cubicSolver()::Error cubicSolver("<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<")\n";
            return {};
        }
        double u,v;
        u= std::signbit(q) ? std::cbrt(r[0]): std::cbrt(r[1]);
        //u=(q<=0)?pow(-0.5*q+sqrt(discriminant),1./3):-pow(0.5*q+sqrt(discriminant),1./3);
        v=(-1./3)*p/u;
        //std::cout<<"u="<<u<<"\tv="<<v<<std::endl;
        //std::cout<<"u^3="<<u*u*u<<"\tv^3="<<v*v*v<<std::endl;
        ans.push_back(u+v - shift);

//        DEBUG_HEADER
//        std::cout<<"cubic: one root: "<<ans[0]<<std::endl;
	}else{
		std::complex<double> u(q,0),rt[3];
        u=std::pow(-0.5*u - std::sqrt(0.25*u*u+p*p*p/27), 1./3);
        rt[0]=u-p/(3.*u)-shift;
        std::complex<double> w(-0.5, std::sqrt(3.)/2);
		rt[1]=u*w-p/(3.*u*w)-shift;
		rt[2]=u/w-p*w/(3.*u)-shift;
		//        DEBUG_HEADER
		//        std::cout<<"Roots:\n";
		//        std::cout<<rt[0]<<std::endl;
		//        std::cout<<rt[1]<<std::endl;
		//        std::cout<<rt[2]<<std::endl;
		ans.push_back(rt[0].real());
		ans.push_back(rt[1].real());
		ans.push_back(rt[2].real());
	}
	// newton-raphson
	for(double& x0: ans){
		double dx=0.;
		for(size_t i=0; i<20; ++i){
			double f=( (x0 + ce[0])*x0 + ce[1])*x0 +ce[2];
			double df=(3.*x0+2.*ce[0])*x0 +ce[1];
            if(std::abs(df)>std::abs(f)+RS_TOLERANCE){
				dx=f/df;
				x0 -= dx;
			}else
				break;
		}
	}

    return ans;
}

/** quartic solver
* x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
@ce, a vector of size 4 contains the coefficient in order
@return, a vector contains real roots
**/
std::vector<double> RS_Math::quarticSolver(const std::vector<double>& ce)
{
    std::vector<double> ans(0,0.);
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
		DEBUG_HEADER
        std::cout<<"expected array size=4, got "<<ce.size()<<std::endl;
    }
    if(ce.size() != 4) return ans;
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
        std::cout<<"x^4+("<<ce[0]<<")*x^3+("<<ce[1]<<")*x^2+("<<ce[2]<<")*x+("<<ce[3]<<")==0"<<std::endl;
    }

    // x^4 + a x^3 + b x^2 +c x + d = 0
    // depressed quartic, x= t - a/4
    // t^4 + ( b - 3/8 a^2 ) t^2 + (c - a b/2 + a^3/8) t + d - a c /4 + a^2 b/16 - 3 a^4/256 =0
    // t^4 + p t^2 + q t + r =0
    // p= b - (3./8)*a*a;
    // q= c - 0.5*a*b+(1./8)*a*a*a;
    // r= d - 0.25*a*c+(1./16)*a*a*b-(3./256)*a^4
    double shift=0.25*ce[0];
    double shift2=shift*shift;
    double a2=ce[0]*ce[0];
    double p= ce[1] - (3./8)*a2;
    double q= ce[2] + ce[0]*((1./8)*a2 - 0.5*ce[1]);
    double r= ce[3] - shift*ce[2] + (ce[1] - 3.*shift2)*shift2;
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
		DEBUG_HEADER
        std::cout<<"x^4+("<<p<<")*x^2+("<<q<<")*x+("<<r<<")==0"<<std::endl;
    }
    if (q*q <= 1.e-4*RS_TOLERANCE*std::abs(p*r)) {// Biquadratic equations
        double discriminant= 0.25*p*p -r;
        if (discriminant < -1.e3*RS_TOLERANCE) {

//            DEBUG_HEADER
//            std::cout<<"discriminant="<<discriminant<<"\tno root"<<std::endl;
            return ans;
        }
        double t2[2];
        t2[0]=-0.5*p-sqrt(std::abs(discriminant));
        t2[1]= -p - t2[0];
        //        std::cout<<"t2[0]="<<t2[0]<<std::endl;
        //        std::cout<<"t2[1]="<<t2[1]<<std::endl;
        if ( t2[1] >= 0.) { // two real roots
            ans.push_back(sqrt(t2[1])-shift);
            ans.push_back(-sqrt(t2[1])-shift);
        }
        if ( t2[0] >= 0. ) {// four real roots
            ans.push_back(sqrt(t2[0])-shift);
            ans.push_back(-sqrt(t2[0])-shift);
        }
//        DEBUG_HEADER
//        for(int i=0;i<ans.size();i++){
//            std::cout<<"root x: "<<ans[i]<<std::endl;
//        }
        return ans;
    }
    if ( std::abs(r)< 1.0e-75 ) {
        std::vector<double> cubic(3,0.);
        cubic[1]=p;
        cubic[2]=q;
        ans.push_back(0.);
		auto r=cubicSolver(cubic);
		std::copy(r.begin(),r.end(), std::back_inserter(ans));
        for(size_t i=0; i<ans.size(); i++) ans[i] -= shift;
        return ans;
    }
    // depressed quartic to two quadratic equations
    // t^4 + p t^2 + q t + r = ( t^2 + u t + v) ( t^2 - u t + w)
    // so,
    // 	p + u^2= w+v
    // 	q/u= w-v
    // 	r= wv
    // so,
    //  (p+u^2)^2 - (q/u)^2 = 4 r
    //  y=u^2,
    //  y^3 + 2 p y^2 + ( p^2 - 4 r) y - q^2 =0
    //
    std::vector<double> cubic(3,0.);
    cubic[0]=2.*p;
    cubic[1]=p*p-4.*r;
    cubic[2]=-q*q;
	auto r3= cubicSolver(cubic);
    if (r3.empty())
        return {};
    //std::cout<<"quartic_solver:: real roots from cubic: "<<ret<<std::endl;
    //for(unsigned int i=0; i<ret; i++)
    //   std::cout<<"cubic["<<i<<"]="<<cubic[i]<<" x= "<<croots[i]<<std::endl;
	//newton-raphson
    if (r3.size()==1) { //one real root from cubic
        if (r3[0]< 0.) {//this should not happen
			DEBUG_HEADER
			qDebug()<<"Quartic Error:: Found one real root for cubic, but negative\n";
            return ans;
        }
        // Solve both quadratic factors. The cubic can report a single real root
        // when rounding has hidden a double root, and then both factors hold
        // real roots: a double root of the quartic, as in (x^2 - 1)(x - 2)^2,
        // gives the cubic a double root.
        double sqrtz0=sqrt(r3[0]);
        std::vector<double> ce2(2,0.);
        ce2[0]=	-sqrtz0;
        ce2[1]=0.5*(p+r3[0])+0.5*q/sqrtz0;
        auto r1=quadraticSolver(ce2);
        ce2[0]=	sqrtz0;
        ce2[1]=0.5*(p+r3[0])-0.5*q/sqrtz0;
        const auto r2=quadraticSolver(ce2);
        r1.insert(r1.end(), r2.begin(), r2.end());
		for(auto& x: r1){
			x -= shift;
		}
        return r1;
    }
    if ( r3[0]> 0. && r3[1] > 0. ) {
        double sqrtz0=sqrt(r3[0]);
        std::vector<double> ce2(2,0.);
        ce2[0]=	-sqrtz0;
        ce2[1]=0.5*(p+r3[0])+0.5*q/sqrtz0;
        ans=quadraticSolver(ce2);
        ce2[0]=	sqrtz0;
        ce2[1]=0.5*(p+r3[0])-0.5*q/sqrtz0;
		auto r1=quadraticSolver(ce2);
		std::copy(r1.begin(),r1.end(),std::back_inserter(ans));
		for(auto& x: ans){
			x -= shift;
		}
    }
	// newton-raphson
	for(double& x0: ans){
		double dx=0.;
		for(size_t i=0; i<20; ++i){
			double f=(( (x0 + ce[0])*x0 + ce[1])*x0 +ce[2])*x0 + ce[3] ;
			double df=((4.*x0+3.*ce[0])*x0 +2.*ce[1])*x0+ce[2];
//			DEBUG_HEADER
//			qDebug()<<"i="<<i<<"\tx0="<<x0<<"\tf="<<f<<"\tdf="<<df;
            if(std::abs(df)>RS_TOLERANCE2){
				dx=f/df;
				x0 -= dx;
			}else
				break;
		}
	}

    return ans;
}

/** quartic solver
* ce[4] x^4 + ce[3] x^3 + ce[2] x^2 + ce[1] x + ce[0] = 0
@ce, a vector of size 5 contains the coefficient in order
@return, a vector contains real roots
*ToDo, need a robust algorithm to locate zero terms, better handling of tolerances
**/
std::vector<double> RS_Math::quarticSolverFull(const std::vector<double>& ce)
{
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
		DEBUG_HEADER
        std::cout<<ce[4]<<"*y^4+("<<ce[3]<<")*y^3+("<<ce[2]<<"*y^2+("<<ce[1]<<")*y+("<<ce[0]<<")==0"<<std::endl;
    }

    std::vector<double> roots(0,0.);
    if(ce.size()!=5) return roots;
    std::vector<double> ce2(4,0.);

    if ( std::abs(ce[4]) < 1.0e-14) { // this should not happen
        if ( std::abs(ce[3]) < 1.0e-14) { // this should not happen
            if ( std::abs(ce[2]) < 1.0e-14) { // this should not happen
                if( std::abs(ce[1]) > 1.0e-14) {
                    roots.push_back(-ce[0]/ce[1]);
                } else { // can not determine y. this means overlapped, but overlap should have been detected before, therefore return empty set
                    return roots;
                }
            } else {
                ce2.resize(2);
                ce2[0]=ce[1]/ce[2];
                ce2[1]=ce[0]/ce[2];
                //std::cout<<"ce2[2]={ "<<ce2[0]<<' '<<ce2[1]<<" }\n";
                roots=RS_Math::quadraticSolver(ce2);
            }
        } else {
            ce2.resize(3);
            ce2[0]=ce[2]/ce[3];
            ce2[1]=ce[1]/ce[3];
            ce2[2]=ce[0]/ce[3];
            //std::cout<<"ce2[3]={ "<<ce2[0]<<' '<<ce2[1]<<' '<<ce2[2]<<" }\n";
            roots=RS_Math::cubicSolver(ce2);
        }
    } else {
        ce2[0]=ce[3]/ce[4];
        ce2[1]=ce[2]/ce[4];
        ce2[2]=ce[1]/ce[4];
        ce2[3]=ce[0]/ce[4];
        if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
			DEBUG_HEADER
            std::cout<<"ce2[4]={ "<<ce2[0]<<' '<<ce2[1]<<' '<<ce2[2]<<' '<<ce2[3]<<" }\n";
        }
        if(std::abs(ce2[3])<= RS_TOLERANCE15) {
            //constant term is zero, factor 0 out, solve a cubic equation
            ce2.resize(3);
            roots=RS_Math::cubicSolver(ce2);
            roots.push_back(0.);
        }else
            roots=RS_Math::quarticSolver(ce2);
    }
    return roots;
}

//linear Equation solver by Gauss-Jordan
/**
  * Solve linear equation set
  *@ mt holds the augmented matrix
  *@ sn holds the solution
  *@ return true, if the equation set has a unique solution, return false otherwise
  *
  *@Author: Dongxu Li
  */

bool RS_Math::linearSolver(const std::vector<std::vector<double> >& mt, std::vector<double>& sn){
    //verify the matrix size
	size_t mSize(mt.size()); //rows
	size_t aSize(mSize+1); //columns of augmented matrix
	if(std::any_of(mt.begin(), mt.end(), [&aSize](const std::vector<double>& v)->bool{
				   return v.size() != aSize;
}))
		return false;
    sn.resize(mSize);//to hold the solution
#if false
    boost::numeric::ublas::matrix<double> bm (mSize, mSize);
    boost::numeric::ublas::vector<double> bs(mSize);

    for(int i=0;i<mSize;i++) {
        for(int j=0;j<mSize;j++) {
            bm(i,j)=mt[i][j];
        }
        bs(i)=mt[i][mSize];
    }
    //solve the linear equation set by LU decomposition in boost ublas

    if ( boost::numeric::ublas::lu_factorize<boost::numeric::ublas::matrix<double> >(bm) ) {
		std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
        std::cout<<" linear solver failed"<<std::endl;
        //        RS_DEBUG->print(RS_Debug::D_WARNING, "linear solver failed");
        return false;
    }

    boost::numeric::ublas:: triangular_matrix<double, boost::numeric::ublas::unit_lower>
            lm = boost::numeric::ublas::triangular_adaptor< boost::numeric::ublas::matrix<double>,  boost::numeric::ublas::unit_lower>(bm);
    boost::numeric::ublas:: triangular_matrix<double,  boost::numeric::ublas::upper>
            um =  boost::numeric::ublas::triangular_adaptor< boost::numeric::ublas::matrix<double>,  boost::numeric::ublas::upper>(bm);
    ;
    boost::numeric::ublas::inplace_solve(lm,bs, boost::numeric::ublas::lower_tag());
    boost::numeric::ublas::inplace_solve(um,bs, boost::numeric::ublas::upper_tag());
    for(int i=0;i<mSize;i++){
        sn[i]=bs(i);
    }
    //    std::cout<<"dn="<<dn<<std::endl;
    //    data.center.set(-0.5*dn(1)/dn(0),-0.5*dn(3)/dn(2)); // center
    //    double d(1.+0.25*(dn(1)*dn(1)/dn(0)+dn(3)*dn(3)/dn(2)));
    //    if(std::abs(dn(0))<RS_TOLERANCE2
    //            ||std::abs(dn(2))<RS_TOLERANCE2
    //            ||d/dn(0)<RS_TOLERANCE2
    //            ||d/dn(2)<RS_TOLERANCE2
    //            ) {
    //        //ellipse not defined
    //        return false;
    //    }
    //    d=sqrt(d/dn(0));
    //    data.majorP.set(d,0.);
    //    data.ratio=sqrt(dn(0)/dn(2));
#else
    // solve the linear equation by Gauss-Jordan elimination
	std::vector<std::vector<double> > mt0(mt); //copy the matrix;
	for(size_t i=0;i<mSize;++i){
		size_t imax(i);
        double cmax(std::abs(mt0[i][i]));
		for(size_t j=i+1;j<mSize;++j) {
            if(std::abs(mt0[j][i]) > cmax ) {
                imax=j;
                cmax=std::abs(mt0[j][i]);
            }
        }
	
	// issue #1386: relax singular condition
	// TODO: switch to QR-decomposition based algorithms
        if(cmax<RS_TOLERANCE) return false; //singular matrix
        if(imax != i) {//move the line with largest absolute value at column i to row i, to avoid division by zero
            std::swap(mt0[i],mt0[imax]);
		}
		for(size_t k=i+1;k<=mSize;++k) { //normalize the i-th row
            mt0[i][k] /= mt0[i][i];
        }
		mt0[i][i]=1.;
		for(size_t j=0;j<mSize;++j) {//Gauss-Jordan
            if(j != i ) {
				double& a = mt0[j][i];
				for(size_t k=i+1;k<=mSize;++k) {
					mt0[j][k] -= mt0[i][k]*a;
                }
				a=0.;
            }
		}
		//output gauss-jordan results for debugging
//		std::cout<<"========"<<i<<"==========\n";
//		for(auto v0: mt0){
//			for(auto v1:v0)
//				std::cout<<v1<<'\t';
//			std::cout<<std::endl;
//		}
    }
	for(size_t i=0;i<mSize;++i) {
        sn[i]=mt0[i][mSize];
    }
#endif

    return true;
}

/**
 * wrapper of elliptic integral of the second type, Legendre form
 * @param k the elliptic modulus or eccentricity
 * @param phi elliptic angle, must be within range of [0, M_PI]
 *
 * @author: Dongxu Li
 */
double RS_Math::ellipticIntegral_2(const double& k, const double& phi)
{
    double a= remainder(phi-M_PI_2,M_PI);
    if(a>0.) {
        return boost::math::ellint_2<double,double>(k,a);
    } else {
        return - boost::math::ellint_2<double,double>(k,std::abs(a));
    }
}

/** solver quadratic simultaneous equations of set two **/
/* solve the following quadratic simultaneous equations,
  *  ma000 x^2 + ma011 y^2 - 1 =0
  * ma100 x^2 + 2 ma101 xy + ma111 y^2 + mb10 x + mb11 y +mc1 =0
  *
  *@m, a vector of size 8 contains coefficients in the strict order of:
  ma000 ma011 ma100 ma101 ma111 mb10 mb11 mc1
  * m[0] m[1] must be positive
  *@return a vector contains real roots
  */
RS_VectorSolutions RS_Math::simultaneousQuadraticSolver(const std::vector<double>& m)
{
    RS_VectorSolutions ret(0);
    if(m.size() != 8 ) return ret; // valid m should contain exact 8 elements
    std::vector< double> c1(0,0.);
    std::vector< std::vector<double> > m1(0,c1);
    c1.resize(6);
    c1[0]=m[0];
    c1[1]=0.;
    c1[2]=m[1];
    c1[3]=0.;
    c1[4]=0.;
    c1[5]=-1.;
    m1.push_back(c1);
    c1[0]=m[2];
    c1[1]=2.*m[3];
    c1[2]=m[4];
    c1[3]=m[5];
    c1[4]=m[6];
    c1[5]=m[7];
    m1.push_back(c1);

    return simultaneousQuadraticSolverFull(m1);
}

namespace {
// A polynomial coefficient summed from its terms. It is zero when it is below the rounding error of
// that sum. An absolute threshold depends on the units: the coefficients of an ellipse are about
// 1/a^2, and those of the elimination quartic about their fourth powers, below 1e-14 for a = 500.
struct Coefficient {
    double value = 0.;
    double size = 0.;
    void add(double term) {
        value += term;
        size += std::abs(term);
    }
    bool isZero() const {
        return std::abs(value) <= 64. * DBL_EPSILON * size;
    }
};

// the real roots of sum_k ce[k] y^k, of the degree of its highest coefficient that is not zero
std::vector<double> polynomialRoots(const std::vector<Coefficient>& ce)
{
    size_t degree = ce.size() - 1;
    while (degree > 0 && ce[degree].isZero()) {
        --degree;
    }
    if (degree == 0) {
        return {};
    }
    if (ce.front().isZero()) {
        std::vector<double> roots = polynomialRoots({ce.begin() + 1, ce.begin() + degree + 1});
        roots.push_back(0.);
        return roots;
    }
    const double lead = ce[degree].value;
    switch (degree) {
    case 1:
        return {-ce[0].value/lead};
    case 2:
        return RS_Math::quadraticSolver({ce[1].value/lead, ce[0].value/lead});
    case 3:
        return RS_Math::cubicSolver({ce[2].value/lead, ce[1].value/lead, ce[0].value/lead});
    default:
        return RS_Math::quarticSolver({ce[3].value/lead, ce[2].value/lead, ce[1].value/lead, ce[0].value/lead});
    }
}

// A point near a conic: its first-order distance |F|/|grad F| from it, and the radius of curvature
struct ConicPoint {
    double distance = 0.;
    double radius = std::numeric_limits<double>::infinity();
};

ConicPoint conicPoint(const std::vector<double>& ce, const RS_Vector& p)
{
    ConicPoint ret;
    const double x = p.x, y = p.y;
    const double terms[6] = {ce[0]*x*x, ce[1]*x*y, ce[2]*y*y, ce[3]*x, ce[4]*y, ce[5]};
    double f = 0., fSize = 0.;
    for (double term: terms) {
        f += term;
        fSize += std::abs(term);
    }
    const double fx = 2.*ce[0]*x + ce[1]*y + ce[3];
    const double fy = ce[1]*x + 2.*ce[2]*y + ce[4];
    const double gradientSize = std::abs(2.*ce[0]*x) + std::abs(ce[1]*y) + std::abs(ce[3])
                                + std::abs(ce[1]*x) + std::abs(2.*ce[2]*y) + std::abs(ce[4]);
    const double gradient = std::hypot(fx, fy);
    // on the conic to the rounding of F, as at a singular point, where the lines of a degenerate
    // conic cross
    if (std::abs(f) <= 64. * DBL_EPSILON * fSize) {
        return ret;
    }
    // a stationary point of F off the conic, as the center of an ellipse
    if (!(gradient > std::sqrt(DBL_EPSILON) * gradientSize)) {
        ret.distance = std::numeric_limits<double>::infinity();
        return ret;
    }
    ret.distance = std::abs(f)/gradient;
    const double bending = std::abs(2.*ce[0]*fy*fy - 2.*ce[1]*fx*fy + 2.*ce[2]*fx*fx);
    if (bending > 0.) {
        ret.radius = gradient*gradient*gradient/bending;
    }
    return ret;
}

// How far a point is from both conics, relative to the smaller radius of curvature. Unlike |F|
// relative to its largest term, this does not depend on the coordinates: that residual accepted
// points far from a small conic away from the origin.
double offConics(const std::vector<std::vector<double> >& m, const RS_Vector& p)
{
    const ConicPoint first = conicPoint(m[0], p);
    const ConicPoint second = conicPoint(m[1], p);
    const double distance = std::max(first.distance, second.distance);
    if (!(distance > 0.) || !std::isfinite(distance)) {
        return distance;
    }
    // a conic that is straight at the point has no radius: that point is left to the residual
    return distance / std::min(first.radius, second.radius);
}

// a point on both conics, closer to them than this times their radius of curvature
constexpr double onConicsTolerance = 1e-8;
} // namespace

/** solver quadratic simultaneous equations of a set of two **/
/* solve the following quadratic simultaneous equations,
  * ma000 x^2 + ma001 xy + ma011 y^2 + mb00 x + mb01 y + mc0 =0
  * ma100 x^2 + ma101 xy + ma111 y^2 + mb10 x + mb11 y + mc1 =0
  *
  *@m, a vector of size 2 each contains a vector of size 6 coefficients in the strict order of:
  ma000 ma001 ma011 mb00 mb01 mc0
  ma100 ma101 ma111 mb10 mb11 mc1
  *@return a RS_VectorSolutions contains real roots (x,y)
  */
RS_VectorSolutions RS_Math::simultaneousQuadraticSolverFull(const std::vector<std::vector<double> >& m)
{
    RS_VectorSolutions ret;
    if(m.size()!=2)  return ret;
    if( m[0].size() ==3 || m[1].size()==3 ){
        return simultaneousQuadraticSolverMixed(m);
    }
    if(m[0].size()!=6 || m[1].size()!=6) return ret;
    /** eliminate x, quartic equation of y **/
    auto& a=m[0][0];
    auto& b=m[0][1];
    auto& c=m[0][2];
    auto& d=m[0][3];
    auto& e=m[0][4];
    auto& f=m[0][5];

    auto& g=m[1][0];
    auto& h=m[1][1];
    auto& i=m[1][2];
    auto& j=m[1][3];
    auto& k=m[1][4];
    auto& l=m[1][5];
    /**
      Collect[Eliminate[{ a*x^2 + b*x*y+c*y^2+d*x+e*y+f==0,g*x^2+h*x*y+i*y^2+j*x+k*y+l==0},x],y]
      **/
    /*
     f^2 g^2 - d f g j + a f j^2 - 2 a f g l + (2 e f g^2 - d f g h - b f g j + 2 a f h j - 2 a f g k) y + (2 c f g^2 - b f g h + a f h^2 - 2 a f g i) y^2
 ==
 -(d^2 g l) + a d j l - a^2 l^2
+
 (d e g j - a e j^2 - d^2 g k + a d j k - 2 b d g l + 2 a e g l + a d h l + a b j l - 2 a^2 k l) y
+
 (-(e^2 g^2) + d e g h - d^2 g i + c d g j + b e g j - 2 a e h j + a d i j - a c j^2 - 2 b d g k + 2 a e g k + a d h k + a b j k - a^2 k^2 - b^2 g l + 2 a c g l + a b h l - 2 a^2 i l) y^2
 +
(-2 c e g^2 + c d g h + b e g h - a e h^2 - 2 b d g i + 2 a e g i + a d h i + b c g j - 2 a c h j + a b i j - b^2 g k + 2 a c g k + a b h k - 2 a^2 i k) y^3
+
 (-(c^2 g^2) + b c g h - a c h^2 - b^2 g i + 2 a c g i + a b h i - a^2 i^2) y^4


      */
    double a2=a*a;
    double b2=b*b;
    double c2=c*c;
    double d2=d*d;
    double e2=e*e;
    double f2=f*f;

    double g2=g*g;
    double  h2=h*h;
    double  i2=i*i;
    double  j2=j*j;
    double  k2=k*k;
    double  l2=l*l;
    // the coefficients with the sizes of their terms, see Coefficient
    std::vector<Coefficient> qy(5);
    //y^4
    qy[4].add(-c2*g2); qy[4].add(b*c*g*h); qy[4].add(-a*c*h2); qy[4].add(-b2*g*i); qy[4].add(2.*a*c*g*i);
    qy[4].add(a*b*h*i); qy[4].add(-a2*i2);
    //y^3
    qy[3].add(-2.*c*e*g2); qy[3].add(c*d*g*h); qy[3].add(b*e*g*h); qy[3].add(-a*e*h2); qy[3].add(-2.*b*d*g*i);
    qy[3].add(2.*a*e*g*i); qy[3].add(a*d*h*i); qy[3].add(b*c*g*j); qy[3].add(-2.*a*c*h*j); qy[3].add(a*b*i*j);
    qy[3].add(-b2*g*k); qy[3].add(2.*a*c*g*k); qy[3].add(a*b*h*k); qy[3].add(-2.*a2*i*k);
    //y^2
    qy[2].add(-e2*g2); qy[2].add(d*e*g*h); qy[2].add(-d2*g*i); qy[2].add(c*d*g*j); qy[2].add(b*e*g*j);
    qy[2].add(-2.*a*e*h*j); qy[2].add(a*d*i*j); qy[2].add(-a*c*j2); qy[2].add(-2.*b*d*g*k);
    qy[2].add(2.*a*e*g*k); qy[2].add(a*d*h*k); qy[2].add(a*b*j*k); qy[2].add(-a2*k2); qy[2].add(-b2*g*l);
    qy[2].add(2.*a*c*g*l); qy[2].add(a*b*h*l); qy[2].add(-2.*a2*i*l); qy[2].add(-2.*c*f*g2);
    qy[2].add(b*f*g*h); qy[2].add(-a*f*h2); qy[2].add(2.*a*f*g*i);
    //y
    qy[1].add(d*e*g*j); qy[1].add(-a*e*j2); qy[1].add(-d2*g*k); qy[1].add(a*d*j*k); qy[1].add(-2.*b*d*g*l);
    qy[1].add(2.*a*e*g*l); qy[1].add(a*d*h*l); qy[1].add(a*b*j*l); qy[1].add(-2.*a2*k*l);
    qy[1].add(-2.*e*f*g2); qy[1].add(d*f*g*h); qy[1].add(b*f*g*j); qy[1].add(-2.*a*f*h*j);
    qy[1].add(2.*a*f*g*k);
    //y^0
    qy[0].add(-d2*g*l); qy[0].add(a*d*j*l); qy[0].add(-a2*l2); qy[0].add(-f2*g2); qy[0].add(d*f*g*j);
    qy[0].add(-a*f*j2); qy[0].add(2.*a*f*g*l);
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
        DEBUG_HEADER
        std::cout<<qy[4].value<<"*y^4 +("<<qy[3].value<<")*y^3+("<<qy[2].value<<")*y^2+("<<qy[1].value<<")*y+("<<qy[0].value<<")==0"<<std::endl;
    }
    auto roots=polynomialRoots(qy);
    // A tangent point is a double root, or a quadruple one where two conics touch at two points
    // with the same y, and the solvers can lose such roots to rounding. They are also roots of
    // the derivative; candidates from it that are not intersections fail the tests of addPoint.
    std::vector<Coefficient> derivative(4);
    for (size_t k = 1; k < qy.size(); ++k) {
        derivative[k-1].value = k*qy[k].value;
        derivative[k-1].size = k*qy[k].size;
    }
    for (double y: polynomialRoots(derivative)) {
        roots.push_back(y);
    }
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
        std::cout<<"roots.size()= "<<roots.size()<<std::endl;
    }

    if (roots.size()==0 ) { // no intersection found
        return ret;
    }
    // Keep a verified point on both conics, once. Two points are the same intersection when the
    // conics are within onConicsTolerance of the point halfway between them too: near a tangent
    // point candidates from different roots are all verified, and the conics are as close as
    // that between them, while between two separate intersections at least one conic is farther.
    std::vector<RS_Vector> points;
    const auto addPoint = [&](RS_Vector vp) {
        if (!simultaneousQuadraticVerify(m, vp) || offConics(m, vp) > onConicsTolerance) {
            return;
        }
        for (RS_Vector& point: points) {
            if (offConics(m, (point + vp) * 0.5) <= onConicsTolerance) {
                if (offConics(m, vp) < offConics(m, point)) {
                    point = vp;
                }
                return;
            }
        }
        points.push_back(vp);
    };

    std::vector<double> ce(3,0.);
    for (const double y: roots) {
        if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
            DEBUG_HEADER
            std::cout<<"y="<<y<<std::endl;
        }
        // x from the first conic at this y, or from the second one where the first does not give it
        ce[0]=a;
        ce[1]=b*y+d;
        ce[2]=c*y*y+e*y+f;
        if(std::abs(ce[0])<1e-75 && std::abs(ce[1])<1e-75) {
            ce[0]=g;
            ce[1]=h*y+j;
            ce[2]=i*y*y+k*y+l;
        }
        if(std::abs(ce[0])<1e-75 && std::abs(ce[1])<1e-75) continue;

        if(std::abs(ce[0])>1e-75){
            const std::vector<double> ce2{ce[1]/ce[0], ce[2]/ce[0]};
            auto xRoots=quadraticSolver(ce2);
            if (xRoots.empty()) {
                // No real x at this y. At a tangent point the discriminant can be zero, rounded
                // below zero: the double root, which fails the tests of addPoint otherwise.
                xRoots.push_back(-0.5*ce2[0]);
            }
            for (double x: xRoots) {
                addPoint(RS_Vector(x, y));
            }
            continue;
        }
        addPoint(RS_Vector(-ce[2]/ce[1], y));
    }
    for (const RS_Vector& point: points) {
        ret.push_back(point);
    }
    if(RS_DEBUG->getLevel()>=RS_Debug::D_INFORMATIONAL){
        DEBUG_HEADER
        std::cout<<"ret="<<ret<<std::endl;
    }
    return ret;
}

RS_VectorSolutions RS_Math::simultaneousQuadraticSolverMixed(const std::vector<std::vector<double> >& m)
{
    RS_VectorSolutions ret;
    auto p0=& (m[0]);
    auto p1=& (m[1]);
    if(p1->size()==3){
        std::swap(p0,p1);
    }
    if(p1->size()==3) {
            //linear
			std::vector<double> sn(2,0.);
			std::vector<std::vector<double> > ce;
			ce.push_back(m[0]);
			ce.push_back(m[1]);
            ce[0][2]=-ce[0][2];
            ce[1][2]=-ce[1][2];
            if( RS_Math::linearSolver(ce,sn)) ret.push_back(RS_Vector(sn[0],sn[1]));
            return ret;
    }
//    DEBUG_HEADER
//    std::cout<<"p0: size="<<p0->size()<<"\n Solve[{("<< p0->at(0)<<")*x + ("<<p0->at(1)<<")*y + ("<<p0->at(2)<<")==0,";
//    std::cout<<"("<< p1->at(0)<<")*x^2 + ("<<p1->at(1)<<")*x*y + ("<<p1->at(2)<<")*y^2 + ("<<p1->at(3)<<")*x +("<<p1->at(4)<<")*y+("
//            <<p1->at(5)<<")==0},{x,y}]"<<std::endl;
    const double& a=p0->at(0);
    const double& b=p0->at(1);
    const double& c=p0->at(2);
    const double& d=p1->at(0);
    const double& e=p1->at(1);
    const double& f=p1->at(2);
    const double& g=p1->at(3);
    const double& h=p1->at(4);
    const double& i=p1->at(5);
    /**
      y (2 b c d-a c e)-a c g+c^2 d = y^2 (a^2 (-f)+a b e-b^2 d)+y (a b g-a^2 h)+a^2 (-i)
      */
    std::vector<double> ce(3,0.);
	const double& a2=a*a;
	const double& b2=b*b;
	const double& c2=c*c;
    ce[0]= -f*a2+a*b*e-b2*d;
    ce[1]=a*b*g-a2*h- (2*b*c*d-a*c*e);
    ce[2]=a*c*g-c2*d-a2*i;
//    DEBUG_HEADER
//    std::cout<<"("<<ce[0]<<") y^2 + ("<<ce[1]<<") y + ("<<ce[2]<<")==0"<<std::endl;
    std::vector<double> roots(0,0.);
    if( std::abs(ce[1])>RS_TOLERANCE15 && std::abs(ce[0]/ce[1])<RS_TOLERANCE15){
        roots.push_back( - ce[2]/ce[1]);
    }else{
        std::vector<double> ce2(2,0.);
        ce2[0]=ce[1]/ce[0];
        ce2[1]=ce[2]/ce[0];
        roots=quadraticSolver(ce2);
    }
//    for(size_t i=0;i<roots.size();i++){
//    std::cout<<"x="<<roots.at(i)<<std::endl;
//    }


    if(roots.size()==0)  {
        return RS_VectorSolutions();
    }
    for(size_t i=0;i<roots.size();i++){
        ret.push_back(RS_Vector(-(b*roots.at(i)+c)/a,roots.at(i)));
//        std::cout<<ret.at(ret.size()-1).x<<", "<<ret.at(ret.size()-1).y<<std::endl;
    }

    return ret;

}

/** verify a solution for simultaneousQuadratic
  *@m the coefficient matrix
  *@v, a candidate to verify
  *@return true, for a valid solution
  **/
bool RS_Math::simultaneousQuadraticVerify(const std::vector<std::vector<double> >& m, RS_Vector& v)
{
	RS_Vector v0=v;
	auto& a=m[0][0];
	auto& b=m[0][1];
	auto& c=m[0][2];
	auto& d=m[0][3];
	auto& e=m[0][4];
	auto& f=m[0][5];

	auto& g=m[1][0];
	auto& h=m[1][1];
	auto& i=m[1][2];
	auto& j=m[1][3];
	auto& k=m[1][4];
	auto& l=m[1][5];
    /**
      * tolerance test for bug#3606099
      * verifying the equations to floating point tolerance by terms
      */
	double sum0=0., sum1=0.;
	double f00=0.,f01=0.;
	double amax0, amax1;
	double amax00=0., amax01=0.;
	for(size_t i0=0; i0<20; ++i0){
		double& x=v.x;
		double& y=v.y;
		double x2=x*x;
		double y2=y*y;
		double const terms0[12]={ a*x2, b*x*y, c*y2, d*x, e*y, f, g*x2, h*x*y, i*y2, j*x, k*y, l};
        amax0=std::abs(terms0[0]), amax1=std::abs(terms0[6]);
		double px=2.*a*x+b*y+d;
		double py=b*x+2.*c*y+e;
		sum0=0.;
		for(int i=0; i<6; i++) {
            if(amax0<std::abs(terms0[i])) amax0=std::abs(terms0[i]);
			sum0 += terms0[i];
		}
		std::vector<std::vector<double>> nrCe;
		nrCe.push_back(std::vector<double>{px, py, sum0});
		px=2.*g*x+h*y+j;
		py=h*x+2.*i*y+k;
		sum1=0.;
		for(int i=6; i<12; i++) {
            if(amax1<std::abs(terms0[i])) amax1=std::abs(terms0[i]);
			sum1 += terms0[i];
		}
		nrCe.push_back(std::vector<double>{px, py, sum1});
		std::vector<double> dn;
		bool ret=linearSolver(nrCe, dn);
//		DEBUG_HEADER
//		qDebug()<<"i0="<<i0<<"\tf=("<<sum0<<','<<sum1<<")\tdn=("<<dn[0]<<","<<dn[1]<<")";
		if(!i0){
			f00=sum0;
			f01=sum1;
			amax00=amax0;
			amax01=amax1;
		}
		if(!ret) break;
		v -= RS_Vector(dn[0], dn[1]);
	}
    if( std::abs(sum0)> std::abs(f00) && std::abs(sum1)>std::abs(f01)){
		v=v0;
		sum0=f00;
		sum1=f01;
		// the term sizes at v0 too: those of a diverged step accepted points off the conics
		amax0=amax00;
		amax1=amax01;
	}

//    DEBUG_HEADER
//    std::cout<<"verifying: x="<<x<<"\ty="<<y<<std::endl;
//    std::cout<<"0: maxterm: "<<amax0<<std::endl;
//    std::cout<<"verifying: std::abs(a*x2 + b*x*y+c*y2+d*x+e*y+f)/maxterm="<<std::abs(sum0)/amax0<<" required to be smaller than "<<sqrt(6.)*sqrt(DBL_EPSILON)<<std::endl;
//    std::cout<<"1: maxterm: "<<amax1<<std::endl;
//    std::cout<<"verifying: std::abs(g*x2+h*x*y+i*y2+j*x+k*y+l)/maxterm="<< std::abs(sum1)/amax1<<std::endl;
    const double tols=2.*sqrt(6.)*sqrt(DBL_EPSILON); //experimental tolerances to verify simultaneous quadratic

    return (amax0<=tols || std::abs(sum0)/amax0<tols) &&  (amax1<=tols || std::abs(sum1)/amax1<tols);
}
//EOF
