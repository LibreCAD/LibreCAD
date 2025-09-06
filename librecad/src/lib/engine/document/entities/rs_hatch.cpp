/*********************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** Copyright (C) 2023-2025 LibreCAD.org
** Copyright (C) 2023-2025 dxli (github.com/dxli)
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
*********************************************************************************/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "rs_hatch.h"

#include "lc_looputils.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_painter.h"
#include "rs_spline.h"

#include <QPainterPath>

std::ostream& operator << (std::ostream& os, const RS_HatchData& td) {
    os << "solid=" << td.solid << " scale=" << td.scale << " angle=" << td.angle << " pattern=" << td.pattern.toStdString();
    return os;
}

RS_Hatch::RS_Hatch(RS_EntityContainer* parent,
                   const RS_HatchData& d)
    : RS_EntityContainer(parent, false)
    , data(d)
    , hatch(this, true)
    , m_area(RS_MAXDOUBLE)
    , updateError(HATCH_UNDEFINED)
    , updateRunning(false)
    , needOptimization(true)
    , m_updated(false)
{
}

RS_Entity* RS_Hatch::clone() const {
    RS_Hatch* h = new RS_Hatch(*this);
    h->hatch = *static_cast<RS_EntityContainer*>(hatch.clone());
    h->hatch.reparent(h);
    h->detach();
    h->initId();
    return h;
}

bool RS_Hatch::isContainer() const {
    return true;
}

bool RS_Hatch::validate() {
    bool ret = true;

    for (unsigned int i = 0; i < count(); ++i) {
        RS_Entity* l = entityAt(i);
        if (l->rtti() != RS2::EntityContainer) {
            ret = false;
        } else {
            RS_EntityContainer* loop = static_cast<RS_EntityContainer*>(l);
            if (!loop->isClosed()) {
                ret = false;
            } else {
                loop->calculateBorders();
            }
        }
    }

    return ret;
}

int RS_Hatch::countLoops() const {
    return count();
}

double RS_Hatch::getTotalArea() const {
    if (m_area < 0.) {
        double area = 0.0;
        for (unsigned i = 0; i < count(); ++i) {
            RS_Entity* loop = entityAt(i);
            if (loop->rtti() == RS2::EntityContainer) {
                area += static_cast<RS_EntityContainer*>(loop)->areaLineIntegral();
            }
        }
        m_area = std::abs(area);
    }
    return m_area;
}

void RS_Hatch::calculateBorders() {
    resetBorders();
    for (unsigned int i = 0; i < count(); ++i) {
        RS_Entity* l = entityAt(i);
        if (l->rtti() == RS2::EntityContainer) {
            RS_EntityContainer* loop = static_cast<RS_EntityContainer*>(l);
            loop->calculateBorders();
            minV = RS_Vector::minimum(minV, loop->getMin());
            maxV = RS_Vector::maximum(maxV, loop->getMax());
        }
    }
}

