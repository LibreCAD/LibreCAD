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
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <cmath>
#include <muParser.h>
#include <QString>
#include <QDebug>

#include "rs_math.h"
#include "rs_vector.h"
#include "rs_debug.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif


typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<50>> Precise_Decimal;


namespace {
constexpr double m_piX2 = M_PI*2; //2*PI
}

/**
 * Rounds the given double to the closest int.
 */
int RS_Math::round(double v) {
    return (int) lrint(v);
}

double RS_Math::round(const double v, const double precision)
{
    return precision * llround(v / precision);
}

/**
 * Save pow function
 */
double RS_Math::pow(double x, double y) {
    errno = 0;
    double ret = ::pow(x, y);
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
RS_Vector RS_Math::pow(RS_Vector vp, double y) {
    return RS_Vector(pow(vp.x,y),pow(vp.y,y));
}

/**
 * Save equal function for real types
 */
bool RS_Math::equal(const double d1, const double d2)
{
    return fabs(d1 - d2) < RS_TOLERANCE;
}

/**
 * Converts radians to degrees.
 */
double RS_Math::rad2deg(double a) {
	return 180./M_PI*a;
}

/**
 * Converts degrees to radians.
 */
double RS_Math::deg2rad(double a) {
	return M_PI/180.0*a;
}

/**
 * Converts radians to gradians.
 */
double RS_Math::rad2gra(double a) {
	return 200./M_PI*a;
}

double RS_Math::gra2rad(double a) {
	return M_PI/200.*a;
}


/**
 * Finds greatest common divider using Euclid's algorithm.
 */
unsigned RS_Math::findGCD(unsigned a, unsigned b) {

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
    return fmod(M_PI + remainder(a - M_PI, m_piX2), m_piX2);
}

/**
 * Corrects the given angle to the range of -PI to +PI.
 */
double RS_Math::correctAngle2(double a) {
    return remainder(a, m_piX2);
}

/**
 * Returns the given angle as an Unsigned Angle in the range of 0 to +PI.
 */
double RS_Math::correctAngleU(double a) {
    return fabs(remainder(a, m_piX2));
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
        return fabs(remainder(angle, m_piX2)) < (M_PI_2 - tolerance);
    else
        return fabs(remainder(angle, m_piX2)) < (M_PI_2 + tolerance);
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

    bool ok;
    double res = RS_Math::eval(expr, &ok);

    if (!ok) {
        //std::cerr << "RS_Math::evaluate: Parse error at col "
        //<< ret << ": " << fp.ErrorMsg() << "\n";
        return def;
    }

    return res;
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
    double ret(0.);
    try{
        mu::Parser p;
        p.DefineConst(_T("pi"),M_PI);
#ifdef _UNICODE
        p.SetExpr(expr.toStdWString());
#else
        p.SetExpr(expr.toStdString());
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
				double const prec = (x0 - x1)/(fabs(x0 + x1) + RS_TOLERANCE2);
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
	// b^2 >= fabs(c)
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
    if ( fabs(p)< 1.0e-75) {
        ans.push_back((q>0)?-pow(q,(1./3)):pow(-q,(1./3)));
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
        if ( r.size()==0 ) { //should not happen
			std::cerr<<__FILE__<<" : "<<__func__<<" : line"<<__LINE__<<" :cubicSolver()::Error cubicSolver("<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<")\n";
        }
        double u,v;
        u= (q<=0) ? pow(r[0], 1./3): -pow(-r[1],1./3);
        //u=(q<=0)?pow(-0.5*q+sqrt(discriminant),1./3):-pow(0.5*q+sqrt(discriminant),1./3);
        v=(-1./3)*p/u;
        //std::cout<<"u="<<u<<"\tv="<<v<<std::endl;
        //std::cout<<"u^3="<<u*u*u<<"\tv^3="<<v*v*v<<std::endl;
        ans.push_back(u+v - shift);

//        DEBUG_HEADER
//        std::cout<<"cubic: one root: "<<ans[0]<<std::endl;
	}else{
		std::complex<double> u(q,0),rt[3];
		u=std::pow(-0.5*u-sqrt(0.25*u*u+p*p*p/27),1./3);
		rt[0]=u-p/(3.*u)-shift;
		std::complex<double> w(-0.5,sqrt(3.)/2);
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
			if(fabs(df)>fabs(f)+RS_TOLERANCE){
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
    if (q*q <= 1.e-4*RS_TOLERANCE*fabs(p*r)) {// Biquadratic equations
        double discriminant= 0.25*p*p -r;
        if (discriminant < -1.e3*RS_TOLERANCE) {

//            DEBUG_HEADER
//            std::cout<<"discriminant="<<discriminant<<"\tno root"<<std::endl;
            return ans;
        }
        double t2[2];
        t2[0]=-0.5*p-sqrt(fabs(discriminant));
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
    if ( fabs(r)< 1.0e-75 ) {
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
        double sqrtz0=sqrt(r3[0]);
        std::vector<double> ce2(2,0.);
        ce2[0]=	-sqrtz0;
        ce2[1]=0.5*(p+r3[0])+0.5*q/sqrtz0;
        auto r1=quadraticSolver(ce2);
        if (r1.size()==0 ) {
            ce2[0]=	sqrtz0;
            ce2[1]=0.5*(p+r3[0])-0.5*q/sqrtz0;
            r1=quadraticSolver(ce2);
        }
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
			if(fabs(df)>RS_TOLERANCE2){
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

    if ( fabs(ce[4]) < 1.0e-14) { // this should not happen
        if ( fabs(ce[3]) < 1.0e-14) { // this should not happen
            if ( fabs(ce[2]) < 1.0e-14) { // this should not happen
                if( fabs(ce[1]) > 1.0e-14) {
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
        if(fabs(ce2[3])<= RS_TOLERANCE15) {
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
    //    if(fabs(dn(0))<RS_TOLERANCE2
    //            ||fabs(dn(2))<RS_TOLERANCE2
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
        double cmax(fabs(mt0[i][i]));
		for(size_t j=i+1;j<mSize;++j) {
            if(fabs(mt0[j][i]) > cmax ) {
                imax=j;
                cmax=fabs(mt0[j][i]);
            }
        }
        if(cmax<RS_TOLERANCE2) return false; //singular matrix
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
        return - boost::math::ellint_2<double,double>(k,fabs(a));
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


/*
    Solves two conic sections equations simultaneously.

    It is used to find the intersection points of two conic sections.

    An arbitrary conic section (equation) is of the following form:

    Ax^2 + Bxy + Cy^2 + Dx + Ey + F = 0

    where A, B, C, D, and E are arbitrary coefficients, and F is an arbitrary constant.

    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
RS_VectorSolutions RS_Math::simultaneousQuadraticSolverFull(const std::vector<std::vector<double>>& input_conicSections)
{
    RS_VectorSolutions intersectionPoints;

    if (input_conicSections.size() != 2) return intersectionPoints;

    std::vector<std::vector<double>> conicSections;

    unsigned int numberOf_degenerateEquations = 0;

    unsigned int degenerateEquation_number = 0;

    if ((input_conicSections[0].size() == 3) || (input_conicSections[1].size() == 3))
    {
        for (int i = 0; i < 2; i++)
        {
            if (input_conicSections[i].size() == 3)
            {
                numberOf_degenerateEquations++;

                degenerateEquation_number = i + 1;

                const std::vector<double> adjustedCoefficients
                {
                    0.0, 0.0, 0.0, input_conicSections[i][0], input_conicSections[i][1], input_conicSections[i][2] 
                };

                conicSections.push_back(adjustedCoefficients);
            }
            else
            {
                conicSections.push_back(input_conicSections[i]);
            }
        }
    }
    else
    {
        conicSections = input_conicSections;
    }

    if ((conicSections[0].size() != 6) || (conicSections[1].size() != 6))
    {
        return intersectionPoints;
    }

    for (int i = 0; i < 2; i++)
    {
        unsigned int max_orderOfMagnitudeFactor = 0;

        for (int j = 0; j < 6; j++)
        {
            if (std::fabs(conicSections[i][j]) < RS_TOLERANCE15)
            {
                conicSections[i][j] = 0.0;
            }

            if (conicSections[i][j] < 1.0)
            {
                const unsigned int orderOfMagnitudeFactor = (unsigned int) std::trunc(std::fabs(std::log10(conicSections[i][j]))) + 1;

                if (orderOfMagnitudeFactor > max_orderOfMagnitudeFactor)
                {
                    max_orderOfMagnitudeFactor = orderOfMagnitudeFactor;
                }
            }
        }

        for (int j = 0; j < 6; j++)
        {
            const double orderOfMagnitude = std::pow(10, max_orderOfMagnitudeFactor);

            conicSections[i][j] = std::trunc(conicSections[i][j] * orderOfMagnitude * 1.0E+10) * 1.0E-10;
        }
    }

    const Precise_Decimal a1 = conicSections[0][0];
    const Precise_Decimal b1 = conicSections[0][1];
    const Precise_Decimal c1 = conicSections[0][2];
    const Precise_Decimal d1 = conicSections[0][3];
    const Precise_Decimal e1 = conicSections[0][4];
    const Precise_Decimal f1 = conicSections[0][5];

    const Precise_Decimal a2 = conicSections[1][0];
    const Precise_Decimal b2 = conicSections[1][1];
    const Precise_Decimal c2 = conicSections[1][2];
    const Precise_Decimal d2 = conicSections[1][3];
    const Precise_Decimal e2 = conicSections[1][4];
    const Precise_Decimal f2 = conicSections[1][5];

    std::vector<double> rootsAbscissae;

    if (numberOf_degenerateEquations == 2)
    {
        Precise_Decimal denominatorTerm1 = (d1 * e2) - (d2 * e1);

        if (fabs(denominatorTerm1) < RS_TOLERANCE) return intersectionPoints;

        Precise_Decimal denominatorTerm2 = (d2 * e1) - (d1 * e2);

        Precise_Decimal numeratorTerm1  = (e1 * f2) - (e2 * f1);
        Precise_Decimal numeratorTerm2  = (d1 * f2) - (d2 * f1);

        intersectionPoints.push_back(

            RS_Vector( (numeratorTerm1 / denominatorTerm1).convert_to<double>(), 
                       (numeratorTerm2 / denominatorTerm2).convert_to<double>()) 

        );

        return intersectionPoints;
    }

    const Precise_Decimal a1_sq = a1 * a1;
    const Precise_Decimal b1_sq = b1 * b1;
    const Precise_Decimal c1_sq = c1 * c1;
    const Precise_Decimal d1_sq = d1 * d1;
    const Precise_Decimal e1_sq = e1 * e1;
    const Precise_Decimal f1_sq = f1 * f1;

    const Precise_Decimal a2_sq = a2 * a2;
    const Precise_Decimal b2_sq = b2 * b2;
    const Precise_Decimal c2_sq = c2 * c2;
    const Precise_Decimal d2_sq = d2 * d2;
    const Precise_Decimal e2_sq = e2 * e2;
    const Precise_Decimal f2_sq = f2 * f2;

    const Precise_Decimal c1_cu = c1_sq * c1;

    std::vector<double> x_n_terms(5, 0.0);

    /* The x^4 term. */
    x_n_terms[4] = ((a1_sq * c1 * c2_sq)  + (a2_sq * c1_cu)  - (2.0 * a1 * a2 * c1_sq * c2) 
                 + (a2 * b1_sq * c1 * c2) - (a2 * b1 * b2 * c1_sq)  
                 + (a1 * b2_sq * c1_sq)  - (a1 * b1 * b2 * c1 * c2)).convert_to<double>();

    /* The x^3 term. */
    x_n_terms[3] = ((2.0 * a1 * c1 * c2_sq * d1) + (2.0 * a2 * c1_cu * d2) - (2.0 * a1 * c1_sq * c2 * d2) - (2.0 * a2 * c1_sq * c2 * d1) 
                 + (2.0 * a2 * b1 * c1 * c2 * e1) + (b1_sq * c1 * c2 * d2) - (a2 * b1 * c1_sq * e2) - (a2 * b2 * c1_sq * e1) - (b1 * b2 * c1_sq * d2) 
                 + (2.0 * a1 * b2 * c1_sq * e2) + (b2_sq * c1_sq * d1) - (a1 * b1 * c1 * c2 * e2) - (a1 * b2 * c1 * c2 * e1) - (b1 * b2 * c1 * c2 * d1)).convert_to<double>();

    /* The x^2 term. */
    x_n_terms[2] = ((2.0 * a1 * c1 * c2_sq * f1) + (2.0 * a2 * c1_cu * f2) + (c1_cu * d2_sq)  + (c1 * c2_sq * d1_sq)  - (2.0 * a1 * c1_sq * c2 * f2) - (2.0 * a2 * c1_sq * c2 * f1) - (2.0 * c1_sq * c2 * d1 * d2) 
                 + (a2 * c1 * c2 * e1_sq)  + (2.0 * b1 * c1 * c2 * d2 * e1) + (b1_sq * c1 * c2 * f2) - (a2 * c1_sq * e1 * e2) - (b1 * b2 * c1_sq * f2) - (b1 * c1_sq * d2 * e2) - (b2 * c1_sq * d2 * e1) 
                 + (a1 * c1_sq * e2_sq)  + (b2_sq * c1_sq * f1) + (2.0 * b2 * c1_sq * d1 * e2) - (a1 * c1 * c2 * e1 * e2) - (b1 * b2 * c1 * c2 * f1) - (b1 * c1 * c2 * d1 * e2) - (b2 * c1 * c2 * d1 * e1)).convert_to<double>();

    /* The x term. */
    x_n_terms[1] = ((2.0 * c1_cu * d2 * f2) + (2.0 * c1 * c2_sq * d1 * f1) - (2.0 * c1_sq * c2 * d1 * f2) - (2.0 * c1_sq * c2 * d2 * f1) 
                 + (2.0 * b1 * c1 * c2 * e1 * f2) + (c1 * c2 * d2 * e1_sq)  - (b1 * c1_sq * e2 * f2) - (b2 * c1_sq * e1 * f2) - (c1_sq * d2 * e1 * e2) 
                 + (c1_sq * d1 * e2_sq)  + (2.0 * b2 * c1_sq * e2 * f1) - (b1 * c1 * c2 * e2 * f1) - (b2 * c1 * c2 * e1 * f1) - (c1 * c2 * d1 * e1 * e2)).convert_to<double>();

    /* The constant term. */
    x_n_terms[0] = ((c1_cu * f2_sq)  + (c1 * c2_sq * f1_sq)  - (2.0 * c1_sq * c2 * f1 * f2) 
                 + (c1 * c2 * e1_sq * f2) - (c1_sq * e1 * e2 * f2) 
                 + (c1_sq * e2_sq * f1) - (c1 * c2 * e1 * e2 * f1)).convert_to<double>();

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
    {
        DEBUG_HEADER
        std::cout << std::endl << std::endl 
                  << " (" << x_n_terms[4] << ")x^4 + " 
                  <<  "(" << x_n_terms[3] << ")x^3 + " 
                  <<  "(" << x_n_terms[2] << ")x^2 + " 
                  <<  "(" << x_n_terms[1] << ")x + " 
                  <<  "(" << x_n_terms[0] << ") = 0" 
                  << std::endl << std::endl;

        const double a = x_n_terms[4];
        const double b = x_n_terms[3];
        const double c = x_n_terms[2];
        const double d = x_n_terms[1];
        const double e = x_n_terms[0];

        const double a_sq = a * a;
        const double b_sq = b * b;
        const double c_sq = c * c;
        const double d_sq = d * d;
        const double e_sq = e * e;

        const double a_cu = a * a * a;
        const double b_cu = b * b * b;
        const double c_cu = c * c * c;
        const double d_cu = d * d * d;
        const double e_cu = e * e * e;

        std::cout << " Discriminant (Delta) = " 
                  << (256.0 * a_cu * e_cu) - (192.0 * a_sq * b * d * e_sq) - (128.0 * a_sq * c_sq * e_sq) 
                   + (144.0 * a_sq * c * d_sq * e) - (27.0 * a_sq * d_cu * d) + (144.0 * a * b_sq * c * e_sq) 
                   - (6.0 * a * b_sq * d_sq * e) - (80.0 * a * b * c_sq * d * e) + (18.0 * a * b * c * d_cu) 
                   + (16.0 * a * c_cu * c * e) - (4.0 * a * c_cu * d_sq) - (27.0 * b_cu * b * e_sq) 
                   + (18.0 * b_cu * c * d * e) - (4.0 * b_cu * d_cu) - (4.0 * b_sq * c_cu * e) + (b_sq * c_sq * d_sq) 
                  << std::endl << std::endl;

        std::cout << " P Factor = " 
                  << (8.0 * a * c) - (3.0 * b_sq) 
                  << std::endl << std::endl;

        std::cout << " D Factor = " 
                  << (64.0 * a_cu * e) - (16.0 * a_sq * c_sq) + (16.0 * a * b_sq * c) - (16.0 * a_sq * b * d) - (3 * b_cu * b) 
                  << std::endl << std::endl;
    }

    if (fabs(x_n_terms[4]) < RS_TOLERANCE)
    {
        if (fabs(x_n_terms[3]) < RS_TOLERANCE)
        {
            if (fabs(x_n_terms[2]) < RS_TOLERANCE)
            {
                if (fabs(x_n_terms[1]) < RS_TOLERANCE)
                {
                    return intersectionPoints;
                }
                else
                {
                    rootsAbscissae.push_back (-x_n_terms[0] / x_n_terms[1]);
                }
            }
            else
            {
                if (fabs(x_n_terms[1]) < RS_TOLERANCE)
                {
                    const double sqrtTerm { - x_n_terms[0] / x_n_terms[2] };

                    if (sqrtTerm < 0.0) return intersectionPoints;

                    rootsAbscissae.push_back ( std::sqrt(sqrtTerm));
                    rootsAbscissae.push_back (-std::sqrt(sqrtTerm));
                }
                else
                {
                    std::vector<double> quadraticCoefficients
                    {
                        x_n_terms[1] / x_n_terms[2], 
                        x_n_terms[0] / x_n_terms[2]
                    };

                    rootsAbscissae = quadraticSolver(quadraticCoefficients);
                }
            }
        }
        else
        {
            std::vector<double> cubicCoefficients
            {
                x_n_terms[2] / x_n_terms[3], 
                x_n_terms[1] / x_n_terms[3], 
                x_n_terms[0] / x_n_terms[3]
            };

            rootsAbscissae = cubicSolver(cubicCoefficients);
        }
    }
    else
    {
        rootsAbscissae = quarticSolverFull(x_n_terms);
    }

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
    {
        std::cout << " Number of roots = " << rootsAbscissae.size() 
                  << std::endl << std::endl;
    }

    for (double& rootX : rootsAbscissae)
    {
        const Precise_Decimal rootXPrecise = rootX;

        if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
        {
            std::cout << " RootX : " << rootX 
                      << std::endl << std::endl;
        }

        if (numberOf_degenerateEquations == 2)
        {
            if (e1 != 0)
            {
                intersectionPoints.push_back(RS_Vector(rootX, (-((d1 * rootXPrecise) + f1) / e1).convert_to<double>()));
            }
            else if (e2 != 0)
            {
                intersectionPoints.push_back(RS_Vector(rootX, (-((d2 * rootXPrecise) + f2) / e2).convert_to<double>()));
            }

            continue;
        }

        const Precise_Decimal sqrtTerm1 = boost::multiprecision::trunc(((((b1 * rootXPrecise) + e1) * ((b1 * rootXPrecise) + e1)) 
                                        - (4.0 * c1 * ((a1 * rootXPrecise * rootXPrecise) + (d1 * rootXPrecise) + f1))) * 1.0E+4) * 1.0E-4;

        const Precise_Decimal sqrtTerm2 = boost::multiprecision::trunc(((((b2 * rootXPrecise) + e2) * ((b2 * rootXPrecise) + e2)) 
                                        - (4.0 * c2 * ((a2 * rootXPrecise * rootXPrecise) + (d2 * rootXPrecise) + f2))) * 1.0E+4) * 1.0E-4;

        if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
        {
            std::cout << " RootX square root terms : " 
                      << sqrtTerm1 << ", " << sqrtTerm2 
                      << std::endl << std::endl;
        }

        if ((numberOf_degenerateEquations == 0) && ((sqrtTerm1 < 0.0) || (sqrtTerm2 < 0.0))) continue;

        const Precise_Decimal numeratorTerm1 = (-b1 * rootXPrecise) - e1;
        const Precise_Decimal numeratorTerm2 = (-b2 * rootXPrecise) - e2;

        const Precise_Decimal denominatorTerm1 = 2.0 * c1;
        const Precise_Decimal denominatorTerm2 = 2.0 * c2;

        if ((numberOf_degenerateEquations == 0) && ((denominatorTerm1 == 0.0) || (denominatorTerm2 == 0.0))) continue;

        const RS_Vector conic_1_points[2] = 
        {
            RS_Vector(rootX, ((numeratorTerm1 + boost::multiprecision::sqrt(sqrtTerm1)) / denominatorTerm1).convert_to<double>()), 
            RS_Vector(rootX, ((numeratorTerm1 - boost::multiprecision::sqrt(sqrtTerm1)) / denominatorTerm1).convert_to<double>()) 
        };

        const RS_Vector conic_2_points[2] = 
        {
            RS_Vector(rootX, ((numeratorTerm2 + boost::multiprecision::sqrt(sqrtTerm2)) / denominatorTerm2).convert_to<double>()), 
            RS_Vector(rootX, ((numeratorTerm2 - boost::multiprecision::sqrt(sqrtTerm2)) / denominatorTerm2).convert_to<double>()) 
        };

        if (numberOf_degenerateEquations != 0)
        {
            if (degenerateEquation_number != 1)
            {
                intersectionPoints.push_back(conic_1_points[0]);
                intersectionPoints.push_back(conic_1_points[1]);
            }
            else
            {
                intersectionPoints.push_back(conic_2_points[0]);
                intersectionPoints.push_back(conic_2_points[1]);
            }

            continue;
        }

        if (((fabs(conic_1_points[0].x - conic_2_points[0].x) < 1.0E-4) 
        &&   (fabs(conic_1_points[0].y - conic_2_points[0].y) < 1.0E-4)) 
        ||  ((fabs(conic_1_points[0].x - conic_2_points[1].x) < 1.0E-4) 
        &&   (fabs(conic_1_points[0].y - conic_2_points[1].y) < 1.0E-4)))
        {
            intersectionPoints.push_back(conic_1_points[0]);
        }


        if (((fabs(conic_1_points[1].x - conic_2_points[0].x) < 1.0E-4) 
        &&   (fabs(conic_1_points[1].y - conic_2_points[0].y) < 1.0E-4)) 
        ||  ((fabs(conic_1_points[1].x - conic_2_points[1].x) < 1.0E-4) 
        &&   (fabs(conic_1_points[1].y - conic_2_points[1].y) < 1.0E-4)))
        {
            intersectionPoints.push_back(conic_1_points[1]);
        }

        if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
        {
            std::cout << " Available roots : " << std::endl 
                      << " 1.1. " << conic_1_points[0] << std::endl 
                      << " 1.2. " << conic_1_points[1] << std::endl 
                      << " 2.1. " << conic_2_points[0] << std::endl 
                      << " 2.2. " << conic_2_points[1] << std::endl << std::endl;

            std::cout << " Chosen root : " << intersectionPoints [intersectionPoints.size() - 1] 
                      << std::endl << std::endl;
        }
    }

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL)
    {
        std::cout << " Number of intersection points = " << intersectionPoints.size() 
                  << std::endl << std::endl;
    }

    return intersectionPoints;
}
//EOF
