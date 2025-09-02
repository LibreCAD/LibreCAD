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

#include <iostream>
#include <set>

#include <QPainterPath>

#include "lc_looputils.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_pen.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

class RS_Arc;
class RS_Pattern;
class RS_AtomicEntity;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using BPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using BBox = bg::model::box<BPoint>;
using EntityBBoxPair = std::pair<BBox, RS_Entity*>;

namespace{

// angular distance corrected for direction and range [0, 2 pi]
double angularDist(double a, double startAngle, bool reversed) {
    return reversed?
               RS_Math::correctAngle(startAngle - a):
               RS_Math::correctAngle(a - startAngle);
}

//loop debugging info
void pr(RS_EntityContainer* loop){
    if (loop == nullptr) {
        LC_ERR<<"-nullptr-;";
        return;
    }
    LC_ERR<<"( id="<<loop->getId()<<"| ";
    for (auto* e: *loop) {
        if (e&& e->rtti() == RS2::EntityContainer)
        {
            pr(static_cast<RS_EntityContainer*>(e));
        } else if (e) {
            auto vp0 = static_cast<RS_AtomicEntity*>(e)->getStartpoint();
            auto vp1 = static_cast<RS_AtomicEntity*>(e)->getEndpoint();
            LC_ERR<<", "<<e->getId()<<": "<<vp0.x<<", "<<vp0.y <<" :: "<<vp1.x<<", "<<vp1.y;
        }
    }
    LC_ERR<<" |"<<loop->getId()<<" )";
}

// Clean up zero length entities from a container
void avoidZeroLength(RS_EntityContainer& container) {
    std::set<RS_Entity*> toCleanUp;
    for (RS_Entity* e: container) {
        if (e != nullptr && e->isContainer())
            avoidZeroLength(*static_cast<RS_EntityContainer*>(e));
        else if (e != nullptr && RS_Math::equal(e->getLength(), 0.)) {
            toCleanUp.insert(e);
        }
    }
    for (RS_Entity* e: toCleanUp)
        container.removeEntity(e);
}
}