void RS_Hatch::update() {
    RS_DEBUG->print("RS_Hatch::update");

    if (updateRunning) {
        RS_DEBUG->print("RS_Hatch::update: skipping recursion");
        return;
    }

    updateRunning = true;
    m_updated = false;

    if (isSolid()) {
        m_updated = true;
    } else {
        hatch.clear();

        std::unique_ptr<RS_Pattern> pat = RS_PATTERNLIST->requestPattern(getPattern());
        if (!pat) {
            updateError = HATCH_PATTERN_NOT_FOUND;
            RS_DEBUG->print("RS_Hatch::update: pattern not found");
            updateRunning = false;
            return;
        }

        if (fabs(getScale()) < 1e-6) {
            updateError = HATCH_TOO_SMALL;
            RS_DEBUG->print("RS_Hatch::update: scale too small");
            updateRunning = false;
            return;
        }

        RS_DEBUG->print("RS_Hatch::update: got pattern: %s", pat->getFileName().toLatin1().data());

        if (needOptimization) {
            if (!optimizeContours()) {
                updateError = HATCH_INVALID_CONTOUR;
                updateRunning = false;
                return;
            }
            needOptimization = false;
        }

        double a = getAngle();
        if (fabs(a) < 1.0e-6) {
            a = 0.0;
        }

        pat->loadPattern();

        if (pat->count() > 0) {
            updateError = HATCH_OK;

            double x1, y1, x2, y2, aOffset;
            double spacing = 1.0; // pat->getScreenBasedLineSpacing(); assume 1.0 for now

            RS_Vector vr(spacing, 0.0);
            vr.rotate(a);

            RS_DEBUG->print("RS_Hatch::update: spacing: %g angle: %g", spacing, a);
            RS_DEBUG->print("RS_Hatch::update: line ver: %g/%g", vr.x, vr.y);

            RS_Vector vMin = getMin();
            RS_Vector vMax = getMax();

            RS_DEBUG->print("RS_Hatch::update: viewport min: %g/%g", vMin.x, vMin.y);
            RS_DEBUG->print("RS_Hatch::update: viewport max: %g/%g", vMax.x, vMax.y);

            int lineNum = 0;
            for (RS_Entity* e = pat->firstEntity(); e; e = pat->nextEntity()) {
                if (e->rtti() == RS2::EntityLine) {
                    RS_Line* line = static_cast<RS_Line*>(e);
                    x1 = line->getStartpoint().x;
                    y1 = line->getStartpoint().y;
                    x2 = line->getEndpoint().x;
                    y2 = line->getEndpoint().y;
                    aOffset = 0.0; // assume 0 for now

                    RS_DEBUG->print("RS_Hatch::update: hatch line %d: %g, %g -> %g, %g aoffset: %g", lineNum, x1, y1, x2, y2, aOffset);

                    RS_Vector v1(x2 - x1, y2 - y1);
                    if (v1.magnitude() < 1e-6) {
                        continue;
                    }

                    RS_Vector v2 = RS_Vector::polar(v1.magnitude(), v1.angle() + a + M_PI_2);
                    v2 *= (aOffset / v1.magnitude());

                    double baseX = x1 + v2.x;
                    double baseY = y1 + v2.y;

                    double baseAngle = v1.angle() + a + M_PI_2;
                    while (baseAngle > 2 * M_PI) {
                        baseAngle -= 2 * M_PI;
                    }

                    RS_Vector vBase(baseX, baseY);
                    vBase.rotate(baseAngle);

                    RS_DEBUG->print("RS_Hatch::update: hatch line %d base point: %g, %g", lineNum, vBase.x, vBase.y);

                    RS_Vector v1s = RS_Vector(x1, y1);
                    v1s.rotate(a);

                    RS_Vector v2s = RS_Vector(x2, y2);
                    v2s.rotate(a);

                    double lineLength = v1s.distanceTo(v2s) * getScale();

                    RS_Vector vLine = (v2s - v1s).scale(getScale());

                    RS_Vector vDelta = vr * ((int)((vMin.distanceTo(vMax)) / spacing) + 3);

                    RS_Vector vStart = vBase * getScale() - vDelta;

                    RS_Vector vCur = vStart;

                    RS_EntityContainer tmpLines;

                    while (true) {

                        RS_Line* tmpLine = new RS_Line(nullptr, vCur, vCur + vLine);

                        if (tmpLine->getStartpoint().x > tmpLine->getEndpoint().x) {

                            tmpLine->reverse();

                        }

                        bool inside = false;

                        RS_Vector mid = tmpLine->getMiddlePoint();

                        inside = RS_Information::isPointInsideContour(mid, const_cast<RS_Hatch*>(this), nullptr);

                        if (inside) {

                            tmpLines.addEntity(tmpLine);

                        } else {

                            delete tmpLine;

                        }

                        vCur += vr;

                        if (vCur.x > vMax.x + 1.0 || vCur.y > vMax.y + 1.0) {

                            break;

                        }

                    }

                    trimPattern(tmpLines);

                }
                lineNum++;
            }

        } else {

            // angled pattern, todo

        }

        m_updated = true;

    }

    updateRunning = false;

}

