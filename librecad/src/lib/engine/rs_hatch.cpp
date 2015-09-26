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
#include <memory>
#include <QPainterPath>
#include <QBrush>
#include <QString>
#include "rs_hatch.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_infoarea.h"

#include "rs_information.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_math.h"


#if QT_VERSION < 0x040400
#include "emu_qt44.h"
#endif

RS_HatchData::RS_HatchData(bool _solid,
						   double _scale,
						   double _angle,
						   const QString& _pattern):
	solid(_solid)
  ,scale(_scale)
  ,angle(_angle)
  ,pattern(_pattern)
{
	//std::cout << "RS_HatchData: " << pattern.latin1() << "\n";
}

std::ostream& operator << (std::ostream& os, const RS_HatchData& td) {
	os << "(" << td.pattern.toLatin1().data() << ")";
	return os;
}

/**
 * Constructor.
 */
RS_Hatch::RS_Hatch(RS_EntityContainer* parent,
                   const RS_HatchData& d)
        : RS_EntityContainer(parent), data(d)
{

	hatch = nullptr;
    updateRunning = false;
    needOptimization = true;
    updateError = HATCH_UNDEFINED;
}



/**
 * Validates the hatch.
 */
bool RS_Hatch::validate() {
        bool ret = true;

    // loops:
		for(auto l: entities){

        if (l->rtti()==RS2::EntityContainer) {
            RS_EntityContainer* loop = (RS_EntityContainer*)l;

            ret = loop->optimizeContours() && ret;
        }
    }

        return ret;
}



RS_Entity* RS_Hatch::clone() const{
    RS_Hatch* t = new RS_Hatch(*this);
    t->setOwner(isOwner());
    t->initId();
    t->detach();
		t->hatch = nullptr;
    return t;
}


/**
 * @return Number of loops.
 */
int RS_Hatch::countLoops() const{
    if (data.solid) {
        return count();
    } else {
        return count() - 1;
    }
}

bool RS_Hatch::isContainer() const {
	return !isSolid();
}

/**
 * Recalculates the borders of this hatch.
 */
void RS_Hatch::calculateBorders() {
    RS_DEBUG->print("RS_Hatch::calculateBorders");

    activateContour(true);

    RS_EntityContainer::calculateBorders();

        RS_DEBUG->print("RS_Hatch::calculateBorders: size: %f,%f",
                getSize().x, getSize().y);

    activateContour(false);
}



/**
 * Updates the Hatch. Called when the
 * hatch or it's data, position, alignment, .. changes.
 */
void RS_Hatch::update() {
        RS_DEBUG->print("RS_Hatch::update");
        RS_DEBUG->print("RS_Hatch::update: contour has %d loops", count());

    updateError = HATCH_OK;
    if (updateRunning) {
        return;
    }

    if (updateEnabled==false) {
        return;
    }

    if (data.solid==true) {
        calculateBorders();
        return;
    }

    RS_DEBUG->print("RS_Hatch::update");
    updateRunning = true;

    // delete old hatch:
    if (hatch) {
        removeEntity(hatch);
		hatch = nullptr;
    }

    if (isUndone()) {
        updateRunning = false;
        return;
    }

    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Hatch::update: invalid contour in hatch found");
        updateRunning = false;
        updateError = HATCH_INVALID_CONTOUR;
        return;
    }

    // search pattern:
    RS_DEBUG->print("RS_Hatch::update: requesting pattern");
    RS_Pattern* pat = RS_PATTERNLIST->requestPattern(data.pattern);
	if (!pat) {
        updateRunning = false;
        RS_DEBUG->print("RS_Hatch::update: requesting pattern: not found");
        updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    }
    RS_DEBUG->print("RS_Hatch::update: requesting pattern: OK");

    RS_DEBUG->print("RS_Hatch::update: cloning pattern");
    pat = (RS_Pattern*)pat->clone();
    RS_DEBUG->print("RS_Hatch::update: cloning pattern: OK");

    // scale pattern
    RS_DEBUG->print("RS_Hatch::update: scaling pattern");
    pat->scale(RS_Vector(0.0,0.0), RS_Vector(data.scale, data.scale));
    pat->calculateBorders();
    forcedCalculateBorders();
    RS_DEBUG->print("RS_Hatch::update: scaling pattern: OK");

    // find out how many pattern-instances we need in x/y:
    int px1, py1, px2, py2;
    double f;
    RS_Hatch* copy = (RS_Hatch*)this->clone();
    copy->rotate(RS_Vector(0.0,0.0), -data.angle);
    copy->forcedCalculateBorders();

    // create a pattern over the whole contour.
    RS_Vector pSize = pat->getSize();
    RS_Vector rot_center=pat->getMin();
