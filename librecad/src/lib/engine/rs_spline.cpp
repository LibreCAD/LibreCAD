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


#include "rs_spline.h"


#include "rs_line.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"


RS_SplineData::RS_SplineData(int _degree, bool _closed):
	degree(_degree)
  ,closed(_closed)
{
}

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld) {
	os << "( degree: " << ld.degree <<
		  " closed: " << ld.closed <<
		  ")";
	return os;
}

/**
 * Constructor.
 */
RS_Spline::RS_Spline(RS_EntityContainer* parent,
                     const RS_SplineData& d)
        :RS_EntityContainer(parent), data(d) {
    calculateBorders();
}

RS_Entity* RS_Spline::clone() const{
    RS_Spline* l = new RS_Spline(*this);
    l->setOwner(isOwner());
    l->initId();
    l->detach();
    return l;
}



void RS_Spline::calculateBorders() {
    /*minV = RS_Vector::minimum(data.startpoint, data.endpoint);
    maxV = RS_Vector::maximum(data.startpoint, data.endpoint);

    QList<RS_Vector>::iterator it;
    for (it = data.controlPoints.begin();
    it!=data.controlPoints.end(); ++it) {

    minV = RS_Vector::minimum(*it, minV);
    maxV = RS_Vector::maximum(*it, maxV);
}
    */
}


void RS_Spline::setDegree(size_t deg) {
	if (deg>=1 && deg<=3) {
		data.degree = deg;
	}
}

/** @return Degree of this spline curve (1-3).*/
size_t RS_Spline::getDegree() const{
	return data.degree;
}

size_t RS_Spline::getNumberOfControlPoints() const {
	return data.controlPoints.size();
}

/**
 * @retval true if the spline is closed.
 * @retval false otherwise.
 */
bool RS_Spline::isClosed() const {
		return data.closed;
}

/**
 * Sets the closed falg of this spline.
 */
void RS_Spline::setClosed(bool c) {
		data.closed = c;
		update();
}

RS_VectorSolutions RS_Spline::getRefPoints() const
{
	return RS_VectorSolutions(data.controlPoints);
}

RS_Vector RS_Spline::getNearestRef( const RS_Vector& coord,
                                    double* dist /*= nullptr*/) const
{
    // override the RS_EntityContainer methode
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Spline::getNearestSelectedRef( const RS_Vector& coord,
                                            double* dist /*= nullptr*/) const
{
    // override the RS_EntityContainer methode
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestSelectedRef(coord, dist);
}

/**
 * Updates the internal polygon of this spline. Called when the
 * spline or it's data, position, .. changes.
 */
void RS_Spline::update() {

    RS_DEBUG->print("RS_Spline::update");

    clear();

    if (isUndone()) {
        return;
    }

    if (data.degree<1 || data.degree>3) {
        RS_DEBUG->print("RS_Spline::update: invalid degree: %d", data.degree);
        return;
    }

    if (data.controlPoints.size() < data.degree+1) {
        RS_DEBUG->print("RS_Spline::update: not enough control points");
        return;
    }

    resetBorders();

	std::vector<RS_Vector> tControlPoints = data.controlPoints;

    if (data.closed) {
		for (size_t i=0; i<data.degree; ++i) {
			tControlPoints.push_back(data.controlPoints.at(i));
        }
    }

	size_t i;
	const size_t npts = tControlPoints.size();
    // order:
	const size_t  k = data.degree+1;
    // resolution:
	const size_t  p1 = getGraphicVariableInt("$SPLINESEGS", 8) * npts;

	std::vector<double> b(npts*3+2, 0.);
	std::vector<double> h(npts+2, 1.);
	std::vector<double> p(3*p1+2, 0.);

    i = 1;
	for(auto const& vp: tControlPoints){
		b[i] = vp.x;
		b[i+1] = vp.y;

        RS_DEBUG->print("RS_Spline::update: b[%d]: %f/%f", i, b[i], b[i+1]);
        i+=3;
    }

    if (data.closed) {
        rbsplinu(npts,k,p1,b,h,p);
    } else {
        rbspline(npts,k,p1,b,h,p);
    }

    RS_Vector prev(false);
	for (i = 1; i <= 3*p1; i += 3) {
        if (prev.valid) {
			RS_Line* line = new RS_Line{this, prev, {p[i], p[i+1]}};
			line->setLayer(nullptr);
			line->setPen(RS2::FlagInvalid);
            addEntity(line);
        }
        prev = RS_Vector(p[i], p[i+1]);

        minV = RS_Vector::minimum(prev, minV);
        maxV = RS_Vector::maximum(prev, maxV);
	}
}

RS_Vector RS_Spline::getStartpoint() const {
   if (data.closed) return RS_Vector(false);
   return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->firstEntity())->getStartpoint();
}