void RS_Hatch::activateContour(bool on) {
    for (unsigned i = 0; i < count(); ++i) {
        RS_Entity* l = entityAt(i);
        if (l->rtti() == RS2::EntityContainer) {
            l->setVisible(on);
        }
    }
}

void RS_Hatch::draw(RS_Painter* painter) {
    if (isSolid()) {
        drawSolidFill(painter);
    } else {
        hatch.draw(painter);
    }
}

double RS_Hatch::getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const {
    if (isSolid()) {
        bool inside = RS_Information::isPointInsideContour(coord, const_cast<RS_Hatch*>(this), nullptr);
        if (inside) {
            return 0.0;
        } else {
            return RS_EntityContainer::getDistanceToPoint(coord, entity, level, solidDist);
        }
    } else {
        return RS_EntityContainer::getDistanceToPoint(coord, entity, level, solidDist);
    }
}

void RS_Hatch::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    hatch.move(offset);
    m_updated = false;
    needOptimization = true;
}

void RS_Hatch::rotate(const RS_Vector& center, double angle) {
    RS_EntityContainer::rotate(center, angle);
    hatch.rotate(center, angle);
    data.angle += angle;
    m_updated = false;
    needOptimization = true;
}

void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    hatch.rotate(center, angleVector);
    data.angle += angleVector.angle();
    m_updated = false;
    needOptimization = true;
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    hatch.scale(center, factor);
    data.scale *= factor.x;
    m_updated = false;
    needOptimization = true;
}

void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    hatch.mirror(axisPoint1, axisPoint2);
    data.angle = RS_Math::correctAngle(data.angle + 2 * (axisPoint2 - axisPoint1).angle());
    m_updated = false;
    needOptimization = true;
}

void RS_Hatch::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    m_updated = false;

}

void RS_Hatch::trimPattern(const RS_EntityContainer& patternEntities) {
    for (unsigned int i = 0; i < patternEntities.count(); ++i) {
        RS_Line* l = static_cast<RS_Line*>(patternEntities.entityAt(i)->clone());

        std::vector<RS_Vector> ints;

        for (unsigned j = 0; j < count(); ++j) {
            RS_Entity* loop = entityAt(j);
            if (loop->rtti() == RS2::EntityContainer) {
                RS_EntityContainer* lp = static_cast<RS_EntityContainer*>(loop);
                for (unsigned k = 0; k < lp->count(); ++k) {
                    RS_Entity* edge = lp->entityAt(k);
                    if (edge->isEdge()) {
                        RS_VectorSolutions sol = RS_Information::getIntersection(l, edge, true);
                        for (int m = 0; m < sol.getNumber(); ++m) {
                            ints.push_back(sol.get(m));
                        }
                    }
                }
            }
        }

        std::sort(ints.begin(), ints.end(), [l](const RS_Vector& p1, const RS_Vector& p2) {
            return l->getStartpoint().distanceTo(p1) < l->getStartpoint().distanceTo(p2);
        });

        RS_Vector start = l->getStartpoint();

        for (auto p : ints) {
            RS_Line* segment = new RS_Line(this, start, p);
            RS_Vector mid = segment->getMiddlePoint();
            if (RS_Information::isPointInsideContour(mid, const_cast<RS_Hatch*>(this), nullptr)) {
                hatch.addEntity(segment);
            } else {
                delete segment;
            }
            start = p;
        }

        RS_Line* segment = new RS_Line(this, start, l->getEndpoint());
        RS_Vector mid = segment->getMiddlePoint();
        if (RS_Information::isPointInsideContour(mid, const_cast<RS_Hatch*>(this), nullptr)) {
            hatch.addEntity(segment);
        } else {
            delete segment;
        }

        delete l;
    }
}

void RS_Hatch::drawSolidFill(RS_Painter* painter) {
    QPainterPath path = createSolidFillPath(painter);
    painter->fillPath(path, QBrush(painter->getPen().getColor().toQColor()));
}