//    RS_Vector cPos = getMin();
    RS_Vector cSize = getSize();


    RS_DEBUG->print("RS_Hatch::update: pattern size: %f/%f", pSize.x, pSize.y);
    RS_DEBUG->print("RS_Hatch::update: contour size: %f/%f", cSize.x, cSize.y);

    if (cSize.x<1.0e-6 || cSize.y<1.0e-6 ||
            pSize.x<1.0e-6 || pSize.y<1.0e-6 ||
            cSize.x>RS_MAXDOUBLE-1 || cSize.y>RS_MAXDOUBLE-1 ||
            pSize.x>RS_MAXDOUBLE-1 || pSize.y>RS_MAXDOUBLE-1) {
        delete pat;
        delete copy;
        updateRunning = false;
        RS_DEBUG->print("RS_Hatch::update: contour size or pattern size too small");
        updateError = HATCH_TOO_SMALL;
        return;
    }

    // avoid huge memory consumption:
    else if ( cSize.x* cSize.y/(pSize.x*pSize.y)>1e4) {
        RS_DEBUG->print("RS_Hatch::update: contour size too large or pattern size too small");
        delete pat;
        delete copy;
        updateError = HATCH_AREA_TOO_BIG;
        return;
    }

    f = copy->getMin().x/pSize.x;
    px1 = (int)floor(f);
    f = copy->getMin().y/pSize.y;
    py1 = (int)floor(f);
    f = copy->getMax().x/pSize.x;
    px2 = (int)ceil(f);
    f = copy->getMax().y/pSize.y;
    py2 = (int)ceil(f);
    RS_Vector dvx=RS_Vector(data.angle)*pSize.x;
    RS_Vector dvy=RS_Vector(data.angle+M_PI*0.5)*pSize.y;
    pat->rotate(rot_center, data.angle);
    pat->move(-rot_center);


    RS_EntityContainer tmp;   // container for untrimmed lines

    // adding array of patterns to tmp:
    RS_DEBUG->print("RS_Hatch::update: creating pattern carpet");

    for (int px=px1; px<px2; px++) {
		for (int py=py1; py<py2; py++) {
			for(auto e: *pat){
                RS_Entity* te=e->clone();
                te->move(dvx*px + dvy*py);
                tmp.addEntity(te);
            }
        }
    }

    delete pat;
    pat = nullptr;
    delete copy;
    copy = nullptr;
    RS_DEBUG->print("RS_Hatch::update: creating pattern carpet: OK");


    RS_DEBUG->print("RS_Hatch::update: cutting pattern carpet");
    // cut pattern to contour shape:
    RS_EntityContainer tmp2;   // container for small cut lines
	RS_Line* line = nullptr;
	RS_Arc* arc = nullptr;
	RS_Circle* circle = nullptr;
	RS_Ellipse* ellipse = nullptr;
	for(auto e: tmp){

		line = nullptr;
		arc = nullptr;
		circle = nullptr;
		ellipse = nullptr;

        RS_Vector startPoint;
        RS_Vector endPoint;
        RS_Vector center = RS_Vector(false);
        bool reversed=false;

        switch(e->rtti()){
        case RS2::EntityLine:
            line=static_cast<RS_Line*>(e);
            startPoint = line->getStartpoint();
            endPoint = line->getEndpoint();
            break;
        case RS2::EntityArc:
            arc=static_cast<RS_Arc*>(e);
            startPoint = arc->getStartpoint();
            endPoint = arc->getEndpoint();
            center = arc->getCenter();
            reversed = arc->isReversed();
            break;
        case RS2::EntityCircle:
            circle=static_cast<RS_Circle*>(e);
            startPoint = circle->getCenter()
                    + RS_Vector(circle->getRadius(), 0.0);
            endPoint = startPoint;
            center = circle->getCenter();
            break;
        case RS2::EntityEllipse:
            ellipse = static_cast<RS_Ellipse*>(e);
            startPoint = ellipse->getStartpoint();
            endPoint = ellipse->getEndpoint();
            center = ellipse->getCenter();
            reversed = ellipse->isReversed();
            break;
        default:
            continue;
        }

        // getting all intersections of this pattern line with the contour:
        QList<std::shared_ptr<RS_Vector> > is;

		for(auto loop: entities){

            if (loop->isContainer()) {
				for(auto p: * static_cast<RS_EntityContainer*>(loop)){

                    RS_VectorSolutions sol =
                        RS_Information::getIntersection(e, p, true);

					for (const RS_Vector& vp: sol){
						if (vp.valid) {
							is.append(std::shared_ptr<RS_Vector>(
										  new RS_Vector(vp)
										  ));
							RS_DEBUG->print("  pattern line intersection: %f/%f",
											vp.x, vp.y);
						}
					}
				}
			}
		}


        QList<std::shared_ptr<RS_Vector> > is2;//to be filled with sorted intersections
        is2.append(std::shared_ptr<RS_Vector>(new RS_Vector(startPoint)));

        // sort the intersection points into is2 (only if there are intersections):
        if(is.size() == 1)
        {//only one intersection
            is2.append(is.first());
        }
        else if(is.size() > 1)
        {
            RS_Vector sp = startPoint;
            double sa = center.angleTo(sp);
			if(ellipse ) sa=ellipse->getEllipseAngle(sp);
            bool done;
            double minDist;
            double dist = 0.0;
            std::shared_ptr<RS_Vector> av;
            std::shared_ptr<RS_Vector> v;
            RS_Vector last = RS_Vector(false);
            do {
                done = true;
                minDist = RS_MAXDOUBLE;
                av.reset();
                for (int i = 0; i < is.size(); ++i) {
                    v = is.at(i);
                    double a;
                    switch(e->rtti()){
                    case RS2::EntityLine:
                        dist = sp.distanceTo(*v);
                        break;
                    case RS2::EntityArc:
                    case RS2::EntityCircle:
                        a = center.angleTo(*v);
                        dist = reversed?
                                    fmod(sa - a + 2.*M_PI,2.*M_PI):
                                    fmod(a - sa + 2.*M_PI,2.*M_PI);
                        break;
                    case RS2::EntityEllipse:
                        a = ellipse->getEllipseAngle(*v);
                        dist = reversed?
                                    fmod(sa - a + 2.*M_PI,2.*M_PI):
                                    fmod(a - sa + 2.*M_PI,2.*M_PI);
                        break;
                    default:
                        break;

                    }

                    if (dist<minDist) {
                        minDist = dist;
                        done = false;
                        av = v;
                    }
                }

                // copy to sorted list, removing double points
                if (!done && av.get()) {
                    if (last.valid==false || last.distanceTo(*av)>RS_TOLERANCE) {
                        is2.append(std::shared_ptr<RS_Vector>(new RS_Vector(*av)));
                        last = *av;
                    }
#if QT_VERSION < 0x040400
                    emu_qt44_removeOne(is, av);
#else
                    is.removeOne(av);
#endif

                    av.reset();
                }
            } while(!done);
        }

is2.append(std::shared_ptr<RS_Vector>(new RS_Vector(endPoint)));

        // add small cut lines / arcs to tmp2:
            for (int i = 1; i < is2.size(); ++i) {
                auto v1 = is2.at(i-1);
                auto v2 = is2.at(i);


                if (line) {

					tmp2.addEntity(new RS_Line{&tmp2, *v1, *v2});
                } else if (arc || circle) {
                    if(fabs(center.angleTo(*v2)-center.angleTo(*v1)) > RS_TOLERANCE_ANGLE)
                    {//don't create an arc with a too small angle
                        tmp2.addEntity(new RS_Arc(&tmp2,
                                                  RS_ArcData(center,
                                                             center.distanceTo(*v1),
                                                             center.angleTo(*v1),
                                                             center.angleTo(*v2),
                                                             reversed)));
                    }

                }
            }

    }

    // updating hatch / adding entities that are inside
    RS_DEBUG->print("RS_Hatch::update: cutting pattern carpet: OK");

    //RS_EntityContainer* rubbish = new RS_EntityContainer(getGraphic());

    // the hatch pattern entities:
    hatch = new RS_EntityContainer(this);
    hatch->setPen(RS_Pen(RS2::FlagInvalid));
	hatch->setLayer(nullptr);
    hatch->setFlag(RS2::FlagTemp);

    //calculateBorders();

	for(auto e: tmp2){

        RS_Vector middlePoint;
        RS_Vector middlePoint2;
        if (e->rtti()==RS2::EntityLine) {
			RS_Line* line = static_cast<RS_Line*>(e);
            middlePoint = line->getMiddlePoint();
            middlePoint2 = line->getNearestDist(line->getLength()/2.1,
                                                line->getStartpoint());
        } else if (e->rtti()==RS2::EntityArc) {
			RS_Arc* arc = static_cast<RS_Arc*>(e);
            middlePoint = arc->getMiddlePoint();
            middlePoint2 = arc->getNearestDist(arc->getLength()/2.1,
                                               arc->getStartpoint());
        } else {
			middlePoint = RS_Vector{false};
			middlePoint2 = RS_Vector{false};
        }

        if (middlePoint.valid) {
            bool onContour=false;

            if (RS_Information::isPointInsideContour(
                        middlePoint,
                        this, &onContour) ||
                    RS_Information::isPointInsideContour(middlePoint2, this)) {

                RS_Entity* te = e->clone();
				te->setPen(RS2::FlagInvalid);
				te->setLayer(nullptr);
                te->reparent(hatch);
                hatch->addEntity(te);
            }
        }
    }

    addEntity(hatch);
    //getGraphic()->addEntity(rubbish);

    forcedCalculateBorders();

    // deactivate contour:
    activateContour(false);

    updateRunning = false;

    RS_DEBUG->print("RS_Hatch::update: OK");
}



