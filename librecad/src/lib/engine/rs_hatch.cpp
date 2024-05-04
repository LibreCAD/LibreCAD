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
#include <cmath>
#include <iostream>
#include <memory>

#include <QPainterPath>
#include <QBrush>
#include <QString>

#include "lc_looputils.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"


namespace
{

// angular distance corrected for direction and range [0, 2 pi]
double angularDist(double a, double startAngle, bool reversed) {
    return reversed?
               RS_Math::correctAngle(startAngle - a):
               RS_Math::correctAngle(a - startAngle);
}

//loop debugging info
void pr(RS_EntityContainer* loop)
{
    if (loop == nullptr)
    {
        LC_ERR<<"-nullptr-;";
        return;
    }
    LC_ERR<<"( id="<<loop->getId()<<"| ";
    for (auto* e: *loop) {
        if (e&& e->rtti() == RS2::EntityContainer)
        {
            pr(static_cast<RS_EntityContainer*>(e));
        } else if (e) {
            LC_ERR<<", "<<e->getId();
        }
    }
    LC_ERR<<" |"<<loop->getId()<<" )";
}
}


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
    foreach(auto* l, entities){

        if (l->rtti()==RS2::EntityContainer) {
            RS_EntityContainer* loop = (RS_EntityContainer*)l;

            ret = loop->optimizeContours() && ret;
        }
    }

    if(ret)
        getTotalArea();

    return ret;
}



RS_Entity* RS_Hatch::clone() const{
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone()");
    RS_Hatch* t = new RS_Hatch(*this);
    t->setOwner(isOwner());
    t->initId();
    t->detach();
    t->update();
//    t->hatch = nullptr;
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone(): OK");
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
 *
 * Refill hatch with pattern. Move, scale, rotate, trim, etc.
 */
void RS_Hatch::update() {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update");

    updateError = HATCH_OK;
    if (updateRunning) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch in updating process");
        return;
    }

    if (updateEnabled==false) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch forbidden to update");
        return;
    }

    if (data.solid==true) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: processing solid hatch");
        calculateBorders();
        return;
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: contour has %d loops", count());
    updateRunning = true;

    // save attributes for the current hatch
    RS_Layer* hatch_layer = this->getLayer();
    RS_Pen hatch_pen = this->getPen();

    // delete old hatch:
    if (hatch) {
        removeEntity(hatch);
		hatch = nullptr;
    }

    if (isUndone()) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip undone hatch");
        updateRunning = false;
        return;
    }

    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: invalid contour in hatch found");
        updateRunning = false;
        updateError = HATCH_INVALID_CONTOUR;
        return;
    }

    // search for pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern");
    std::unique_ptr<RS_Pattern> pat = RS_PATTERNLIST->requestPattern(data.pattern);
    if (pat == nullptr) {
        updateRunning = false;
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: requesting pattern: %s not found", data.pattern.toStdString().c_str());
        updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern: OK");
        // make a working copy of hatch pattern
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern");
        pat.reset((RS_Pattern*)pat->clone());
        if (pat != nullptr) {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern: OK");
        } else {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: error while cloning hatch pattern");
            return;
        }
    }

    // scale pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern");
    pat->scale(RS_Vector(0.0,0.0), RS_Vector(data.scale, data.scale));
    pat->calculateBorders();
    forcedCalculateBorders();
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern: OK");

    std::unique_ptr<RS_Hatch> copy {(RS_Hatch*)this->clone()};
    copy->rotate(RS_Vector(0.0,0.0), -data.angle);
    copy->forcedCalculateBorders();

    // create a pattern over the whole contour.
    RS_Vector pSize = pat->getSize();
    RS_Vector rot_center=pat->getMin();