RS_Vector RS_Spline::getEndpoint() const {
   if (data.closed) return RS_Vector(false);
   return static_cast<RS_Line*>(const_cast<RS_Spline*>(this)->lastEntity())->getEndpoint();
}


RS_Vector RS_Spline::getNearestEndpoint(const RS_Vector& coord,
                                        double* dist)const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector ret(false);
    if(! data.closed) { // no endpoint for closed spline
       RS_Vector vp1(getStartpoint());
       RS_Vector vp2(getEndpoint());
       double d1( (coord-vp1).squared());
       double d2( (coord-vp2).squared());
       if( d1<d2){
           ret=vp1;
           minDist=sqrt(d1);
       }else{
           ret=vp2;
           minDist=sqrt(d2);
       }
//        for (int i=0; i<data.controlPoints.count(); i++) {
//            d = (data.controlPoints.at(i)).distanceTo(coord);

//            if (d<minDist) {
//                minDist = d;
//                ret = data.controlPoints.at(i);
//            }
//        }
    }
	if (dist) {
        *dist = minDist;
    }
    return ret;
}



/*
// The default implementation of RS_EntityContainer is inaccurate but
//   has to do for now..
RS_Vector RS_Spline::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) {
}
*/



RS_Vector RS_Spline::getNearestCenter(const RS_Vector& /*coord*/,
									  double* dist) const{

	if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}



RS_Vector RS_Spline::getNearestMiddle(const RS_Vector& /*coord*/,
                                      double* dist,
                                      int /*middlePoints*/)const {
	if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}



RS_Vector RS_Spline::getNearestDist(double /*distance*/,
                                    const RS_Vector& /*coord*/,
									double* dist) const{
	if (dist) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}



void RS_Spline::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
	for (RS_Vector& vp: data.controlPoints) {
		vp.move(offset);
    }
//    update();
}



void RS_Spline::rotate(const RS_Vector& center, const double& angle) {
    rotate(center,RS_Vector(angle));
}



void RS_Spline::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
	RS_EntityContainer::rotate(center, angleVector);
	for (RS_Vector& vp: data.controlPoints) {
		vp.rotate(center, angleVector);
	}
//    update();
}

void RS_Spline::scale(const RS_Vector& center, const RS_Vector& factor) {
	for (RS_Vector& vp: data.controlPoints) {
		vp.scale(center, factor);
	}

    update();
}



void RS_Spline::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
	for (RS_Vector& vp: data.controlPoints) {
		vp.mirror(axisPoint1, axisPoint2);
	}

	update();
}



void RS_Spline::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
	for (RS_Vector& vp: data.controlPoints) {
		if (ref.distanceTo(vp)<1.0e-4) {
			vp.move(offset);
		}
	}

    update();
}

void RS_Spline::revertDirection() {
	std::reverse(data.controlPoints.begin(), data.controlPoints.end());
}




void RS_Spline::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/) {

	if (!(painter && view)) {
        return;
    }


    RS_Entity* e=firstEntity(RS2::ResolveNone);
	if (e) {
        RS_Pen p=this->getPen(true);
        e->setPen(p);
        double patternOffset(0.0);
        view->drawEntity(painter, e, patternOffset);
        //RS_DEBUG->print("offset: %f\nlength was: %f", offset, e->getLength());

        e = nextEntity(RS2::ResolveNone);
		while(e) {
            view->drawEntityPlain(painter, e, patternOffset);
            e = nextEntity(RS2::ResolveNone);
            //RS_DEBUG->print("offset: %f\nlength was: %f", offset, e->getLength());
        }
    }
}



/**
 * Todo: draw the spline, user patterns.
 */
