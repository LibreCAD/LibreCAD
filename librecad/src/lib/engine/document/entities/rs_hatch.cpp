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

#include <algorithm>
#include <iostream>
#include <set>
#include <memory>  // For std::shared_ptr

#include <QPainterPath>
#include "lc_looputils.h"
#include "lc_rect.h"
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
#include "lc_containertraverser.h"

namespace {

double angularDist(double a, double startAngle, bool reversed) {
    return reversed ? RS_Math::correctAngle(startAngle - a) : RS_Math::correctAngle(a - startAngle);
}

void pr(RS_EntityContainer* loop) {
    if (loop == nullptr) {
        LC_ERR << "-nullptr-;";
        return;
    }
    LC_ERR << "( id=" << loop->getId() << "| ";
    for (auto* e : *loop) {
        if (e && e->rtti() == RS2::EntityContainer) {
            pr(static_cast<RS_EntityContainer*>(e));
        } else if (e) {
            auto vp0 = static_cast<RS_AtomicEntity*>(e)->getStartpoint();
            auto vp1 = static_cast<RS_AtomicEntity*>(e)->getEndpoint();
            LC_ERR << ", " << e->getId() << ": " << vp0.x << ", " << vp0.y << " :: " << vp1.x << ", " << vp1.y;
        }
    }
    LC_ERR << " |" << loop->getId() << " )";
}

void avoidZeroLength(RS_EntityContainer& container) {
    std::set<RS_Entity*> toCleanUp;
    for (RS_Entity* e : container) {
        if (e != nullptr && e->isContainer())
            avoidZeroLength(*static_cast<RS_EntityContainer*>(e));
        else if (e != nullptr && RS_Math::equal(e->getLength(), 0.)) {
            toCleanUp.insert(e);
        }
    }
    for (RS_Entity* e : toCleanUp)
        container.removeEntity(e);
}

}

RS_HatchData::RS_HatchData(bool solid, double scale, double angle, QString pattern)
    : solid{solid}, scale{scale}, angle{angle}, pattern{std::move(pattern)} {
}

std::ostream& operator<<(std::ostream& os, const RS_HatchData& td) {
    os << "(" << td.pattern.toLatin1().data() << ")";
    return os;
}

RS_Hatch::RS_Hatch(RS_EntityContainer* parent, const RS_HatchData& d)
    : RS_EntityContainer(parent), data(d) {
    // Initialize m_orderedLoops with owning behavior to match RS_Hatch's ownership
    m_orderedLoops = std::make_shared<std::vector<LC_LoopUtils::LC_Loops>>(true);
    // Explicitly set ownership for this container
    setOwner(true);
}

RS_Entity* RS_Hatch::clone() const {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone()");
    auto* t = new RS_Hatch(*this);
    t->setOwner(isOwner());
    t->detach();
    // Deep copy shared members

    t->update();
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone(): OK");
    return t;
}

bool RS_Hatch::validate() {
    bool ret = true;

    // loops:
    if (needOptimization && count() > 0) {
        try {
            // copy to a container
            RS_EntityContainer container{nullptr, false};
            std::copy(begin(), end(), std::back_inserter(container));
            lc::LC_ContainerTraverser traverser{container, RS2::ResolveAll};
            for(RS_Entity* en: traverser.entities())
                if (en->isAtomic())
                    container.addEntity(en);
            LC_LoopUtils::LoopOptimizer optimizer{container};
            m_orderedLoops = optimizer.GetResults();
            if (!m_orderedLoops) {
                throw std::runtime_error("Optimization failed: empty loops");
            }
        } catch (const std::exception& e) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "Optimization error: %s", e.what());
            updateError = HATCH_INVALID_CONTOUR;
            updateRunning = false;
            return false;
        }
        needOptimization = false;
    }

    return ret;
}

int RS_Hatch::countLoops() const {
    if (data.solid) {
        return count();
    } else {
        return count() - 1;
    }
}

bool RS_Hatch::isContainer() const {
    return !isSolid();
}

void RS_Hatch::calculateBorders() {
    RS_DEBUG->print("RS_Hatch::calculateBorders");

    activateContour(true);
    RS_EntityContainer::calculateBorders();
    RS_DEBUG->print("RS_Hatch::calculateBorders: size: %f,%f", getSize().x, getSize().y);
    activateContour(false);
}

