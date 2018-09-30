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

#include<iostream>
#include<cmath>
#include<numeric>

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
	      " closed: " << ld.closed;
	if (ld.controlPoints.size()) {
		os << "\n(control points:\n";
		for (auto const& v: ld.controlPoints)
			os<<v;
		os<<")\n";
	}
	if (ld.knotslist.size()) {
		os << "\n(knot vector:\n";
		for (auto const& v: ld.knotslist)
			os<<v;
		os<<")\n";
	}
	os  << ")";
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
 * Sets the closed flag of this spline.
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
    // override the RS_EntityContainer method
    // use RS_Entity instead for spline point dragging
    return RS_Entity::getNearestRef(coord, dist);
}

RS_Vector RS_Spline::getNearestSelectedRef( const RS_Vector& coord,
                                            double* dist /*= nullptr*/) const
{
    // override the RS_EntityContainer method
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

	const size_t npts = tControlPoints.size();
    // order:
	const size_t  k = data.degree+1;
    // resolution:
	const size_t  p1 = getGraphicVariableInt("$SPLINESEGS", 8) * npts;

	std::vector<double> h(npts+1, 1.);
	std::vector<RS_Vector> p(p1, {0., 0.});
    if (data.closed) {
		rbsplinu(npts,k,p1,tControlPoints,h,p);
    } else {
		rbspline(npts,k,p1,tControlPoints,h,p);
    }

	RS_Vector prev{};
	for (auto const& vp: p) {
		if (prev.valid) {
			RS_Line* line = new RS_Line{this, prev, vp};
			line->setLayer(nullptr);
			line->setPen(RS2::FlagInvalid);
			addEntity(line);
		}
		prev = vp;
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

//TODO: private interface cleanup; de Boor's Algorithm
/**
 * Generates B-Spline open knot vector with multiplicity
 * equal to the order at the ends.
 */
std::vector<double> RS_Spline::knot(size_t num, size_t order) const{
	if (data.knotslist.size() == num + order) {
		//use custom knot vector
		return data.knotslist;
	}

	std::vector<double> knotVector(num + order, 0.);
	//use uniform knots
	std::iota(knotVector.begin() + order, knotVector.begin() + num + 1, 1);
	std::fill(knotVector.begin() + num + 1, knotVector.end(), knotVector[num]);
	return knotVector;
}



/**
 * Generates rational B-spline basis functions for an open knot vector.
 */
namespace{
std::vector<double> rbasis(int c, double t, int npts,
                                      const std::vector<double>& x,
                                      const std::vector<double>& h) {

	int const nplusc = npts + c;

	std::vector<double> temp(nplusc,0.);

    // calculate the first order nonrational basis functions n[i]
	for (int i = 0; i< nplusc-1; i++)
		if ((t >= x[i]) && (t < x[i+1])) temp[i] = 1;

    /* calculate the higher order nonrational basis functions */

	for (int k = 2; k <= c; k++) {
		for (int i = 0; i < nplusc-k; i++) {
			// if the lower order basis function is zero skip the calculation
            if (temp[i] != 0)
				temp[i] = ((t-x[i])*temp[i])/(x[i+k-1]-x[i]);
            // if the lower order basis function is zero skip the calculation
            if (temp[i+1] != 0)
				temp[i] += ((x[i+k]-t)*temp[i+1])/(x[i+k]-x[i+1]);
        }
    }

    // pick up last point
	if (t >= x[nplusc-1]) temp[npts-1] = 1;

    // calculate sum for denominator of rational basis functions
	double sum = 0.;
	for (int i = 0; i < npts; i++) {
		sum += temp[i]*h[i];
    }

	std::vector<double> r(npts, 0);
    // form rational basis functions and put in r vector
	if (sum != 0) {
		for (int i = 0; i < npts; i++)
			r[i] = (temp[i]*h[i])/sum;
	}
	return r;
}
}


/**
 * Generates a rational B-spline curve using a uniform open knot vector.
 */
void RS_Spline::rbspline(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const{
	size_t const nplusc = npts + k;

	// generate the open knot vector
	auto const x = knot(npts, k);

    // calculate the points on the rational B-spline curve
    double t {x[0]};
    double const step {(x[nplusc-1] - t) / (p1-1)};

	for (auto& vp: p) {
		if (x[nplusc-1] - t < 5e-6) t = x[nplusc-1];

        // generate the basis function for this value of t
		auto const nbasis = rbasis(k, t, npts, x, h);

        // generate a point on the curve
		for (size_t i = 0; i < npts; i++)
			vp += b[i] * nbasis[i];

		t += step;
    }

}



std::vector<double> RS_Spline::knotu(size_t num, size_t order) const{
	if (data.knotslist.size() == num + order) {
		//use custom knot vector
		return data.knotslist;
	}
	std::vector<double> knotVector(num + order, 0.);
	std::iota(knotVector.begin(), knotVector.end(), 0);
	return knotVector;
}



void RS_Spline::rbsplinu(size_t npts, size_t k, size_t p1,
                         const std::vector<RS_Vector>& b,
                         const std::vector<double>& h,
                         std::vector<RS_Vector>& p) const{
	size_t const nplusc = npts + k;

	/* generate the periodic knot vector */
	std::vector<double> const x = knotu(npts, k);

    /*    calculate the points on the rational B-spline curve */
	double t = k-1;
	double const step = double(npts - k + 1)/(p1 - 1);

	for (auto& vp: p) {
		if (x[nplusc-1] - t < 5e-6) t = x[nplusc-1];

		/* generate the basis function for this value of t */
		auto const nbasis = rbasis(k, t, npts, x, h);
		/* generate a point on the curve, for x, y, z */
		for (size_t i = 0; i < npts; i++)
			vp += b[i] * nbasis[i];

		t += step;
    }

}


/**
 * Dumps the spline's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Spline& l) {
    os << " Spline: " << l.getData() << "\n";
    return os;
}


