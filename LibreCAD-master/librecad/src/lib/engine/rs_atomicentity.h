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

struct RS_CircleData;

/**
 * Class representing an atomic entity
 * Typical atomic entities: RS_Line, RS_Arc, RS_Circle, RS_Ellipse
 *
 * @author Andrew Mustun
 */
class RS_AtomicEntity : public RS_Entity {

public:
    /**
     * Constructor.
     */
	RS_AtomicEntity(RS_EntityContainer* parent=nullptr);

    /**
     * @return false because entities made from subclasses are
     *  atomic entities.
     */
	bool isContainer() const override;

    /**
     * @return true because entities made from subclasses are
     *  atomic entities.
     */
	bool isAtomic() const override;

    /**
     * @return Always 1 for atomic entities.
     */
	unsigned count() const override;

    /**
     * @return Always 1 for atomic entities.
     */
	unsigned countDeep() const override;

    /**
     * Implementation must return the endpoint of the entity or
     * an invalid vector if the entity has no endpoint.
     */
	RS_Vector getEndpoint() const override;

    /**
     * Implementation must return the startpoint of the entity or
     * an invalid vector if the entity has no startpoint.
     */
	RS_Vector getStartpoint() const override;

    /**
     * Implementation must return the angle in which direction the entity starts.
     */
	double getDirection1() const override;

    /**
     * Implementation must return the angle in which direction the entity starts the opposite way.
     */
	double getDirection2() const override;

	RS_Vector getCenter() const override;
	double getRadius() const override;
	/**
    * return the nearest center for snapping
    * @param coord Coordinate (typically a mouse coordinate)
    * @param dist Pointer to a value which will contain the measured
    * distance between 'coord' and the closest center point. The passed
    * pointer can also be NULL in which case the distance will be
    * lost.
    *
    * @return The closest center point.
    */
   RS_Vector getNearestCenter(const RS_Vector& /*coord*/,
									  double* /*dist*/) const override;

    /**
     * (De-)selects startpoint.
     */
	virtual void setStartpointSelected(bool select);

    /**
     * (De-)selects endpoint.
     */
	virtual void setEndpointSelected(bool select);
	virtual bool isTangent(const RS_CircleData& /* circleData */) const;

    /**
     * @return True if the entities startpoint is selected.
     */
	bool isStartpointSelected() const;

    /**
     * @return True if the entities endpoint is selected.
     */
	bool isEndpointSelected() const;

	void revertDirection() override;

    /**
     * Implementation must create offset of the entity to
     * the given direction and distance
     */
	bool offset(const RS_Vector& /*position*/, const double& /*distance*/) override;

    /**
     * Implementation must move the startpoint of the entity to
     * the given position.
     */
	virtual void moveStartpoint(const RS_Vector& /*pos*/);

    /**
     * Implementation must move the endpoint of the entity to
     * the given position.
     */
	virtual void moveEndpoint(const RS_Vector& /*pos*/);

    /**
     * Implementation must trim the startpoint of the entity to
     * the given position.
     */
	virtual void trimStartpoint(const RS_Vector& pos);

    /**
     * Implementation must trim the endpoint of the entity to
     * the given position.
     */
	virtual void trimEndpoint(const RS_Vector& pos);

    /**
     * Implementation must return which ending of the entity will
     * be trimmed if 'coord' is the coordinate chosen to indicate the
     * trim entity and 'trimPoint' is the point to which the entity will
     * be trimmed.
     */
	virtual RS2::Ending getTrimPoint(const RS_Vector& /*coord*/,
									 const RS_Vector& /*trimPoint*/);

    /**
     * Implementation must trim the entity in the case of multiple
     * intersections and return the trimPoint
     * trimCoord indicts the trigger trim position
     * trimSol contains intersections
     * */
	virtual RS_Vector prepareTrim(const RS_Vector& /*trimCoord*/,
								  const RS_VectorSolutions& /*trimSol*/);

	virtual void reverse();

	void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) override;
};


#endif