void RS_Hatch::update() {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update");

    updateError = HATCH_OK;
    if (updateRunning) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch in updating process");
        return;
    }

    if (updateEnabled == false) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: skip hatch forbidden to update");
        return;
    }

    // Clear cached path upfront
    m_solidPath = std::make_shared<std::vector<QPainterPath>>();

    // Save attributes for the current hatch
    RS_Layer* hatch_layer = this->getLayer();
    RS_Pen hatch_pen = this->getPen();

    // Delete old hatch
    hatch = std::make_shared<RS_EntityContainer>(nullptr, true);

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

    // Optimize loops if needed
    if (needOptimization) {
        try {
            // copy to a container
            RS_EntityContainer container{nullptr, false};
            std::copy(begin(), end(), std::back_inserter(container));
            lc::LC_ContainerTraverser traverser{container, RS2::ResolveAll};
            for(RS_Entity* en: traverser.entities())
                if (en->isAtomic())
                    container.addEntity(en);
            LC_LoopUtils::LoopOptimizer optimizer{container};
            m_orderedLoops = optimizer.GetResults();
            if (!m_orderedLoops || m_orderedLoops->empty()) {
                throw std::runtime_error("Optimization failed: empty loops");
            }
        } catch (const std::exception& e) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "Optimization error: %s", e.what());
            updateError = HATCH_INVALID_CONTOUR;
            updateRunning = false;
            return;
        }
        needOptimization = false;
    }

    // Create new hatch container
    hatch->setPen(hatch_pen);
    hatch->setLayer(hatch_layer);
    hatch->setFlag(RS2::FlagTemp);
    hatch->setOwner(true);  // Explicitly set hatch to own its child entities

    if (data.solid) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: processing solid hatch");
        // Generate and cache the shared QPainterPath for solid fill
        m_solidPath = std::make_shared<std::vector<QPainterPath>>();
        std::transform(m_orderedLoops->begin(), m_orderedLoops->end(), std::back_inserter(*m_solidPath), [](const LC_LoopUtils::LC_Loops& loop){return loop.getPainterPath();});
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: processing pattern hatch");
        // Search for pattern
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern");
        std::unique_ptr<RS_Pattern> pat = RS_PATTERNLIST->requestPattern(data.pattern);
        if (pat == nullptr) {
            updateRunning = false;
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: requesting pattern: %s not found", data.pattern.toUtf8().constData());
            updateError = HATCH_PATTERN_NOT_FOUND;
            return;
        } else {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: requesting pattern: OK");
            // Make a working copy of hatch pattern
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern");
            pat.reset((RS_Pattern*)pat->clone());
            if (pat != nullptr) {
                RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: cloning pattern: OK");
                pat->setOwner(true);  // Ensure cloned pattern owns its entities
            } else {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: error while cloning hatch pattern");
                return;
            }
        }

        // Scale and rotate pattern
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern");
        pat->scale(getCenter(), RS_Vector(data.scale, data.scale));
        pat->rotate(getCenter(), data.angle);
        pat->calculateBorders();
        forcedCalculateBorders();
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: scaling pattern: OK");

        // Check pattern sizes for sanity
        RS_Vector pSize = pat->getSize();
        RS_Vector cSize = getSize();
        if (cSize.x < 1.0e-6 || cSize.y < 1.0e-6 || pSize.x < 1.0e-6 || pSize.y < 1.0e-6 ||
            cSize.x > RS_MAXDOUBLE - 1 || cSize.y > RS_MAXDOUBLE - 1 ||
            pSize.x > RS_MAXDOUBLE - 1 || pSize.y > RS_MAXDOUBLE - 1) {
            updateRunning = false;
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size or pattern size too small");
            updateError = HATCH_TOO_SMALL;
            return;
        } else if (cSize.x * cSize.y / (pSize.x * pSize.y) > 1e4) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: contour size too large or pattern size too small");
            updateError = HATCH_AREA_TOO_BIG;
            return;
        }

        // Use LC_Loops to trim pattern entities and add to hatch
        hatch->clear();
        for(const LC_LoopUtils::LC_Loops& loop: *m_orderedLoops) {
            auto trimmed = loop.trimPatternEntities(*pat);
            for (auto* e : *trimmed) {
                e->setPen(hatch_pen);
                e->setLayer(hatch_layer);
                e->reparent(hatch.get());
                e->setFlag(RS2::FlagHatchChild);
                hatch->addEntity(e);  // hatch takes ownership
            }
            trimmed->setOwner(false);
        }
    }

    forcedCalculateBorders();
    activateContour(false);
    updateRunning = false;
    m_updated = true;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: OK");
}