//    RS_Vector cPos = getMin();
    RS_Vector cSize = getSize();

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: pattern size: %f/%f", pSize.x, pSize.y);
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: contour size: %f/%f", cSize.x, cSize.y);

    // check pattern sizes for sanity
    if (cSize.x<1.0e-6 || cSize.y<1.0e-6 ||
            pSize.x<1.0e-6 || pSize.y<1.0e-6 ||
            cSize.x>RS_MAXDOUBLE-1 || cSize.y>RS_MAXDOUBLE-1 ||
            pSize.x>RS_MAXDOUBLE-1 || pSize.y>RS_MAXDOUBLE-1) {
        updateRunning = false;
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size or pattern size too small");
        updateError = HATCH_TOO_SMALL;
        return;
    }
    // avoid huge memory consumption:
    else if ( cSize.x* cSize.y/(pSize.x*pSize.y)>1e4) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size too large or pattern size too small");
        updateError = HATCH_AREA_TOO_BIG;
        return;
    }

    // calculate pattern pieces quantity
    // find out how many pattern-instances we need in x/y:
    int px1 = (int)floor(copy->getMin().x/pSize.x);
    int py1 = (int)floor(copy->getMin().y/pSize.y);
    int px2 = (int)ceil(copy->getMax().x/pSize.x);
    int py2 = (int)ceil(copy->getMax().y/pSize.y);
    RS_Vector dvx=RS_Vector(data.angle)*pSize.x;
    RS_Vector dvy=RS_Vector(data.angle+M_PI*0.5)*pSize.y;
    pat->rotate(rot_center, data.angle);
    pat->move(-rot_center);

    RS_EntityContainer tmp;   // container for untrimmed lines

    // adding array of patterns to tmp:
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: creating pattern carpet");
    for (int px=px1; px<px2; px++) {
		for (int py=py1; py<py2; py++) {
			for(auto e: *pat){
                RS_Entity* te=e->clone();
                te->move(dvx*px + dvy*py);
                tmp.addEntity(te);
            }
        }
    }

    // clean memory
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: creating pattern carpet: OK");

    // cut pattern to contour shape
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cutting pattern carpet");
    // start for very very long for(auto e: tmp) loop
    RS_EntityContainer tmp2 = trimPattern(tmp);   // container for small cut lines
    // end for very very long for(auto e: tmp) loop

    // updating hatch / adding entities that are inside
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cutting pattern carpet: OK");

    // add the hatch pattern entities
    hatch = new RS_EntityContainer(this);
    hatch->setPen(hatch_pen);
    hatch->setLayer(hatch_layer);
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
                te->setPen(hatch_pen);
                te->setLayer(hatch_layer);
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
    m_updated = true;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: OK");
}