RS_HatchData::RS_HatchData(bool solid,
                           double scale,
                           double angle,
                           QString pattern,
                           RS_Vector origin):
    solid{solid}
    , scale{scale}
    , angle{angle}
    , pattern{std::move(pattern)}
    , origin{origin}
{
    //LC_LOG << "RS_HatchData: " << pattern.latin1() << "\n";
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
    : RS_EntityContainer(parent), data(d){
}

RS_Hatch::RS_Hatch(const RS_Hatch& other)
    : RS_EntityContainer(other)
    , data{other.data}
    , updateError{other.updateError}
    , m_updateRunning{other.m_updateRunning}
    , m_needOptimization{other.m_needOptimization}
    , m_updated{other.m_updated}
    , m_area{other.m_area}
{
    if (other.m_patternContainer) {
        m_patternContainer = std::make_shared<RS_EntityContainer>(*other.m_patternContainer);
    }
    if (other.m_orderedLoops) {
        m_orderedLoops = std::make_shared<RS_EntityContainer>(*other.m_orderedLoops);
    }
}

RS_Hatch& RS_Hatch::operator=(const RS_Hatch& other) {
    if (this == &other) return *this;
    RS_EntityContainer::operator=(other);
    data = other.data;
    updateError = other.updateError;
    m_updateRunning = other.m_updateRunning;
    m_needOptimization = other.m_needOptimization;
    m_updated = other.m_updated;
    m_area = other.m_area;
    if (other.m_patternContainer) {
        m_patternContainer = std::make_shared<RS_EntityContainer>(*other.m_patternContainer);
    } else {
        m_patternContainer = nullptr;
    }
    if (other.m_orderedLoops) {
        m_orderedLoops = std::make_shared<RS_EntityContainer>(*other.m_orderedLoops);
    } else {
        m_orderedLoops = nullptr;
    }
    return *this;
}

RS_Hatch::RS_Hatch(RS_Hatch&& other)
    : RS_EntityContainer(std::move(other))
    , data{std::move(other.data)}
    , m_patternContainer{std::move(other.m_patternContainer)}
    , updateError{std::move(other.updateError)}
    , m_updateRunning{std::move(other.m_updateRunning)}
    , m_needOptimization{std::move(other.m_needOptimization)}
    , m_updated{std::move(other.m_updated)}
    , m_orderedLoops{std::move(other.m_orderedLoops)}
    , m_area{std::move(other.m_area)}
{
}

RS_Hatch& RS_Hatch::operator=(RS_Hatch&& other) {
    if (this == &other) return *this;
    RS_EntityContainer::operator=(std::move(other));
    data = std::move(other.data);
    m_patternContainer = std::move(other.m_patternContainer);
    updateError = std::move(other.updateError);
    m_updateRunning = std::move(other.m_updateRunning);
    m_needOptimization = std::move(other.m_needOptimization);
    m_updated = std::move(other.m_updated);
    m_orderedLoops = std::move(other.m_orderedLoops);
    m_area = std::move(other.m_area);
    return *this;
}

/**
 * Validates the hatch.
 */
bool RS_Hatch::validate() {
    bool ret = true;

    // Collect current loops (non-owning copies to avoid modifying during iteration)
    std::vector<std::unique_ptr<RS_EntityContainer>> oldLoops;
    for (RS_Entity* e : *this) {
        if (e->isContainer()) {
            std::unique_ptr<RS_EntityContainer> loop{static_cast<RS_EntityContainer*>(e->clone())};
            avoidZeroLength(*loop);  // Retain zero-length cleanup
            oldLoops.push_back(std::move(loop));
        }
    }

    // Clear existing loops (hatch owns them, so this deletes old ones if autoDelete=true)
    clear();

    // Optimize each old loop using LoopOptimizer
    for (const std::unique_ptr<RS_EntityContainer>& oldLoop : oldLoops) {
        LC_LoopUtils::LoopOptimizer optimizer{*oldLoop};
        auto optimized = optimizer.GetResults();  // Shared ptr to owned result container

        // Expect one loop per input; if multiple, log error and mark invalid
        if (optimized->count() != 1) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::validate: Optimization produced %d sub-loops (expected 1); invalid contour.", optimized->count());
            ret = false;
            continue;
        }

        // Add the optimized loop to hatch
        RS_EntityContainer* newLoopContainer = new RS_EntityContainer(this, false);
        RS_EntityContainer* optimizedLoop = static_cast<RS_EntityContainer*>(optimized->entityAt(0));
        for (RS_Entity* edge : *optimizedLoop) {
            newLoopContainer->addEntity(edge->clone());  // Clone to transfer ownership
        }
        addEntity(newLoopContainer);
    }

    // Compute area if valid loops remain
    if (ret && countLoops() > 0) {
        getTotalArea();
    } else {
        ret = false;
    }

    // Final closure check using new helper
    for (RS_Entity* e : *this) {
        if (e->isContainer() && !LC_LoopUtils::isLoopClosed(*static_cast<RS_EntityContainer*>(e))) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::validate: Loop not closed after optimization.");
            ret = false;
        }
    }

    return ret;
}

RS_Entity* RS_Hatch::clone() const{
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone()");
    auto* t = new RS_Hatch(*this);
    t->setOwner(isOwner());
    t->detach();
    t->update();
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
    if (m_updateRunning) {
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
    m_updateRunning = true;

    // save attributes for the current hatch
    RS_Layer* hatch_layer = this->getLayer(); // fixme - sand - is it really necessary to use resolved pen there?
    RS_Pen hatch_pen = this->getPen();

    // delete old hatch:
    if (m_patternContainer) {
        m_patternContainer.reset();
    }

    if (isUndone()) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip undone hatch");
        m_updateRunning = false;
        return;
    }

    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: invalid contour in hatch found");
        m_updateRunning = false;
        updateError = HATCH_INVALID_CONTOUR;
        return;
    }

    // search for pattern
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern");
    std::unique_ptr<RS_Pattern> pat = RS_PATTERNLIST->requestPattern(data.pattern);
    if (pat == nullptr) {
        m_updateRunning = false;
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: requesting pattern: %s not found", data.pattern.toUtf8().constData());
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
        m_updateRunning = false;
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
    m_patternContainer = std::make_shared<RS_EntityContainer>(this);
    m_patternContainer->setPen(hatch_pen);
    m_patternContainer->setLayer(hatch_layer);
    m_patternContainer->setFlag(RS2::FlagTemp);

    //calculateBorders();
    for(auto e: tmp2){

        RS_Vector middlePoint;
        RS_Vector middlePoint2;
        if (e->rtti()==RS2::EntityLine) {
            auto* line = static_cast<RS_Line*>(e);
            middlePoint = line->getMiddlePoint();
            middlePoint2 = line->getNearestDist(line->getLength()/2.1,
                                                line->getStartpoint());
        } else if (e->rtti()==RS2::EntityArc) {
            auto* arc = static_cast<RS_Arc*>(e);
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
                te->reparent(m_patternContainer.get());
                te->setFlag(RS2::FlagHatchChild);
                m_patternContainer->addEntity(te);
            }
        }
    }

    //addEntity(hatch);
    //getGraphic()->addEntity(rubbish);

    forcedCalculateBorders();

    // deactivate contour:
    activateContour(false);

    m_updateRunning = false;
    m_updated = true;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: OK");
}