void RS_Hatch::activateContour(bool on) {
    RS_DEBUG->print("RS_Hatch::activateContour: %d", (int)on);
    for (RS_Entity* e : *this) {
        if (!e->isUndone()) {
            if (!e->getFlag(RS2::FlagTemp)) {
                RS_DEBUG->print("RS_Hatch::activateContour: set visible");
                e->setVisible(on);
            } else {
                RS_DEBUG->print("RS_Hatch::activateContour: entity temp");
            }
        } else {
            RS_DEBUG->print("RS_Hatch::activateContour: entity undone");
        }
    }
    RS_DEBUG->print("RS_Hatch::activateContour: OK");
}

void RS_Hatch::draw(RS_Painter* painter) {
    if (data.solid) {
        if (m_solidPath) {
            // Use cached shared path for filling
            const QBrush brush(painter->brush());
            const RS_Pen pen = painter->getPen();
            QBrush fillBrush = brush;
            fillBrush.setColor(pen.getColor());
            fillBrush.setStyle(Qt::SolidPattern);
            painter->save();
            painter->setBrush(fillBrush);
            QTransform transform = painter->getToGuiTransform();
            painter->setTransform(transform, false);
            for(const QPainterPath& path: *m_solidPath)
                painter->drawPath(path);  // Dereference
            painter->restore();
        } else {
            // Fallback: Regenerate if cache miss (shouldn't happen)
            LC_ERR<<__func__<<"(): RS_Hatch solid fill failure: no QPainterPath created";
        }
    } else {
        // Existing: Draw hatch children for patterns
        if (hatch) {
            for (RS_Entity* se : *hatch) {
                painter->drawEntity(se);
            }
        }
    }
}



void RS_Hatch::debugOutPath(const QPainterPath& tmpPath) const {
    int c = tmpPath.elementCount();
    for (int i = 0; i < c; i++) {
        const QPainterPath::Element& element = tmpPath.elementAt(i);
        LC_ERR << "i " << i << "(" << element.x << "," << element.y << ") Line To " << element.isLineTo()
               << " Move To: " << element.isMoveTo() << " Is Curve:" << element.isCurveTo();
    }
}

double RS_Hatch::getTotalArea() const {
    if (m_area + RS_Math::ulp(m_area) < RS_MAXDOUBLE) return m_area;
    if (needOptimization) {
        const_cast<RS_Hatch*>(this)->update();  // Force update in non-const context if needed
    }
    m_area = std::accumulate(m_orderedLoops->begin(), m_orderedLoops->end(), 0, [](double area, const LC_LoopUtils::LC_Loops& loop) {
        return area + loop.getTotalArea();
    } );
    return m_area;
}

double RS_Hatch::getDistanceToPoint(
    const RS_Vector& coord,
    RS_Entity** entity,
    RS2::ResolveLevel level,
    double solidDist) const {
    if (data.solid == true) {
        if (entity) {
            *entity = const_cast<RS_Hatch*>(this);
        }
        bool onContour;
        if (RS_Information::isPointInsideContour(coord, const_cast<RS_Hatch*>(this), &onContour)) {
            return solidDist;
        }
        return RS_MAXDOUBLE;
    } else {
        return RS_EntityContainer::getDistanceToPoint(coord, entity, level, solidDist);
    }
}

void RS_Hatch::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    m_solidPath.reset();  // Invalidate cache
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, double angle) {
    RS_EntityContainer::rotate(center, angle);
    data.angle = RS_Math::correctAngle(data.angle + angle);
    m_solidPath.reset();  // Invalidate cache
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle + angleVector.angle());
    m_solidPath.reset();  // Invalidate cache
    update();
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    data.scale *= factor.x;
    m_solidPath.reset();  // Invalidate cache
    needOptimization = true;
    m_area = RS_MAXDOUBLE;
    m_updated = false;
    update();
}

void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    double ang = axisPoint1.angleTo(axisPoint2);
    data.angle = RS_Math::correctAngle(data.angle + ang * 2.0);
    m_solidPath.reset();  // Invalidate cache
    update();
}

void RS_Hatch::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    m_solidPath.reset();  // Invalidate cache
    update();
}

void RS_Hatch::setPattern(const QString& pattern)
{
    data.pattern = pattern;
}

std::ostream& operator << (std::ostream& os, const RS_Hatch& p) {
    os << " Hatch: " << p.getData() << "\n";
    return os;
}