/*
void RS_Spline::draw(RS_Painter* painter, RS_GraphicView* view) {
   if (!(painter && view)) {
       return;
   }

   / *
      if (data.controlPoints.count()>0) {
          RS_Vector prev(false);
          QList<RS_Vector>::iterator it;
          for (it = data.controlPoints.begin(); it!=data.controlPoints.end(); ++it) {
              if (prev.valid) {
                  painter->drawLine(view->toGui(prev),
                                    view->toGui(*it));
              }
              prev = (*it);
          }
      }
   * /

   int i;
   int npts = data.controlPoints.count();
   // order:
   int k = 4;
   // resolution:
   int p1 = 100;

   double* b = new double[npts*3+1];
   double* h = new double[npts+1];
   double* p = new double[p1*3+1];

   QList<RS_Vector>::iterator it;
   i = 1;
   for (it = data.controlPoints.begin(); it!=data.controlPoints.end(); ++it) {
       b[i] = (*it).x;
       b[i+1] = (*it).y;
       b[i+2] = 0.0;

        RS_DEBUG->print("RS_Spline::draw: b[%d]: %f/%f", i, b[i], b[i+1]);
        i+=3;
   }

   // set all homogeneous weighting factors to 1.0
   for (i=1; i <= npts; i++) {
       h[i] = 1.0;
   }

   //
   for (i = 1; i <= 3*p1; i++) {
       p[i] = 0.0;
   }

   rbspline(npts,k,p1,b,h,p);

   RS_Vector prev(false);
   for (i = 1; i <= 3*p1; i=i+3) {
       if (prev.valid) {
           painter->drawLine(view->toGui(prev),
                             view->toGui(RS_Vector(p[i], p[i+1])));
       }
       prev = RS_Vector(p[i], p[i+1]);
   }
}
*/



/**
 * @return The reference points of the spline.
 */
const std::vector<RS_Vector>& RS_Spline::getControlPoints() const{
    return data.controlPoints;
}



/**
 * Appends the given point to the control points.
 */
void RS_Spline::addControlPoint(const RS_Vector& v) {
	data.controlPoints.push_back(v);
}



/**
 * Removes the control point that was last added.
 */
void RS_Spline::removeLastControlPoint() {
    data.controlPoints.pop_back();
}


/**
 * Generates B-Spline open knot vector with multiplicity
 * equal to the order at the ends.
 */
void RS_Spline::knot(int num, int order, std::vector<int>& knotVector) {
    knotVector[1] = 0;
    for (int i = 2; i <= num + order; i++) {
        if ( (i > order) && (i < num + 2) ) {
            knotVector[i] = knotVector[i-1] + 1;
        } else {
            knotVector[i] = knotVector[i-1];
        }
    }
}



/**
 * Generates rational B-spline basis functions for an open knot vector.
 */
void RS_Spline::rbasis(int c, double t, int npts,
					   const std::vector<int>& x, const std::vector<double>& h, std::vector<double>& r) {

    int nplusc;
    int i,k;
    double d,e;
    double sum;
    //double temp[36];

    nplusc = npts + c;

	std::vector<double> temp(nplusc+1,0.);

    // calculate the first order nonrational basis functions n[i]
    for (i = 1; i<= nplusc-1; i++) {
        if (( t >= x[i]) && (t < x[i+1]))
            temp[i] = 1;
        else
            temp[i] = 0;
    }

    /* calculate the higher order nonrational basis functions */

    for (k = 2; k <= c; k++) {
        for (i = 1; i <= nplusc-k; i++) {
            // if the lower order basis function is zero skip the calculation
            if (temp[i] != 0)
                d = ((t-x[i])*temp[i])/(x[i+k-1]-x[i]);
            else
                d = 0;
            // if the lower order basis function is zero skip the calculation
            if (temp[i+1] != 0)
                e = ((x[i+k]-t)*temp[i+1])/(x[i+k]-x[i+1]);
            else
                e = 0;

            temp[i] = d + e;
        }
    }

    // pick up last point
    if (t == (double)x[nplusc]) {
        temp[npts] = 1;
    }

    // calculate sum for denominator of rational basis functions
    sum = 0.;
    for (i = 1; i <= npts; i++) {
        sum = sum + temp[i]*h[i];
    }

    // form rational basis functions and put in r vector
    for (i = 1; i <= npts; i++) {
        if (sum != 0) {
            r[i] = (temp[i]*h[i])/(sum);
        } else
            r[i] = 0;
    }

}