// Build R-tree for contour entities to accelerate intersection queries
std::unique_ptr<bgi::rtree<EntityBBoxPair, bgi::quadratic<16>>> buildContourIndex(const RS_EntityContainer& contour) {
    auto index = std::make_unique<bgi::rtree<EntityBBoxPair, bgi::quadratic<16>>>();
    for (const RS_Entity* loop : contour) {
        if (loop->isContainer()) {
            auto* ec = static_cast<const RS_EntityContainer*>(loop);
            for (const RS_Entity* e : *ec) {
                if (e->isVisible()) {
                    RS_Vector minV = e->getMin(), maxV = e->getMax();
                    BBox box{{minV.x, minV.y}, {maxV.x, maxV.y}};
                    index->insert({box, const_cast<RS_Entity*>(e)});
                }
            }
        }
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "buildContourIndex: Built R-tree with %d entities", index->size());
    return index;
}

/**
 * Trims the pattern to the contour shape using accelerated spatial queries.
 */
RS_EntityContainer RS_Hatch::trimPattern(const RS_EntityContainer& patternEntities) const {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::trimPattern: begin");

    RS_EntityContainer trimmed;
    auto contourIndex = buildContourIndex(*this);  // O(c log c) build time

    for (RS_Entity *e: patternEntities) {

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

        // Get entity bbox for query
        RS_Vector minV = e->getMin(), maxV = e->getMax();
        BBox eBox{{minV.x, minV.y}, {maxV.x, maxV.y}};

        // Query overlapping contour entities
        std::vector<EntityBBoxPair> candidates;
        contourIndex->query(bgi::intersects(eBox), std::back_inserter(candidates));
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "trimPattern: Found %d candidate contours for entity", candidates.size());

        // getting all intersections of this pattern line with the contour:
        QList<RS_Vector> is;

        for(const auto& cand: candidates){

            RS_VectorSolutions sol =
                RS_Information::getIntersection(e, cand.second, true);

            for (const RS_Vector& vp: sol) {
                if (vp.valid) {
                    is.append(vp);
                    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "  pattern line intersection: %f/%f", vp.x, vp.y);
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
    return trimmed;
}

/**
 * Activates of deactivates the hatch boundary.
 */
void RS_Hatch::activateContour(bool on) {
    RS_DEBUG->print("RS_Hatch::activateContour: %d", (int)on);
    for(RS_Entity* e: *this){
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
void RS_Hatch::draw(RS_Painter* painter) {
    if (data.solid) {
        drawSolidFill(painter);
    }
    else{
        for(RS_Entity* se: *this){
            painter->drawEntity(se);
        }
        if (m_patternContainer) {
            for(RS_Entity* pe: *m_patternContainer) {
                painter->drawEntity(pe);
            }
        }
    }
}

void RS_Hatch::drawSolidFill(RS_Painter *painter) {//area of solid fill. Use polygon approximation, except trivial cases

    if (m_needOptimization == true) {
        LC_LoopUtils::LoopOptimizer optimizer{*this};
        m_orderedLoops = optimizer.GetResults();
        m_needOptimization = false;
    }

    if (m_orderedLoops == nullptr)
        return;

    const QBrush brush(painter->brush());
    const RS_Pen pen=painter->getPen();
    try {
        QPainterPath path = createSolidFillPath(painter);
        QBrush fillBrush = brush;
        fillBrush.setColor(pen.getColor());
        fillBrush.setStyle(Qt::SolidPattern);
        painter->setBrush(fillBrush);
        painter->drawPath(path);
    } catch(...) {
    }

    painter->setBrush(brush);
    painter->setPen(pen);
}

QPainterPath RS_Hatch::createSolidFillPath(RS_Painter *painter) const
{
    return painter->createSolidFillPath(*m_orderedLoops);
}

void RS_Hatch::debugOutPath(const QPainterPath &tmpPath) const {
    int c = tmpPath.elementCount();
    for (int i = 0; i < c; i++){
        const QPainterPath::Element &element = tmpPath.elementAt(i);
        LC_ERR << "i " << i << "("<< element.x << "," << element.y <<  ") Line To " << element.isLineTo() << " Move To: " << element.isMoveTo() << " Is Curve:" << element.isCurveTo();
    }
}

//must be called after update()
double RS_Hatch::getTotalArea() const
{
    if (m_area + RS_Math::ulp(m_area) < RS_MAXDOUBLE)
        return m_area;
    try {
        getTotalAreaImpl();
    } catch(...) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch:: %s() failure in find hatch area", __func__);
    }

    return m_area;
}

// #define DEBUG_TOTAL_AREA_
double RS_Hatch::getTotalAreaImpl() const
{
    auto loops = getLoops();
#ifdef DEBUG_TOTAL_AREA
    LC_ERR<<__func__<<"(): loops.size()="<<loops.size();
    int i=0;
    for (auto& l: loops) {
        LC_ERR<<i++<<": "<<l->getId()<<": "<<l->rtti();
        pr(l.get());
    }
    LC_ERR<<"loops: done";
#endif
    LC_LoopUtils::LoopSorter loopSorter(std::move(loops));
    auto sorted = loopSorter.getResults();

    m_area=0.;

    // loops:
    for(auto* loop: sorted){
        m_area += loop->areaLineIntegral();
#ifdef DEBUG_TOTAL_AREA
        LC_LOG<<__func__<<"(): "<<loop->getId()<<": m_area="<<m_area;
#endif
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
    data.origin += offset;
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, double angle) {
    RS_EntityContainer::rotate(center, angle);
    data.angle = RS_Math::correctAngle(data.angle+angle);
    data.origin.rotate(center, angle);
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
    data.origin.rotate(center, angleVector);
    update();
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    data.scale *= sqrt(factor.x * factor.y);  // Geometric mean for non-uniform
    if (fabs(factor.x - factor.y) > RS_TOLERANCE) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Non-uniform scale applied to hatch; using geometric mean for pattern scale.");
    }
    data.origin.scale(center, factor);
    update();
}

void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    if (axisPoint1.distanceTo(axisPoint2) > RS_TOLERANCE) {
        RS_EntityContainer::mirror(axisPoint1, axisPoint2);
        double ang = axisPoint1.angleTo(axisPoint2);
        data.angle = RS_Math::correctAngle(data.angle + ang*2.0);
        data.origin.mirror(axisPoint1, axisPoint2);
    }
    update();
}

void RS_Hatch::stretch(
    const RS_Vector &firstCorner,
    const RS_Vector &secondCorner,
    const RS_Vector &offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
        getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    } else {

        for (RS_Entity *e: *this) {
            e->stretch(firstCorner, secondCorner, offset);
        }
    }

    // some entitiycontainers might need an update (e.g. RS_Leader):
    update();
}


void RS_Hatch::moveRef(
    const RS_Vector &ref,
    const RS_Vector &offset) {

    resetBorders();
    for (RS_Entity *e: *this) {
        e->moveRef(ref, offset);
        adjustBorders(e);
    }
        calculateBorders();
}

void RS_Hatch::moveSelectedRef(
    const RS_Vector &ref,
    const RS_Vector &offset) {

    resetBorders();
    for (RS_Entity *e: *this) {
        e->moveSelectedRef(ref, offset);
        adjustBorders(e);
    }
        calculateBorders();
}

void RS_Hatch::revertDirection() {
    // revert entity order in the container
    for (size_t k = 0; k < size() / 2; ++k) {
        size_t kv = size() - k - 1;
        RS_Entity* entK = entityAt(k);
        RS_Entity* entKv = entityAt(kv);
        setEntityAt(int(k), entKv);
        setEntityAt(int(kv), entK);
    }

    // revert each entity itself
    for (RS_Entity *entity: std::as_const(*this))
        entity->revertDirection();
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Hatch& p) {
    os << " Hatch: " << p.getData() << "\n";
    return os;
}
