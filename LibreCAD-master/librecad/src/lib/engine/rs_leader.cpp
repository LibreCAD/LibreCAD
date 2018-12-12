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
#include "rs_leader.h"

#include "rs_debug.h"
#include "rs_line.h"
#include "rs_solid.h"


/**
 * Constructor.
 */
RS_Leader::RS_Leader(RS_EntityContainer* parent)
		:RS_EntityContainer(parent)
		,empty(true)
{
}


/**
 * Constructor.
 * @param d Leader data
 */
RS_Leader::RS_Leader(RS_EntityContainer* parent,
                     const RS_LeaderData& d)
        :RS_EntityContainer(parent), data(d) {
    empty = true;
}

RS_Entity* RS_Leader::clone() const{
	RS_Leader* p = new RS_Leader(*this);
	p->setOwner(isOwner());
	p->initId();
	p->detach();
	return p;
}

/**
 * Implementation of update. Updates the arrow.
 */
void RS_Leader::update() {

    // find and delete arrow:
	for(auto e: entities){
        if (e->rtti()==RS2::EntitySolid) {
            removeEntity(e);
            break;
        }
    }

        if (isUndone()) {
                return;
        }

    RS_Entity* fe = firstEntity();
    if (fe && fe->isAtomic()) {
        RS_Vector p1 = ((RS_AtomicEntity*)fe)->getStartpoint();
        RS_Vector p2 = ((RS_AtomicEntity*)fe)->getEndpoint();

        // first entity must be the line which gets the arrow:
        if (hasArrowHead()) {
            RS_Solid* s = new RS_Solid(this, RS_SolidData());
            s->shapeArrow(p1,
                          p2.angleTo(p1),
                          getGraphicVariableDouble("$DIMASZ", 2.5)* getGraphicVariableDouble("$DIMSCALE", 1.0));
            s->setPen(RS_Pen(RS2::FlagInvalid));
			s->setLayer(nullptr);
            RS_EntityContainer::addEntity(s);
        }
    }
    calculateBorders();
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

	RS_Entity* entity{nullptr};
	static RS_Vector last = RS_Vector{false};

    if (empty) {
        last = v;
        empty = false;
    } else {
        // add line to the leader:
		entity = new RS_Line{this, {last, v}};
        entity->setPen(RS_Pen(RS2::FlagInvalid));
		entity->setLayer(nullptr);
        RS_EntityContainer::addEntity(entity);

                if (count()==1 && hasArrowHead()) {
                        update();
                }

        last = v;
    }

    return entity;
}



/**
 * Reimplementation of the addEntity method for a normal container.
 * This reimplementation deletes the given entity!
 *
 * To add entities use addVertex() instead.
 */
void RS_Leader::addEntity(RS_Entity* entity) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Leader::addEntity:"
                    " should never be called");

	if (!entity) return;

	delete entity;
}



void RS_Leader::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
    update();
}



void RS_Leader::rotate(const RS_Vector& center, const double& angle) {
    RS_EntityContainer::rotate(center, angle);
    update();
}


void RS_Leader::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
    update();
}


void RS_Leader::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_EntityContainer::scale(center, factor);
    update();
}



void RS_Leader::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_EntityContainer::mirror(axisPoint1, axisPoint2);
    update();
}


void RS_Leader::stretch(const RS_Vector& firstCorner,
                       const RS_Vector& secondCorner,
                       const RS_Vector& offset) {

    RS_EntityContainer::stretch(firstCorner, secondCorner, offset);
    update();
}

/**
 * Dumps the leader's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Leader& l) {
    os << " Leader: " << l.getData() << " {\n";

    os << (RS_EntityContainer&)l;

    os << "\n}\n";

    return os;
}

std::ostream& operator << (std::ostream& os,
                                      const RS_LeaderData& /*ld*/) {
        os << "(Leader)";
        return os;
}

