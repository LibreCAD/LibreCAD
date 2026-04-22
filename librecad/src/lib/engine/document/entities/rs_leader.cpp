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

#include "rs_leader.h"

#include<iostream>

#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_solid.h"

// fixme - sand - RS_LEADER should be reworked, add support of moving ref points, properties, probably various rendering types
/**
 * Constructor.
 */
RS_Leader::RS_Leader(RS_EntityContainer* parent)
    : RS_Dimension(parent, RS_DimensionData()) {
}

/**
 * Constructor.
 * @param parent
 * @param d Leader data
 */
RS_Leader::RS_Leader(RS_EntityContainer* parent, const RS_LeaderData& d)
    : RS_Dimension(parent, RS_DimensionData()), m_data(d) {
}

RS_Entity* RS_Leader::clone() const {
    const RS_LeaderData data(hasArrowHead(), m_data.styleName);
    // fixme - setup other parts of data?
    auto* p = new RS_Leader(nullptr, data);
    p->setOwner(isOwner());

    p->setPen(getPen(false));
    p->setLayer(m_layer);

    p->m_empty = true;
    p->m_data.arrowHead = m_data.arrowHead;
    for (const auto v : m_data.vertexes) {
        p->m_data.vertexes << v;
    }
    p->update();
    return p;
}

void RS_Leader::doUpdateDim() {
    // fixme - sand - rework leader!
}


void RS_Leader::addArrowHead(const RS_Vector& start, const RS_Vector& end) {
    // fixme - rework this method, use arrowhead based on style, similarly to normal dimensions!!!
    auto* s = new RS_Solid(this, RS_SolidData());
    s->shapeArrow(start, end.angleTo(start), getGraphicVariableDouble("$DIMASZ", 2.5) * getGraphicVariableDouble("$DIMSCALE", 1.0));
    s->setPen(RS_Pen(RS2::FlagInvalid));
    s->setLayer(nullptr);
    RS_EntityContainer::addEntity(s);
}

/**
 * Implementation of update. Updates the arrow.
 */
void RS_Leader::update() {
    if (isDeleted()) {
        return;
    }
    clear();
    m_empty = true;
    if (!m_data.vertexes.empty()) {
        for (const auto v: std::as_const(m_data.vertexes)) {
            doAddVertex(v);
        }
        m_dimGenericData.definitionPoint = m_data.vertexes.first();
    }
    calculateBorders();
}

RS_VectorSolutions RS_Leader::getRefPoints() const {
    auto result = RS_VectorSolutions();
    result.clear();
    for (const auto& v : m_data.vertexes) {
        result.push_back(v);
    }
    return result;
}

void RS_Leader::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    for (auto& v: m_data.vertexes) {
        if (v == ref) {
            v.move(offset);
        }
    }
    update();
}

void RS_Leader::moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (isSelected()) {
        moveRef(ref, offset);
    }
}

RS_Entity* RS_Leader::doAddVertex(const RS_Vector& v) {
    RS_Entity* entity{nullptr};
    if (m_empty) {
        m_dimGenericData.definitionPoint = v;
        m_empty = false;
    }
    else {
        // add line to the leader:
        RS_Vector startPoint;
        const bool firstSegment = count() == 0;
        if (firstSegment) {
            startPoint = m_dimGenericData.definitionPoint;
        }
        else {
            startPoint = getEntityList().last()->getEndpoint();
        }

        if (firstSegment && hasArrowHead()) {
            addArrowHead(startPoint, v);
        }

        const auto line = new RS_Line{this, {startPoint,v}};
        auto pen = RS_Pen();
        pen.setLineType(RS2::LineByBlock);
        pen.setWidth(RS2::WidthByBlock);
        pen.setColor(RS_Color(RS2::FlagByBlock));
        line->setPen(pen);
        RS_EntityContainer::addEntity(line);
        entity = line;
    }
    return entity;
}

/**
 * Adds a vertex from the endpoint of the last element or
 * sets the startpoint to the point 'v'.
 *
 * The very first vertex added is the starting point.
 *
 * @param v vertex coordinate
 *
 * @return Pointer to the entity that was added or nullptr if this
 *         was the first vertex added.
 */
RS_Entity* RS_Leader::addVertex(const RS_Vector& v) {
    m_data.vertexes.push_back(v);
    RS_Entity* entity = doAddVertex(v);
    return entity;
}

/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() instead.
 */
void RS_Leader::addEntity(const RS_Entity* entity) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Leader::addEntity:" " should never be called");

    if (entity == nullptr) {
        return;
    }

    delete entity;
}

void RS_Leader::move(const RS_Vector& offset) {
   for (auto& v: m_data.vertexes) {
        v.move(offset);
    }
    update();
}

void RS_Leader::rotate(const RS_Vector& center, const double angle) {
    for (auto& v: m_data.vertexes) {
        v.rotate(center, angle);
    }
    update();
}

void RS_Leader::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
     for (auto& v: m_data.vertexes) {
        v.rotate(center, angleVector);
     }
    update();
}

void RS_Leader::scale(const RS_Vector& center, const RS_Vector& factor) {
     for (auto& v: m_data.vertexes) {
        v.scale(center, factor);
     }
     update();
}

void RS_Leader::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
     for (auto& v: m_data.vertexes) {
        v.mirror(axisPoint1, axisPoint2);
     }
     update();
}

void RS_Leader::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    const RS_Vector vLow{std::min(firstCorner.x, secondCorner.x), std::min(firstCorner.y, secondCorner.y)};
    const RS_Vector vHigh{std::max(firstCorner.x, secondCorner.x), std::max(firstCorner.y, secondCorner.y)};
    for (auto& v : m_data.vertexes) {
        if (getStartpoint().isInWindowOrdered(vLow, vHigh)) {
            v.move(offset);
        }
    }
    update();
}

/**
 * Dumps the leader's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Leader& l) {
    os << " Leader: " << l.getData() << " {\n";
    auto asContainer = static_cast<const RS_EntityContainer&>(l);
    os << asContainer;
    os << "\n}\n";
    return os;
}

std::ostream& operator <<(std::ostream& os, const RS_LeaderData& /*ld*/) {
    os << "(Leader)";
    return os;
}
