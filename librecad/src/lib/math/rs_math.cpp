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
#ifdef  HAS_BOOST
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions/ellint_2.hpp>
#endif

#include <muParser.h>
#include "rs_math.h"

#include "rs_debug.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/**
 * Rounds the given double to the closest int.
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

/* pow of vector components */
RS_Vector RS_Math::pow(RS_Vector vp, double y) {
        return RS_Vector(pow(vp.x,y),pow(vp.y,y));
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

    if (reversed) swap(a1,a2);
    if ( ( correctAngle(a2 -a1) >= correctAngle(a - a1) + RS_TOLERANCE_ANGLE &&
            correctAngle(a - a1) >= RS_TOLERANCE_ANGLE  ) || fabs( remainder(correctAngle(a2 - a1 ) , 2.*M_PI)) < RS_TOLERANCE_ANGLE) {
            // the |a2-a1| % (2 pi)=0 means the whole angular range
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



//Equation solvers

// quadratic, cubic, and quartic equation solver
// @ ce[] contains coefficent of the cubic equation:
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
    if(ce.size() != 2) return ans;
    double discriminant=0.25*ce[0]*ce[0]-ce[1];
    if (discriminant >= 0.){
        ans.push_back(-0.5*ce[0] + sqrt(discriminant));
        ans.push_back(-ce[0] - ans[0]);
    }
    return ans;
}

std::vector<double> RS_Math::cubicSolver(const std::vector<double>& ce)
//cubic equation solver
// x^3 + ce[0] x^2 + ce[1] x + ce[2] = 0
{
    std::vector<double> ans(0,0.);
    if(ce.size() != 3) return ans;
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
        return ans;
    }
    //std::cout<<"discriminant="<<discriminant<<std::endl;
    if(discriminant>0) {
        std::vector<double> ce2(2,0.);
        ce2[0]=q;
        ce2[1]=-1./27*p*p*p;
        auto&& r=quadraticSolver(ce2);
        if ( r.size()==0 ) { //should not happen
            std::cerr<<__FILE__<<" : "<<__FUNCTION__<<" : line"<<__LINE__<<" :cubicSolver()::Error cubicSolver("<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<")\n";
        }
        double u,v;
        u= (q<=0) ? pow(r[0], 1./3): -pow(-r[1],1./3);
        //u=(q<=0)?pow(-0.5*q+sqrt(discriminant),1./3):-pow(0.5*q+sqrt(discriminant),1./3);
        v=(-1./3)*p/u;
        //std::cout<<"u="<<u<<"\tv="<<v<<std::endl;
        //std::cout<<"u^3="<<u*u*u<<"\tv^3="<<v*v*v<<std::endl;
        ans.push_back(u+v - shift);
        return ans;
    }
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
    ans.push_back(rt[0].real());
    ans.push_back(rt[1].real());
    ans.push_back(rt[2].real());

    return ans;
}

