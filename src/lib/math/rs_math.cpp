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

#include <muParser.h>
#include "rs_math.h"

#include "rs_debug.h"


/**
 * Rounds the given double to the next int.
 */
int RS_Math::round(double v) {
    return (int) lrint(v);
    //return (v-floor(v)<0.5 ? (int)floor(v) : (int)ceil(v));
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



/**
 * Converts radians to degrees.
 */
double RS_Math::rad2deg(double a) {
    return (a/(2.0*M_PI)*360.0);
}



/**
 * Converts degrees to radians.
 */
double RS_Math::deg2rad(double a) {
    return ((a/360.0)*(2.0*M_PI));
}



/**
 * Converts radians to gradians.
 */
double RS_Math::rad2gra(double a) {
    return (a/(2.0*M_PI)*400.0);
}



/**
 * Finds greatest common divider using Euclid's algorithm.
 */
int RS_Math::findGCD(int a, int b) {
    int rem;

    while (b!=0) {
        rem = a % b;
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

//    bool ret = false;

    if (reversed) {
        double tmp = a1;
        a1 = a2;
        a2 = tmp;
    }
    if ( ( correctAngle(a2 -a1) >= correctAngle(a - a1) + RS_TOLERANCE_ANGLE &&
            correctAngle(a - a1) >= RS_TOLERANCE_ANGLE ) 
                    || isSameDirection(a1,a2,RS_TOLERANCE_ANGLE) ) { //if a1 and a2 overlap, take the range as whole 2 pi
        return true;
    } else {
        return false;
    }
}

//    if(a1>=a2-RS_TOLERENCE) {
//        if(a>=a1-RS_TOLERENCE || a<=a2+RS_TOLERENCE) {
//            ret = true;
//        }
//    } else {
//        if(a>=a1-RS_TOLERENCE && a<=a2+RS_TOLERENCE) {
//            ret = true;
//        }
//    }
//RS_DEBUG->print("angle %f is %sbetween %f and %f",
//                a, ret ? "" : "not ", a1, a2);
//    return ret;
//}



/**
 * Corrects the given angle to the range of 0-2*Pi.
 */
double RS_Math::correctAngle(double a) {
    return M_PI + remainder(a - M_PI, 2*M_PI);
}
//    while (a>2*M_PI)
//        a-=2*M_PI;
//    while (a<0)
//        a+=2*M_PI;
//    return a;
//}



/**
 * @return The angle that needs to be added to a1 to reach a2.
 *         Always positive and less than 2*pi.
 */
double RS_Math::getAngleDifference(double a1, double a2) {
    double ret;
    ret=M_PI + remainder(a2 -a1 -M_PI, 2*M_PI);

//    if (a1>=a2) {
//        a2+=2*M_PI;
//    }
//    ret = a2-a1;

    if (ret>=2*M_PI) {
        ret=0.0;
    }

    return ret;
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

    double ret;

    bool cor = isAngleReadable(angle) ^ readable;

    // quadrant 1 & 4
    if (!cor) {
        ret = angle;
    }
    // quadrant 2 & 3
    else {
        ret = angle+M_PI;
    }

    if (corrected!=NULL) {
        *corrected = cor;
    }

    return ret;
}


/**
 * @return true: if the given angle is in a range that is readable
 * for texts created with that angle.
 */
bool RS_Math::isAngleReadable(double angle) {
    if (angle>M_PI/2.0*3.0+0.001 ||
            angle<M_PI/2.0+0.001) {
        return true;
    } else {
        return false;
    }
}



/**
 * @param tol Tolerance in rad.
 * @retval true The two angles point in the same direction.
 */
bool RS_Math::isSameDirection(double dir1, double dir2, double tol) {
    double diff = fabs(dir1-dir2);
    if (diff<tol || diff>2*M_PI-tol) {
        //std::cout << "RS_Math::isSameDirection: " << dir1 << " and " << dir2
        //	<< " point in the same direction" << "\n";
        return true;
    }
    else {
        //std::cout << "RS_Math::isSameDirection: " << dir1 << " and " << dir2
        //	<< " don't point in the same direction" << "\n";
        return false;
    }
}


/**
 * Compares two double values with a tolerance.
 */
bool RS_Math::cmpDouble(double v1, double v2, double tol) {
    return (fabs(v2-v1)<tol);
}



/**
 * Evaluates a mathematical expression and returns the result.
 * If an error occured, the given default value 'def' will be returned.
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
 * If an error occured, ok will be set to false (if ok isn't NULL).
 */
//double RS_Math::eval(const QString& expr, bool* ok);


/**
 * Converts a double into a string which is as short as possible
 *
 * @param value The double value
 * @param prec Precision e.g. a precision of 1 would mean that a
 *     value of 2.12030 will be converted to "2.1". 2.000 is always just "2").
 */
QString RS_Math::doubleToString(double value, double prec) {
    if (prec< RS_TOLERANCE ) {
        std::cerr << "RS_Math::doubleToString: invalid precision\n";
        return "";
    }

    QString ret;
    QString exaStr;
    int dotPos;
    int num = RS_Math::round(value / prec);

    exaStr = RS_Math::doubleToString(prec, 10);
    dotPos = exaStr.indexOf('.');

    if (dotPos==-1) {
        ret.sprintf("%d", RS_Math::round(num*prec));
    } else {
        int digits = exaStr.length() - dotPos - 1;
        ret = RS_Math::doubleToString(num*prec, digits);
    }

    return ret;
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
        // Remove zeros at the end:
        while (valStr.at(valStr.length()-1)=='0') {
            valStr.truncate(valStr.length()-1);
        }

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
    QString s;
    double v;

    std::cout << "RS_Math::test: doubleToString:\n";

    v = 0.1;
    s = RS_Math::doubleToString(v, 0.1);
    assert(s=="0.1");
    s = RS_Math::doubleToString(v, 0.01);
    assert(s=="0.1");
    s = RS_Math::doubleToString(v, 0.0);
    assert(s=="0");

    v = 0.01;
    s = RS_Math::doubleToString(v, 0.1);
    assert(s=="0");
    s = RS_Math::doubleToString(v, 0.01);
    assert(s=="0.01");
    s = RS_Math::doubleToString(v, 0.0);
    assert(s=="0");

    v = 0.001;
    s = RS_Math::doubleToString(v, 0.1);
    assert(s=="0");
    s = RS_Math::doubleToString(v, 0.01);
    assert(s=="0");
    s = RS_Math::doubleToString(v, 0.001);
    assert(s=="0.001");
    s = RS_Math::doubleToString(v, 0.0);
    assert(s=="0");

    std::cout << "RS_Math::test: complete\n";
}
//quadratic, cubic, and quartic equation solver
// ce[] contains coefficent of the cubic equation:
// roots[] pointed to a list of real roots
// solvers assume arguments are valid, and there's no attempt to verify validity of the argument pointers
//
unsigned int RS_Math::quadraticSolver(double * ce,  double * roots)
//quadratic solver for
// x^2 + ce[0] x + ce[2] =0
{
    double discriminant=0.25*ce[0]*ce[0]-ce[1];
    if (discriminant < 0. ) return 0;
    roots[0]= -0.5*ce[0] + sqrt(discriminant);
    roots[1]= -ce[0] - roots[0];
    return 2;
}

unsigned int RS_Math::cubicSolver(double * ce, double *roots)
//cubic equation solver
// x^3 + ce[0] x^2 + ce[1] x + ce[2] = 0
{
    // depressed cubic, Tschirnhaus transformation, x= t - b/(3a)
    // t^3 + p t +q =0
    unsigned int ret=0;
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
        ret=1;
        *roots=(q>0)?-pow(q,(1./3)):pow(-q,(1./3));
        *roots -= shift;
        return ret;
    }
    //std::cout<<"discriminant="<<discriminant<<std::endl;
    if(discriminant>0) {
        double ce2[2]= {q, -1./27*p*p*p},u3[2];
        ret=quadraticSolver(ce2,u3);
        if (! ret ) { //should not happen
            std::cerr<<"cubicSolver()::Error cubicSolver("<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<")\n";
        }
        ret=1;
        double u,v;
        u= (q<=0) ? pow(u3[0], 1./3): -pow(-u3[1],1./3);
        //u=(q<=0)?pow(-0.5*q+sqrt(discriminant),1./3):-pow(0.5*q+sqrt(discriminant),1./3);
        v=(-1./3)*p/u;
        //std::cout<<"u="<<u<<"\tv="<<v<<std::endl;
        //std::cout<<"u^3="<<u*u*u<<"\tv^3="<<v*v*v<<std::endl;
        *roots=u+v - shift;
        return ret;
    }
    ret=3;
    std::complex<double> u(q,0),rt[3];
    u=std::pow(-0.5*u-sqrt(0.25*u*u+p*p*p/27),1./3);
    rt[0]=u-p/(3.*u)-shift;
    std::complex<double> w(-0.5,sqrt(3.)/2);
    rt[1]=u*w-p/(3.*u*w)-shift;
    rt[2]=u/w-p*w/(3.*u)-shift;
//	std::cout<<"Roots:\n";
//	std::cout<<rt[0]<<std::endl;
//	std::cout<<rt[1]<<std::endl;
//	std::cout<<rt[2]<<std::endl;

    roots[0]=rt[0].real();
    roots[1]=rt[1].real();
    roots[2]=rt[2].real();
    return ret;
}

unsigned int RS_Math::quarticSolver(double * ce, double *roots)
//quartic solver
// x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
{
    // x^4 + a x^3 + b x^2 +c x + d = 0
    // depressed quartic, x= t - a/4
    // t^4 + ( b - 3/8 a^2 ) t^2 + (c - a b/2 + a^3/8) t + d - a c /4 + a^2 b/16 - 3 a^4/256 =0
    // t^4 + p t^2 + q t + r =0
    // p= b - (3./8)*a*a;
    // q= c - 0.5*a*b+(1./8)*a*a*a;
    // r= d - 0.25*a*c+(1./16)*a*a*b-(3./256)*a^4
    unsigned int ret=0;
    double shift=0.25*ce[0];
    double shift2=shift*shift;
    double a2=ce[0]*ce[0];
    double p= ce[1] - (3./8)*a2;
    double q= ce[2] + ce[0]*((1./8)*a2 - 0.5*ce[1]);
    double r= ce[3] - shift*ce[2] + (ce[1] - 3.*shift2)*shift2;
    //std::cout<<"quartic_solver:: p="<<p<<"\tq="<<q<<"\tr="<<r<<std::endl;
    if (fabs(q) <= RS_TOLERANCE) {// Biquadratic equations
        double discriminant= 0.25*p*p -r;
        if (discriminant < 0.) {
            return 0;
        }
        double t2[2];
        t2[0]=-0.5*p-sqrt(discriminant);
        t2[1]= -p - t2[0];
        if ( t2[0] >= 0. ) {// four real roots
            roots[0]=sqrt(t2[0])-shift;
            roots[1]= -roots[0]-shift;
            roots[2]=sqrt(t2[1])-shift;
            roots[3]= -roots[2]-shift;
            return 4;
        }
        if ( t2[1] >= 0.) { // two real roots
            roots[0]=sqrt(t2[1])-shift;
            roots[1]= -roots[0]-shift;
            return 2;
        }
        return 0;
    }
    if ( fabs(r)< 1.0e-75 ) {
        double cubic[3]= {0.,p,q};
        roots[0]=0.;
        ret=1+cubicSolver(cubic,roots+1);
        for(unsigned int i=0; i<ret; i++) roots[i] -= shift;
        return ret;
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
    double cubic[3]= {2.*p,p*p-4.*r,-q*q},croots[3];
    ret = cubicSolver(cubic,croots);
    //std::cout<<"quartic_solver:: real roots from cubic: "<<ret<<std::endl;
    //for(unsigned int i=0; i<ret; i++)
    //   std::cout<<"cubic["<<i<<"]="<<cubic[i]<<" x= "<<croots[i]<<std::endl;
    if (ret==1) { //one real root from cubic
        if (croots[0]< 0.) {//this should not happen
            std::cerr<<"Quartic Error:: Found one real root for cubic, but negative\n";
            return 0;
        }
        double sqrtz0=sqrt(croots[0]);
        double ce2[2];
        ce2[0]=	-sqrtz0;
        ce2[1]=0.5*(p+croots[0])+0.5*q/sqrtz0;
        ret=quadraticSolver(ce2,roots);
        if (! ret ) {
            ce2[0]=	sqrtz0;
            ce2[1]=0.5*(p+croots[0])-0.5*q/sqrtz0;
            ret=quadraticSolver(ce2,roots);
        }
        ret=2;
        for(unsigned int i=0; i<ret; i++) roots[i] -= shift;
        return ret;
    }
    if ( croots[0]> 0. && croots[1] > 0. ) {
        double sqrtz0=sqrt(croots[0]);
        double ce2[2];
        ce2[0]=	-sqrtz0;
        ce2[1]=0.5*(p+croots[0])+0.5*q/sqrtz0;
        ret=quadraticSolver(ce2,roots);
        ce2[0]=	sqrtz0;
        ce2[1]=0.5*(p+croots[0])-0.5*q/sqrtz0;
        ret=quadraticSolver(ce2,roots+2);
        ret=4;
        for(unsigned int i=0; i<ret; i++) roots[i] -= shift;
        return ret;
    }
    return 0;
}


/**
 * Evaluates a mathematical expression and returns the result.
 * If an error occured, ok will be set to false (if ok isn't NULL).
 */
double RS_Math::eval(const QString& expr, bool* ok) {
    bool okTmp(false);
    if(ok==NULL) ok=&okTmp;
    if (expr.isEmpty()) {
        *ok = false;
        return 0.0;
    }
    double ret(0.);
    try{
        mu::Parser p;
        p.DefineConst("pi",M_PI);
        p.SetExpr(expr.toStdString());
        ret=p.Eval();
        *ok=true;
    }
    catch (mu::Parser::exception_type &e)
      {
        std::cout << e.GetMsg() << std::endl;
        *ok=false;
      }
    return ret;
}