/**
 * Generates a rational B-spline curve using a uniform open knot vector.
 */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
						 const std::vector<double>& b, const std::vector<double>& h, std::vector<double>& p) {

	size_t i,j,icount,jcount;
	size_t i1;
    //int x[30]; /* allows for 20 data points with basis function of order 5 */
    int nplusc;

    double step;
    double t;
    //double nbasis[20];
//    double temp;

    nplusc = npts + k;

	std::vector<int> x(nplusc+1, 0);
	std::vector<double> nbasis(npts+1, 0.);


    // generate the uniform open knot vector
    knot(npts,k,x);

    icount = 0;

    // calculate the points on the rational B-spline curve
    t = 0;
    step = ((double)x[nplusc])/((double)(p1-1));

    for (i1 = 1; i1<= p1; i1++) {

        if ((double)x[nplusc] - t < 5e-6) {
            t = (double)x[nplusc];
        }

        // generate the basis function for this value of t
        rbasis(k,t,npts,x,h,nbasis);

        // generate a point on the curve
        for (j = 1; j <= 3; j++) {
            jcount = j;
            p[icount+j] = 0.;

            // Do local matrix multiplication
            for (i = 1; i <= npts; i++) {
//                temp = nbasis[i]*b[jcount];
//                p[icount + j] = p[icount + j] + temp;
                p[icount + j] +=  nbasis[i]*b[jcount];
                jcount = jcount + 3;
            }
        }
        icount = icount + 3;
        t = t + step;
    }

}


void RS_Spline::knotu(int num, int order, std::vector<int>& knotVector) {
    int nplusc,/*nplus2,*/i;

    nplusc = num + order;
//    nplus2 = num + 2;

    knotVector[1] = 0;
    for (i = 2; i <= nplusc; i++) {
        knotVector[i] = i-1;
    }
}



void RS_Spline::rbsplinu(int npts, int k, int p1,
						 const std::vector<double>& b, const std::vector<double>& h, std::vector<double>& p) {

    int i,j,icount,jcount;
    int i1;
    //int x[30];		/* allows for 20 data points with basis function of order 5 */
    int nplusc;

    double step;
    double t;
    //double nbasis[20];
//    double temp;


    nplusc = npts + k;

	std::vector<int> x(nplusc+1, 0);
	std::vector<double> nbasis(npts+1, 0.);

    /* generate the uniform periodic knot vector */

    knotu(npts,k,x);

    /*
        printf("The knot vector is ");
        for (i = 1; i <= nplusc; i++){
                printf(" %d ", x[i]);
        }
        printf("\n");

        printf("The usable parameter range is ");
        for (i = k; i <= npts+1; i++){
                printf(" %d ", x[i]);
        }
        printf("\n");
    */

    icount = 0;

    /*    calculate the points on the rational B-spline curve */

    t = k-1;
    step = ((double)((npts)-(k-1)))/((double)(p1-1));

    for (i1 = 1; i1<= p1; i1++) {

        if ((double)x[nplusc] - t < 5e-6) {
            t = (double)x[nplusc];
        }

        rbasis(k,t,npts,x,h,nbasis);      /* generate the basis function for this value of t */
        /*
                        printf("t = %f \n",t);
                        printf("nbasis = ");
                        for (i = 1; i <= npts; i++){
                                printf("%f  ",nbasis[i]);
                        }
                        printf("\n");
        */
        for (j = 1; j <= 3; j++) {      /* generate a point on the curve */
            jcount = j;
            p[icount+j] = 0.;

            for (i = 1; i <= npts; i++) { /* Do local matrix multiplication */
//                temp = nbasis[i]*b[jcount];
//                p[icount + j] = p[icount + j] + temp;
                p[icount + j] += nbasis[i]*b[jcount];
                /*
                                                printf("jcount,nbasis,b,nbasis*b,p = %d %f %f %f %f\n",jcount,nbasis[i],b[jcount],temp,p[icount+j]);
                */
                jcount = jcount + 3;
            }
        }
        /*
                        printf("icount, p %d %f %f %f \n",icount,p[icount+1],p[icount+2],p[icount+3]);
        */
        icount = icount + 3;
        t = t + step;
    }

}


/**
 * Dumps the spline's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}