QPainterPath RS_Hatch::createSolidFillPath(RS_Painter* painter) const {
    QPainterPath path;

    for (unsigned i = 0; i < count(); ++i) {
        RS_Entity* loop = entityAt(i);
        if (loop->rtti() == RS2::EntityContainer) {
            RS_EntityContainer* l = static_cast<RS_EntityContainer*>(loop);
            RS_Vector start = l->getStartpoint();
            path.moveTo(painter->toGuiPointF(start));

            for (unsigned j = 0; j < l->count(); ++j) {
                RS_Entity* edge = l->entityAt(j);
                if (edge->isAtomic()) {
                    RS_AtomicEntity* ae = dynamic_cast<RS_AtomicEntity*>(edge);
                    if (ae) {
                        path.lineTo(painter->toGuiPointF(ae->getEndpoint()));
                    }
                } else if (edge->rtti() == RS2::EntityArc) {
                    RS_Arc* arc = static_cast<RS_Arc*>(edge);
                    double r = arc->getRadius();
                    RS_Vector center = painter->toGui(arc->getCenter());
                    double a1 = arc->getAngle1();
                    double a2 = arc->getAngle2();
                    bool reversed = arc->isReversed();
                    path.arcTo(center.x - r, center.y - r, 2 * r, 2 * r, RS_Math::rad2deg(a1), RS_Math::rad2deg(a2 - a1) * (reversed ? -1 : 1));
                } else if (edge->rtti() == RS2::EntityCircle) {
                    RS_Circle* circle = static_cast<RS_Circle*>(edge);
                    RS_Vector center = painter->toGui(circle->getCenter());
                    double r = painter->toGuiDX(circle->getRadius());
                    path.addEllipse(center.x - r, center.y - r, 2 * r, 2 * r);
                } else if (edge->rtti() == RS2::EntityEllipse) {
                    RS_Ellipse* ellipse = static_cast<RS_Ellipse*>(edge);
                    RS_Vector center = painter->toGui(ellipse->getCenter());
                    double majorR = painter->toGuiDX(ellipse->getMajorRadius());
                    double minorR = painter->toGuiDY(ellipse->getMinorRadius());
                    double angle = ellipse->getAngle();
                    QTransform trans;
                    trans.translate(center.x, center.y);
                    trans.rotate(RS_Math::rad2deg(angle));
                    trans.translate(-center.x, -center.y);
                    QPainterPath ellPath;
                    if (ellipse->isEllipticArc()) {
                        double a1 = ellipse->getAngle1();
                        double a2 = ellipse->getAngle2();
                        bool reversed = ellipse->isReversed();
                        ellPath.arcTo(-majorR, -minorR, 2 * majorR, 2 * minorR, RS_Math::rad2deg(a1), RS_Math::rad2deg(a2 - a1) * (reversed ? -1 : 1));
                    } else {
                        ellPath.addEllipse(-majorR, -minorR, 2 * majorR, 2 * minorR);
                    }
                    path.addPath(trans.map(ellPath));
                } else if (edge->rtti() == RS2::EntitySpline) {
                    RS_Spline* spline = static_cast<RS_Spline*>(edge);
                    std::vector<RS_Vector> points;
                    spline->fillStrokePoints(10, points); // 10 segments
                    for (size_t k = 1; k < points.size(); ++k) {
                        path.lineTo(painter->toGuiPointF(points[k]));
                    }
                    break;
                }
            }
            path.closeSubpath();
        }
    }

    return path;
}

void RS_Hatch::debugOutPath(const QPainterPath &tmpPath) const {
    RS_DEBUG->print("QPainterPath element count: %d", tmpPath.elementCount());
    for (int i = 0; i < tmpPath.elementCount(); ++i) {
        QPainterPath::Element e = tmpPath.elementAt(i);
        RS_DEBUG->print("element %d: %g/%g type: %d", i, e.x, e.y, e.type);
    }
}