RS_EntityContainer RS_Hatch::trimPattern(const RS_EntityContainer& patternEntities) const
{
    LC_LOG<<"RS_Hatch::"<<__func__<<": begin";
    RS_EntityContainer trimmed;
    for(auto* e: patternEntities) {

        if (!e) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Hatch::update: nullptr entity found");
            continue;
        }

        RS_Line* line = nullptr;
        RS_Arc* arc = nullptr;
        RS_Circle* circle = nullptr;
        RS_Ellipse* ellipse = nullptr;

        RS_Vector startPoint;
        RS_Vector endPoint;
        RS_Vector center{};
        bool reversed=false;

        switch(e->rtti()) {
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
        QList<RS_Vector> is;

        foreach(const auto& loop, entities){

            if (loop->isContainer()) {
                for(auto p: * static_cast<RS_EntityContainer*>(loop)){

                    RS_VectorSolutions sol =
                        RS_Information::getIntersection(e, p, true);

                    for (const RS_Vector& vp: sol) {
                        if (vp.valid) {
                            is.append(vp);
                            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "  pattern line intersection: %f/%f", vp.x, vp.y);
                        }
                    }
                }
            }
        }

        QList<RS_Vector> is2;       //to be filled with sorted intersections
        is2.append(startPoint);

        // sort the intersection points into is2 (only if there are intersections):
        if(is.size() == 1) {        //only one intersection
            is2.append(is.first());
        }
        else if(is.size() > 1) {
            RS_Vector sp = startPoint;
            double sa = center.angleTo(sp);
            if(ellipse )
                sa=ellipse->getEllipseAngle(sp);
            bool done = false;
            double dist = 0.0;
            RS_Vector av{};
            RS_Vector last{};
            do {    // very long while(!done) loop
                done = true;
                double minDist = RS_MAXDOUBLE;
                av.valid = false;
                for (const RS_Vector& v: is) {
                    switch(e->rtti()){
                    case RS2::EntityLine:
                        dist = sp.distanceTo(v);
                        break;
                    case RS2::EntityArc:
                    case RS2::EntityCircle:
                        dist = angularDist(center.angleTo(v), sa, reversed);
                        break;
                    case RS2::EntityEllipse:
                        dist = angularDist(ellipse->getEllipseAngle(v), sa, reversed);
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
                if (!done && av) {
                    if (last.valid==false || last.distanceTo(av)>RS_TOLERANCE) {
                        is2.append(av);
                        last = av;
                    }
                    is.removeOne(av);

                    av.valid = false;
                }
            } while(!done);
        }

        is2.append(endPoint);

        // add small cut lines / arcs to tmp2:
        for (int i = 1; i < is2.size(); ++i) {
            auto v1 = is2.at(i-1);
            auto v2 = is2.at(i);

            if (line) {
                trimmed.addEntity(new RS_Line{&trimmed, v1, v2});
            } else if (arc || circle) {
                if(fabs(center.angleTo(v2)-center.angleTo(v1)) > RS_TOLERANCE_ANGLE)
                {//don't create an arc with a too small angle
                    trimmed.addEntity(new RS_Arc(&trimmed,
                                              RS_ArcData(center,
                                                         center.distanceTo(v1),
                                                         center.angleTo(v1),
                                                         center.angleTo(v2),
                                                         reversed)));
                }
            }
        }
    }
    LC_LOG<<"RS_Hatch::"<<__func__<<": done";
    return trimmed;
}

/**
 * Activates of deactivates the hatch boundary.
 */
void RS_Hatch::activateContour(bool on) {
        RS_DEBUG->print("RS_Hatch::activateContour: %d", (int)on);
        foreach(auto* e, entities){
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

/**
 * Overrides drawing of subentities. This is only ever called for solid fills.
 */
void RS_Hatch::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/) {

    if (!data.solid) {
        foreach (auto se, entities){

            view->drawEntity(painter,se);
        }
        return;
    }

    //area of solid fill. Use polygon approximation, except trivial cases
    QPainterPath path;
    QList<QPolygon> paClosed;
    QPolygon pa;

    if (needOptimization==true) {
        foreach (auto l, entities){

            if (l->rtti()==RS2::EntityContainer) {
                RS_EntityContainer* loop = (RS_EntityContainer*)l;

                loop->optimizeContours();
            }
        }
        needOptimization = false;
    }

    foreach (auto l, entities){
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

                    if(!pa.size() || (pa.last() - pt1).manhattanLength() >= 1) {
                        pa << pt1;
                    }
                    pa << pt2;
                }
                    break;

                case RS2::EntityArc: {
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
                    RS_Vector c=view->toGui(circle->getCenter());
                    double r=view->toGuiDX(circle->getRadius());
                    path.addEllipse(QPoint(c.x,c.y),r,r);
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
    if (m_area + RS_Math::ulp(m_area) < RS_MAXDOUBLE)
        return m_area;
    try {
        getTotalAreaImpl();
    } catch(...) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch:: %s() failure in find hatch area", __func__);
    }

    return m_area;
}

double RS_Hatch::getTotalAreaImpl() {
    auto loops = getLoops();
    LC_LOG<<__func__<<"(): loops.size()="<<loops.size();
    for (auto& l: loops)
    {
        LC_LOG<<l->getId()<<": "<<l->rtti();
        pr(l.get());
    }
    LC_LOG<<"loops: done";

    LC_LoopUtils::LoopSorter loopSorter(std::move(loops));
    auto sorted = loopSorter.getResults();

    m_area=0.;

    // loops:
    for(auto* loop: sorted){
        m_area += loop->areaLineIntegral();
        LC_LOG<<__func__<<"(): "<<loop->getId()<<": m_area="<<m_area;
    }
    return m_area;
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
