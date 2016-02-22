/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**

Copyright (C) 2012-2015 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "rs_atomicentity.h"
RS_AtomicEntity::RS_AtomicEntity(RS_EntityContainer* parent) : RS_Entity(parent) {}

bool RS_AtomicEntity::isContainer() const {
	return false;
}

/**
 * @return true because entities made from subclasses are
 *  atomic entities.
 */
bool RS_AtomicEntity::isAtomic() const {
	return true;
}

/**
 * @return Always 1 for atomic entities.
 */
unsigned int RS_AtomicEntity::count() const{
	return 1;
}

/**
 * @return Always 1 for atomic entities.
 */
unsigned int RS_AtomicEntity::countDeep() const{
	return 1;
}

/**
 * Implementation must return the endpoint of the entity or
 * an invalid vector if the entity has no endpoint.
 */
RS_Vector RS_AtomicEntity::getEndpoint() const {
	return RS_Vector(false);
}

/**
 * Implementation must return the startpoint of the entity or
 * an invalid vector if the entity has no startpoint.
 */
RS_Vector RS_AtomicEntity::getStartpoint() const {
	return RS_Vector(false);
}

/**
 * Implementation must return the angle in which direction the entity starts.
 */
double RS_AtomicEntity::getDirection1() const {
	return 0.0;
}

/**
 * Implementation must return the angle in which direction the entity starts the opposite way.
 */
double RS_AtomicEntity::getDirection2() const {
	return 0.0;
}
RS_Vector RS_AtomicEntity::getCenter() const {
	return RS_Vector(false);
}
double RS_AtomicEntity::getRadius() const {
	   return 0.;
   }
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
RS_Vector RS_AtomicEntity::getNearestCenter(const RS_Vector& /*coord*/,
								  double* /*dist*/) const{
	return RS_Vector(false);
}

/**
 * (De-)selects startpoint.
 */
void RS_AtomicEntity::setStartpointSelected(bool select) {
	if (select) {
		setFlag(RS2::FlagSelected1);
	} else {
		delFlag(RS2::FlagSelected1);
	}
}

/**
 * (De-)selects endpoint.
 */
void RS_AtomicEntity::setEndpointSelected(bool select) {
	if (select) {
		setFlag(RS2::FlagSelected2);
	} else {
		delFlag(RS2::FlagSelected2);
	}
}
bool RS_AtomicEntity::isTangent(const RS_CircleData& /* circleData */) const{
	return false;
}

/**
 * @return True if the entities startpoint is selected.
 */
bool RS_AtomicEntity::isStartpointSelected() const {
	return getFlag(RS2::FlagSelected1);
}

/**
 * @return True if the entities endpoint is selected.
 */
bool RS_AtomicEntity::isEndpointSelected() const {
	return getFlag(RS2::FlagSelected2);
}

void RS_AtomicEntity::revertDirection(){}

/**
 * Implementation must create offset of the entity to
 * the given direction and distance
 */
bool RS_AtomicEntity::offset(const RS_Vector& /*position*/, const double& /*distance*/) {return false;}

/**
 * Implementation must move the startpoint of the entity to
 * the given position.
 */
void RS_AtomicEntity::moveStartpoint(const RS_Vector& /*pos*/) {}

/**
 * Implementation must move the endpoint of the entity to
 * the given position.
 */
void RS_AtomicEntity::moveEndpoint(const RS_Vector& /*pos*/) {}

/**
 * Implementation must trim the startpoint of the entity to
 * the given position.
 */
void RS_AtomicEntity::trimStartpoint(const RS_Vector& pos) {
	moveStartpoint(pos);
}

/**
 * Implementation must trim the endpoint of the entity to
 * the given position.
 */
void RS_AtomicEntity::trimEndpoint(const RS_Vector& pos) {
	moveEndpoint(pos);
}

/**
 * Implementation must return which ending of the entity will
 * be trimmed if 'coord' is the coordinate chosen to indicate the
 * trim entity and 'trimPoint' is the point to which the entity will
 * be trimmed.
 */
RS2::Ending RS_AtomicEntity::getTrimPoint(const RS_Vector& /*coord*/,
								 const RS_Vector& /*trimPoint*/) {
	return RS2::EndingNone;
}

/**
 * Implementation must trim the entity in the case of multiple
 * intersections and return the trimPoint
 * trimCoord indicts the trigger trim position
 * trimSol contains intersections
 * */
RS_Vector RS_AtomicEntity::prepareTrim(const RS_Vector& /*trimCoord*/,
							  const RS_VectorSolutions& /*trimSol*/) {
	return RS_Vector(false);
}

void RS_AtomicEntity::reverse() {}

void RS_AtomicEntity::moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) {
	if (isSelected()) {
		moveRef(ref, offset);
	}
}