std::vector<double> RS_Math::quarticSolver(const std::vector<double>& ce)
//quartic solver
// x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
{
    std::vector<double> ans(0,0.);
    if(ce.size() != 4) return ans;
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
//    std::cout<<"quartic_solver:: p="<<p<<"\tq="<<q<<"\tr="<<r<<std::endl;
    if (fabs(q) <= RS_TOLERANCE) {// Biquadratic equations
        double discriminant= 0.25*p*p -r;
        if (discriminant < 0.) {
            return ans;
        }
        double t2[2];
        t2[0]=-0.5*p-sqrt(discriminant);
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
        return ans;
    }
    if ( fabs(r)< 1.0e-75 ) {
        std::vector<double> cubic(3,0.);
        cubic[1]=p;
        cubic[2]=q;
        ans.push_back(0.);
        auto&& r=cubicSolver(cubic);
        std::copy(r.begin(),r.end(),back_inserter(ans));
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
    auto&& r3= cubicSolver(cubic);
    //std::cout<<"quartic_solver:: real roots from cubic: "<<ret<<std::endl;
    //for(unsigned int i=0; i<ret; i++)
    //   std::cout<<"cubic["<<i<<"]="<<cubic[i]<<" x= "<<croots[i]<<std::endl;
    if (r3.size()==1) { //one real root from cubic
        if (r3[0]< 0.) {//this should not happen
            std::cerr<<__FILE__<<" : "<<__FUNCTION__<<" : line "<<__LINE__<<std::endl;
            std::cerr<<"Quartic Error:: Found one real root for cubic, but negative\n";
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
        for(size_t i=0; i<r1.size(); i++) r1[i] -= shift;
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
        auto&& r1=quadraticSolver(ce2);
        std::copy(r1.begin(),r1.end(),back_inserter(ans));
        for(size_t i=0; i<ans.size(); i++) ans[i] -= shift;
        return ans;
    }
    return ans;
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

bool RS_Math::linearSolver(const QVector<QVector<double> >& mt, QVector<double>& sn){
    //verify the matrix size
    int mSize(mt.size()); //rows
    int aSize(mSize+1); //columns of augmented matrix
    for(int i=0;i<mSize;i++) {
        if(mt[i].size() != aSize ) return false;
    }
    sn.resize(mSize);//to hold the solution
//#ifdef	HAS_BOOST
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
        std::cout<<__FILE__<<" : "<<__FUNCTION__<<" : line "<<__LINE__<<std::endl;
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
    //    if(fabs(dn(0))<RS_TOLERANCE*RS_TOLERANCE
    //            ||fabs(dn(2))<RS_TOLERANCE*RS_TOLERANCE
    //            ||d/dn(0)<RS_TOLERANCE*RS_TOLERANCE
    //            ||d/dn(2)<RS_TOLERANCE*RS_TOLERANCE
    //            ) {
    //        //ellipse not defined
    //        return false;
    //    }
    //    d=sqrt(d/dn(0));
    //    data.majorP.set(d,0.);
    //    data.ratio=sqrt(dn(0)/dn(2));
#else
    // solve the linear equation by Gauss-Jordan elimination
    QVector<QVector<double> > mt0(mt); //copy the matrix;
    for(int i=0;i<mSize;i++){
        int imax(i);
        double cmax(fabs(mt0[i][i]));
        for(int j=i+1;j<mSize;j++) {
            if(fabs(mt0[j][i]) > cmax ) {
                imax=j;
                cmax=fabs(mt0[j][i]);
            }
        }
        if(cmax<RS_TOLERANCE*RS_TOLERANCE) return false; //singular matrix
        if(imax != i) {//move the line with largest absolute value at column i to row i, to avoid division by zero
            std::swap(mt0[i],mt0[imax]);
            //            for(int j=i;j<=mSize;j++) {
            //                std::swap(m[i][j],m[imax][j]);
            //            }
        }
        //        for(int k=i+1;k<5;k++) { //normalize the i-th row
        for(int k=mSize;k>=i;k--) { //normalize the i-th row
            mt0[i][k] /= mt0[i][i];
        }
        for(int j=0;j<mSize;j++) {//Gauss-Jordan
            if(j != i ) {
                //                for(int k=i+1;k<5;k++) {
                for(int k=mSize;k>=i;k--) {
                    mt0[j][k] -= mt0[i][k]*mt0[j][i];
                }
            }
        }
        //output gauss-jordan results for debugging
        //        std::cout<<"========"<<i<<"==========\n";
        //        for(int j=0;j<mSize;j++) {//Gauss-Jordan
        //            for(int k=0;k<=mSize;k++) {
        //                std::cout<<m[j][k]<<'\t';
        //            }
        //            std::cout<<std::endl;
        //        }
    }
    for(int i=0;i<mSize;i++) {
        sn[i]=mt0[i][mSize];
    }
#endif

    return true;
}

/**
 * wrapper of elliptic integral of the second type, Legendre form
 *@k the elliptic modulus or eccentricity
 *@phi elliptic angle, must be within range of [0, M_PI]
 *
 *Author: Dongxu Li
 */
double RS_Math::ellipticIntegral_2(const double& k, const double& phi)
{
    double a= remainder(phi-M_PI/2.,M_PI);
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
    const double& ma000= m[0];
    const double& ma011= m[1];
    const double& ma100= m[2];
    const double& ma101= m[3];
    const double& ma111= m[4];
    const double& mb10 = m[5];
    const double& mb11 = m[6];
    const double& mc1 = m[7];

//    std::cout<<__FILE__<<" :  line "<<__LINE__<<" "<<__FUNCTION__<<std::endl;
//    std::cout<<"simplified e1: "<<ma000<<"*x^2 + "<<ma011<<"*y^2 -1 =0\n";
//    std::cout<<"simplified e2: "<<ma100<<"*x^2 + 2*("<<ma101<<")*x*y + "<<ma111<<"*y^2 "<<" + ("<<mb10<<")*x + ("<<mb11<<")*y + ("<<mc1<<") =0\n";
    // construct the Bezout determinant
    double v0=2.*ma000*ma101;
    double v2=ma000*mb10;
    double v3=ma000*mb11;
    double v4=ma000*mc1+ma100;
    //double v5= 2.*ma101*ma011;
    //double v6= ma000*ma111;
    //double v7= 2.*ma101;
    double v8= 2.*ma011*mb10;
    //double v9= ma100*ma011;
    double v1=ma000*ma111-ma100*ma011;
    //double v1= v6 - v9;
    double u0 = v4*v4-v2*mb10;
    double u1 = 2.*(v3*v4-v0*mb10);
    double u2 = 2.*(v4*v1-ma101*v0)+v3*v3+0.5*v2*v8;
    double u3 = v0*v8+2.*v3*v1;
    double u4 = v1*v1+2.*ma101*ma011*v0;
    //std::cout<<"u0="<<u0<<"\tu1="<<u1<<"\tu2="<<u2<<"\tu3="<<u3<<"\tu4="<<u4<<std::endl;
    //std::cout<<"("<<u4<<")*x^4+("<<u3<<")*x^3+("<<u2<<")*x^2+("<<u1<<")*x+("<<u0<<")=0\n";
    std::vector<double> ce(4,0.);
    std::vector<double> roots(0,0.);

    if ( fabs(u4) < 1.0e-75) { // this should not happen
        if ( fabs(u3) < 1.0e-75) { // this should not happen
            if ( fabs(u2) < 1.0e-75) { // this should not happen
                if( fabs(u1) > 1.0e-75) {
                    roots.push_back(-u0/u1);
                } else { // can not determine y. this means overlapped, but overlap should have been detected before, therefore return empty set
                    return ret;
                }
            } else {
                ce.resize(2);
                ce[0]=u1/u2;
                ce[1]=u0/u2;
                //std::cout<<"ce[2]={ "<<ce[0]<<' '<<ce[1]<<" }\n";
                roots=RS_Math::quadraticSolver(ce);
            }
        } else {
            ce.resize(3);
            ce[0]=u2/u3;
            ce[1]=u1/u3;
            ce[2]=u0/u3;
            //std::cout<<"ce[3]={ "<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<" }\n";
            roots=RS_Math::cubicSolver(ce);
        }
    } else {
        ce[0]=u3/u4;
        ce[1]=u2/u4;
        ce[2]=u1/u4;
        ce[3]=u0/u4;
        //std::cout<<"ce[4]={ "<<ce[0]<<' '<<ce[1]<<' '<<ce[2]<<' '<<ce[3]<<" }\n";
        roots=RS_Math::quarticSolver(ce);
    }
//	std::cout<<"Equation for y: y^4";
//        for(int i=3; i>=0; i--) {
//		std::cout<<"+("<<ce[3-i]<<")";
//	    if ( i ) {
//		    std::cout<<"*y^"<<i;
//	    }else {
//		    std::cout<<" ==0\n";
//	    }
//    }

    if (roots.size()==0 ) { // no intersection found
        return ret;
    }
//      std::cout<<"counts="<<counts<<": ";
//	for(unsigned int i=0;i<counts;i++){
//	std::cout<<roots[i]<<" ";
//	}
//	std::cout<<std::endl;
//    RS_VectorSolutions vs0;
    unsigned int ivs0=0;
    double a1=sqrt(1./ma000);
    for(size_t i=0; i<roots.size(); i++) {
        double y=roots[i];
        //double x=(ma100*(ma011*y*y-1.)-ma000*(ma111*y*y+mb11*y+mc1))/(ma000*(2.*ma101*y+mb11));
        double x,d=v0*y+v2;
//        std::cout<<"d= "<<d<<std::endl;
        if( fabs(d)>10.*RS_TOLERANCE*sqrt(RS_TOLERANCE)) {//whether there's x^1 term in bezout determinant
            x=-((v1*y+v3)*y+v4 )/d;
            if(ret.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                ret.push_back(RS_Vector(x,y));
        } else { // no x^1 term, have to use x^2 term, then, have to check plus/minus sqrt
            x=a1*sqrt(1-y*y*ma011);
            if(ret.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                ret.push_back(RS_Vector(x,y));
            x=-x;
            if(ret.getClosestDistance(RS_Vector(x,y),ivs0)>RS_TOLERANCE)
                ret.push_back(RS_Vector(x,y));
        }
        //std::cout<<"eq1="<<ma000*x*x+ma011*y*y-1.<<std::endl;
        //std::cout<<"eq2="<<ma100*x*x + 2.*ma101*x*y+ma111*y*y+mb10*x+mb11*y+mc1<<std::endl;
//            if (
//                fabs(ma100*x*x + 2.*ma101*x*y+ma111*y*y+mb10*x+mb11*y+mc1)< RS_TOLERANCE
//            ) {//found
//                vs0.set(ivs0++, RS_Vector(x,y));
//            }
    }
//    for(unsigned int j=0; j<vs0.getNumber(); j++) {
//        std::cout<<" ( "<<vs0.get(j).x<<" , "<<vs0.get(j).y<<" ) ";
//    }
//    std::cout<<std::endl;
//    std::cout<<"counts= "<<counts<<"\tFound "<<ivs0<<" EllipseEllipse intersections\n";
    //ret.alloc(ivs0);
    return ret;
}
//EOF