/**
 * Activates of deactivates the hatch boundary.
 */
void RS_Hatch::activateContour(bool on) {
        RS_DEBUG->print("RS_Hatch::activateContour: %d", (int)on);
		for(auto e: entities){
        if (!e->isUndone()) {
            if (!e->getFlag(RS2::FlagTemp)) {
                                RS_DEBUG->print("RS_Hatch::activateContour: set visible");
                e->setVisible(on);
            }
                        else {
                                RS_DEBUG->print("RS_Hatch::activateContour: entity temp");
                        }
        }
                else {
                        RS_DEBUG->print("RS_Hatch::activateContour: entity undone");
                }
    }
        RS_DEBUG->print("RS_Hatch::activateContour: OK");
}

//#include<QDebug>
/**
 * Overrides drawing of subentities. This is only ever called for solid fills.
 */
void RS_Hatch::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/) {

    if (!data.solid) {
		for(auto se: entities){

            view->drawEntity(painter,se);
        }
        return;
    }

    //area of solid fill. Use polygon approximation, except trivial cases
    QPainterPath path;
    QList<QPolygon> paClosed;
    QPolygon pa;
//    QPolygon jp;   // jump points

    // loops:
    if (needOptimization==true) {
		for(auto l: entities){

            if (l->rtti()==RS2::EntityContainer) {
                RS_EntityContainer* loop = (RS_EntityContainer*)l;

                loop->optimizeContours();
            }
        }
        needOptimization = false;
    }

    // loops:
	for(auto l: entities){
        l->setLayer(getLayer());

        if (l->rtti()==RS2::EntityContainer) {
            RS_EntityContainer* loop = (RS_EntityContainer*)l;

            // edges:
			for(auto e: *loop){

                e->setLayer(getLayer());
                switch (e->rtti()) {
                case RS2::EntityLine: {
                    QPoint pt1(RS_Math::round(view->toGuiX(e->getStartpoint().x)),
                               RS_Math::round(view->toGuiY(e->getStartpoint().y)));
                    QPoint pt2(RS_Math::round(view->toGuiX(e->getEndpoint().x)),
                               RS_Math::round(view->toGuiY(e->getEndpoint().y)));

//                    if (! (pa.size()>0 && (pa.last() - pt1).manhattanLength()<=2)) {
//                        jp<<pt1;
//                    }
                    if(pa.size() && (pa.last()-pt1).manhattanLength()>=1)
                        pa<<pt1;
                    pa<<pt2;
                }
                    break;

                case RS2::EntityArc: {
//                    QPoint pt1(RS_Math::round(view->toGuiX(e->getStartpoint().x)),
//                               RS_Math::round(view->toGuiY(e->getStartpoint().y)));
//                    if (! (pa.size()>0 && (pa.last() - pt1).manhattanLength()<=2)) {
//                        jp<<pt1;
//                    }

                    QPolygon pa2;
                    RS_Arc* arc=static_cast<RS_Arc*>(e);

                    painter->createArc(pa2, view->toGui(arc->getCenter()),
                                       view->toGuiDX(arc->getRadius())
                                       ,arc->getAngle1(),arc->getAngle2(),arc->isReversed());
                    if(pa.size() &&pa2.size()&&(pa.last()-pa2.first()).manhattanLength()<1)
                        pa2.remove(0,1);
                    pa<<pa2;

                }
                    break;

                case RS2::EntityCircle: {
                    RS_Circle* circle = static_cast<RS_Circle*>(e);
//                    QPoint pt1(RS_Math::round(view->toGuiX(circle->getCenter().x+circle->getRadius())),
//                               RS_Math::round(view->toGuiY(circle->getCenter().y)));
//                    if (! (pa.size()>0 && (pa.last() - pt1).manhattanLength()<=2)) {
//                        jp<<pt1;
//                    }

                    RS_Vector c=view->toGui(circle->getCenter());
                    double r=view->toGuiDX(circle->getRadius());
#if QT_VERSION >= 0x040400
                    path.addEllipse(QPoint(c.x,c.y),r,r);
#else
                    path.addEllipse(c.x - r, c.y + r, 2.*r, 2.*r);
//                    QPolygon pa2;
//                    painter->createArc(pa2, view->toGui(circle->getCenter()),
//                                       view->toGuiDX(circle->getRadius()),
//                                       0.0,
//                                       2*M_PI,
//                                       false);
//                    pa<<pa2;
#endif
                }
                    break;
                case RS2::EntityEllipse:
                if(static_cast<RS_Ellipse*>(e)->isArc()) {
                    QPolygon pa2;
                    auto ellipse=static_cast<RS_Ellipse*>(e);

                    painter->createEllipse(pa2,
                                           view->toGui(ellipse->getCenter()),
                                           view->toGuiDX(ellipse->getMajorRadius()),
                                           view->toGuiDX(ellipse->getMinorRadius()),
                                           ellipse->getAngle()
                                           ,ellipse->getAngle1(),ellipse->getAngle2(),ellipse->isReversed()
                                           );
//                    qDebug()<<"ellipse: "<<ellipse->getCenter().x<<","<<ellipse->getCenter().y;
//                    qDebug()<<"ellipse: pa2.size()="<<pa2.size();
//                    qDebug()<<"ellipse: pa2="<<pa2;
                    if(pa.size() && pa2.size()&&(pa.last()-pa2.first()).manhattanLength()<1)
                        pa2.remove(0,1);
                    pa<<pa2;
                }else{
                    QPolygon pa2;
                    auto ellipse=static_cast<RS_Ellipse*>(e);
                    painter->createEllipse(pa2,
                                           view->toGui(ellipse->getCenter()),
                                           view->toGuiDX(ellipse->getMajorRadius()),
                                           view->toGuiDX(ellipse->getMinorRadius()),
                                           ellipse->getAngle(),
                                           ellipse->getAngle1(), ellipse->getAngle2(),
                                           ellipse->isReversed()
                                           );
                    path.addPolygon(pa2);
                }
                    break;
                default:
                    break;
                }
//                qDebug()<<"pa="<<pa;
                if( pa.size()>2 && pa.first() == pa.last()) {
                    paClosed<<pa;
                    pa.clear();
                }

            }

        }
    }
    if(pa.size()>2){
        pa<<pa.first();
        paClosed<<pa;
    }

    for(auto& p:paClosed){
        path.addPolygon(p);
    }

    //bug#474, restore brush after solid fill
    const QBrush brush(painter->brush());
    const RS_Pen pen=painter->getPen();
    painter->setBrush(pen.getColor());
    painter->disablePen();
    painter->drawPath(path);
    painter->setBrush(brush);
    painter->setPen(pen);


}

