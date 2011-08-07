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


#ifndef RS_ATOMICENTITY_H
#define RS_ATOMICENTITY_H

#include "rs_entity.h"


/**
 * Class representing a tree of entities.
 * Typical entity containers are graphics, polylines, groups, texts, ...)
 *
 * @author Andrew Mustun
 */
class RS_AtomicEntity : public RS_Entity {

public:
    /**
     * Construtor.
     */
    RS_AtomicEntity(RS_EntityContainer* parent=NULL) : RS_Entity(parent) {}
    /**
     * Destrutor.
     */
    virtual ~RS_AtomicEntity() {}

    /**
     * @return false because entities made from subclasses are
     *  atomic entities.
     */
    virtual bool isContainer() const {
        return false;
    }

    /**
     * @return true because entities made from subclasses are
     *  atomic entities.
     */
    virtual bool isAtomic() const {
        return true;
    }

    /**
     * @return Always 1 for atomic entities.
     */
    virtual unsigned long int count() {
        return 1;
    }

    /**
     * @return Always 1 for atomic entities.
     */
    virtual unsigned long int countDeep() {
        return 1;
    }

    /**
     * Implementation must return the endpoint of the entity or
     * an invalid vector if the entity has no endpoint.
     */
    virtual RS_Vector getEndpoint() const {
        return RS_Vector(false);
    }

    /**
     * Implementation must return the startpoint of the entity or
     * an invalid vector if the entity has no startpoint.
     */
    virtual RS_Vector getStartpoint() const {
        return RS_Vector(false);
    }

    /**
     * Implementation must return the angle in which direction the entity starts.
     */
    virtual double getDirection1() const {
        return 0.0;
    }

    /**
     * Implementation must return the angle in which direction the entity starts the opposite way.
     */
    virtual double getDirection2() const {
        return 0.0;
    }

    /**
     * (De-)selects startpoint.
     */
    virtual void setStartpointSelected(bool select) {
        if (select) {
            setFlag(RS2::FlagSelected1);
        } else {
            delFlag(RS2::FlagSelected1);
        }
    }

    /**
     * (De-)selects endpoint.
     */
    virtual void setEndpointSelected(bool select) {
        if (select) {
            setFlag(RS2::FlagSelected2);
        } else {
            delFlag(RS2::FlagSelected2);
        }
    }

    /**
     * @return True if the entities startpoint is selected.
     */
    bool isStartpointSelected() const {
        return getFlag(RS2::FlagSelected1);
    }

    /**
     * @return True if the entities endpoint is selected.
     */
    bool isEndpointSelected() const {
        return getFlag(RS2::FlagSelected2);
    }

    /**
     * Implementation must move the startpoint of the entity to
     * the given position.
     */
    virtual void moveStartpoint(const RS_Vector& /*pos*/) {}

    /**
     * Implementation must move the endpoint of the entity to
     * the given position.
     */
    virtual void moveEndpoint(const RS_Vector& /*pos*/) {}

    /**
     * Implementation must trim the startpoint of the entity to
     * the given position.
     */
    virtual void trimStartpoint(const RS_Vector& pos) {
        moveStartpoint(pos);
    }

    /**
     * Implementation must trim the endpoint of the entity to
     * the given position.
     */
    virtual void trimEndpoint(const RS_Vector& pos) {
        moveEndpoint(pos);
    }

    /**
     * Implementation must return which ending of the entity will
     * be trimmed if 'trimCoord' is the coordinate chosen to indicate the
     * trim entity and 'trimPoint' is the point to which the entity will
     * be trimmed.
     */
    virtual RS2::Ending getTrimPoint(const RS_Vector& /*trimCoord*/,
                                     const RS_Vector& /*trimPoint*/) {
        return RS2::EndingNone;
    }

    /**
     * Implementation must trim the entity in the case of multiple
     * intersections and return the trimPoint
     * trimCoord indicts the trigger trim position
     * trimSol contains intersections
     * */
    virtual RS_Vector prepareTrim(const RS_Vector& /*trimCoord*/,
                                  const RS_VectorSolutions& /*trimSol*/) {
        return RS_Vector(false);
    }

    virtual void reverse() { }

    virtual void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) {
        if (isSelected()) {
            moveRef(ref, offset);
        }
    }
}
;


#endif