//must be called after update()
double RS_Hatch::getTotalArea() {

    double totalArea=0.;

    // loops:
	for(auto l: entities){

        if (l!=hatch && l->rtti()==RS2::EntityContainer) {
            totalArea += l->areaLineIntegral();
        }
    }
    return totalArea;
}

double RS_Hatch::getDistanceToPoint(
    const RS_Vector& coord,
    RS_Entity** entity,
    RS2::ResolveLevel level,
    double solidDist) const {

    if (data.solid==true) {
        if (entity) {
            *entity = const_cast<RS_Hatch*>(this);
        }

        bool onContour;
        if (RS_Information::isPointInsideContour(
                    coord,
                     const_cast<RS_Hatch*>(this), &onContour)) {

            // distance is the snap range:
            return solidDist;
        }

        return RS_MAXDOUBLE;
    } else {
        return RS_EntityContainer::getDistanceToPoint(coord, entity,
                level, solidDist);
    }
}



void RS_Hatch::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    update();
}



void RS_Hatch::rotate(const RS_Vector& center, const double& angle) {
    RS_EntityContainer::rotate(center, angle);
    data.angle = RS_Math::correctAngle(data.angle+angle);
    update();
}


void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
    update();
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    data.scale *= factor.x;
    update();
}



void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    double ang = axisPoint1.angleTo(axisPoint2);
    data.angle = RS_Math::correctAngle(data.angle + ang*2.0);
    update();
}


void RS_Hatch::stretch(const RS_Vector& firstCorner,
                       const RS_Vector& secondCorner,
                       const RS_Vector& offset) {

    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    update();
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Hatch& p) {
    os << " Hatch: " << p.getData() << "\n";
    return os;
}
