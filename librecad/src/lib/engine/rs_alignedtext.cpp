/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Douglas B. Geiger ()
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
#include<cmath>
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_alignedtext.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_information.h"

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

double TextOffset(RS_Entity *_entity, double offset, bool above)
{
	double
		textOffset(0.0),
		height(0.0);

	if (_entity->rtti() == RS2::EntityAlignedText)
		textOffset = TextOffset(dynamic_cast<RS_AlignedText *>(_entity)->getTextEntity(), offset, above);
	else if (_entity->rtti() == RS2::EntityText)
	{
		height = dynamic_cast<RS_Text *>(_entity)->getHeight();
		switch (dynamic_cast<RS_Text *>(_entity)->getVAlign())
		{
		case RS_TextData::VABaseline:
			if (above)
				textOffset = offset;
			else
				textOffset = -offset - height;
			break;
		case RS_TextData::VABottom:
			if (above)
				textOffset = offset;
			else
				textOffset = -offset - height;
			break;
		case RS_TextData::VAMiddle:
			textOffset = offset + height / 2.0;
			if (!above)
				textOffset = -textOffset;
			break;
		case RS_TextData::VATop:
			if (above)
				textOffset = offset + height;
			else
				textOffset = -offset;
			break;
		}

	}
	else if (_entity->rtti() == RS2::EntityMText)
	{
		height = dynamic_cast<RS_MText *>(_entity)->getUsedTextHeight();
		switch (dynamic_cast<RS_MText *>(_entity)->getVAlign())
		{
		case RS_MTextData::VABottom:
			if (above)
				textOffset = offset;
			else
				textOffset = -offset - height;
			break;
		case RS_MTextData::VAMiddle:
			textOffset = offset + height / 2.0;
			if (!above)
				textOffset = -textOffset;
			break;
		case RS_MTextData::VATop:
			if (above)
				textOffset = offset + height;
			else
				textOffset = -offset;
			break;
		}
	}
	return textOffset;
}

void GetTextAttributes(RS_Entity *textEntity1, bool above, double &offset, RS_Vector &anchorPoint, double &textAngle, int &HAlign)
{
	if (textEntity1->rtti() == RS2::EntityText)
	{
		RS_Text *tent = dynamic_cast<RS_Text *>(textEntity1);
		offset = TextOffset(tent, offset, above);
		anchorPoint = tent->getInsertionPoint();
		textAngle = tent->getAngle();
		HAlign = tent->getHAlign();
		if (HAlign > 2)
			HAlign = 0;
	}
	else
	{
		RS_MText *tent = dynamic_cast<RS_MText *>(textEntity1);
		offset = TextOffset(tent, offset, above);
		anchorPoint = tent->getInsertionPoint();
		textAngle = tent->getAngle();
		HAlign = tent->getHAlign();
	}
}

RS_AlignedTextData::RS_AlignedTextData(RS_Entity *_textEntity,
			RS_Entity *_entity,
			RS_Vector _insertionPoint,
			double _offset,
			bool _above,
			RS2::UpdateMode _updateMode):
	textEntity(_textEntity)
	,entity(_entity)
	,insertionPoint(_insertionPoint)
	,offset(_offset)
	,above(_above)
	,updateMode(_updateMode)
{
}

std::ostream& operator << (std::ostream& os, const RS_AlignedTextData& td) {
	os << "("
	   <<td.textEntity<<','
	  <<td.entity<<','
	  <<td.insertionPoint<<','
	 <<td.offset<<','
	<<td.above<<','
	<<td.updateMode<<','
	<<")";
	return os;
}

/**
 * Constructor.
 */
RS_AlignedText::RS_AlignedText(RS_EntityContainer* parent,
                 const RS_AlignedTextData& d)
        : RS_EntityContainer(parent), data(d) {
	setOwner(false);		// don't autodelete entities
	addEntity(data.textEntity);
	update();
}

RS_Entity* RS_AlignedText::clone() const{
	RS_AlignedText* t = new RS_AlignedText(*this);

												// need to create new copy of text entity in "t"
	t->data.textEntity = this->data.textEntity->clone();
	if (t->entities.size() > 0)
		t->removeEntity(t->entities[0]);
												// and add it to the entity list
	t->addEntity(t->data.textEntity);
	t->data.textEntity->setParent(t);

	t->setOwner(isOwner());
	t->initId();
	t->update();
	return t;
}

RS_AlignedText::~RS_AlignedText()
{
	setOwner(true);
	clear();
}

/**
 * Updates the Inserts (letters) of this text. Called when the
 * text or it's data, position, alignment, .. changes.
 * This method also updates the usedTextWidth / usedTextHeight property.
 */
void RS_AlignedText::update()
{
    RS_DEBUG->print("RS_AlignedText::update");

	if (entities.size() > 0 && !isOwner())
		removeEntity(entities[0]);
    clear();
    if (isUndone()) {
        return;
    }

// beginning of added stuff
	RS_Entity
		*textEntity1 = data.textEntity,
		*shapeEntity = data.entity;

	textEntity1->update();
	RS_Vector offsetVector,
		anchorPoint,
		nearestPoint;
	double
		textAngle,
		totalAngle,
		offset(data.offset);
	nearestPoint = shapeEntity->getNearestPointOnEntity(data.insertionPoint);
	double pointangle,
		rotateAngle,
		shapeAngle;
	RS2::EntityType
		eType = shapeEntity->rtti();

	if (eType == RS2::EntityArc)
	{
		RS_Arc
			*arc = dynamic_cast<RS_Arc *>(shapeEntity);
		shapeAngle = arc->getTangentDirection(nearestPoint).angle() - M_PI;
		double
			pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
		if (pointDistance > arc->getRadius())
			data.above = true;
		else
			data.above = false;
	}
	else if (eType == RS2::EntityCircle)
	{
		RS_Circle
			*arc = dynamic_cast<RS_Circle *>(shapeEntity);
		shapeAngle = arc->getTangentDirection(nearestPoint).angle() - M_PI;
		double
			pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
		if (pointDistance > arc->getRadius())
			data.above = true;
		else
			data.above = false;
	}
	else if (eType == RS2::EntityEllipse)
	{
		RS_Ellipse
			*arc = dynamic_cast<RS_Ellipse *>(shapeEntity);
		shapeAngle = arc->getTangentDirection(nearestPoint).angle() - M_PI;
		double
			pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
		if (pointDistance > arc->getCenter().distanceTo(nearestPoint))
			data.above = true;
		else
			data.above = false;
	}
	else if (eType == RS2::EntityLine)
	{
		shapeAngle = RS_Math::correctAngle(shapeEntity->getStartpoint().angleTo(shapeEntity->getEndpoint()));
		pointangle = RS_Math::correctAngle(shapeEntity->getStartpoint().angleTo(data.insertionPoint) - shapeAngle);
		data.above = (pointangle >= 0.0 && pointangle <= M_PI);
	}
	// move text to start at insertionPoint
	int
		HAlign;
	if (textEntity1->rtti() == RS2::EntityAlignedText)
	{
		RS_AlignedText
			*tent = dynamic_cast<RS_AlignedText *>(textEntity1);
		GetTextAttributes(tent->getTextEntity(), data.above, offset, anchorPoint, textAngle, HAlign);
	}
	else
		GetTextAttributes(textEntity1, data.above, offset, anchorPoint, textAngle, HAlign);
	offsetVector.x = (nearestPoint.x - anchorPoint.x) + cos(textAngle + M_PI_2) * offset;
	offsetVector.y = (nearestPoint.y - anchorPoint.y) + sin(textAngle + M_PI_2) * offset;
	textEntity1->move(offsetVector);
	rotateAngle = shapeAngle - textAngle;
	textEntity1->rotate(nearestPoint, rotateAngle);
	if (eType == RS2::EntityCircle || eType == RS2::EntityArc || eType == RS2::EntityEllipse)
	{
		RS_Entity *inner_tent = ((RS_EntityContainer *)textEntity1)->firstEntity();
		while (inner_tent)
		{
			RS_Entity
				*letter;
			RS_Insert
				*iLetter;
			RS_Vector
				lastpt,
				lastrotpt,
				pt,
				center;
			bool
				first(true);
			double
				radius,
				angle,
				distance;
			int
				direction(1);		// Left edge orientation
			if (HAlign == 2)		// Right edge orientation
				direction = -1;
			if (eType == RS2::EntityEllipse)
			{
				double
					baseAngle,
					baseDist;
				RS_Ellipse
					*ellipse = dynamic_cast<RS_Ellipse *>(shapeEntity);
				center = ellipse->getCenter();
				if (direction == 1)
					letter = ((RS_EntityContainer *)inner_tent)->firstEntity();
				else if (direction == -1)
					letter = ((RS_EntityContainer *)inner_tent)->lastEntity();
				radius = center.distanceTo(nearestPoint);
				while (letter)
				{
					iLetter = (RS_Insert *)letter;
					if (first)
					{
						lastpt = iLetter->getInsertionPoint();
						lastrotpt = lastpt;
						first = false;
						baseAngle = ellipse->getEllipseAngle(nearestPoint);
						baseDist = ellipse->getEllipseLength(baseAngle);
					}
					else
					{
						pt = iLetter->getInsertionPoint();
						double
							distance = lastpt.distanceTo(pt),
							angle,
							angle0,
							radius (center.distanceTo(pt));
						RS_Vector
							pt2(lastrotpt);
						lastpt = pt;

						double
							theta = 2 * atan2(distance / 2, radius),
							angle1 = baseAngle + theta,
							angle2 = baseAngle - theta;
						RS_Vector
							ep1,
							ep2,
							endpt;
						ep1.x = center.x + cos(angle1) * (ellipse->getMajorRadius() + 1);
						ep1.y = center.y + sin(angle1) * (ellipse->getMajorRadius() + 1);
						ep2.x = center.x + cos(angle2) * (ellipse->getMajorRadius() + 1);
						ep2.y = center.y + sin(angle2) * (ellipse->getMajorRadius() + 1);
						RS_Line
							line1(center, ep1),
							line2(center, ep2);
						RS_Ellipse e1(nullptr, ellipse->getData()),
							e2(nullptr, ellipse->getData());
						RS_VectorSolutions rsvs1,
							rsvs2;
						rsvs1 = RS_Information::getIntersection(&e1, &line1, true);
						rsvs2 = RS_Information::getIntersection(&e2, &line2, true);
						if (rsvs1.size() > 0 && rsvs2.size() == 0)
							endpt = rsvs1[0];
						else if (rsvs2.size() > 0 && rsvs1.size() == 0)
							endpt = rsvs2[0];
						else if (rsvs1.size() > 0 && rsvs2.size() > 0)
						{
							if (center.distanceTo(rsvs1[0]) < center.distanceTo(rsvs2[0]))	// Try the shorter radius
								endpt = rsvs1[0];
							else
								endpt = rsvs2[0];
						}
						// this is where each of the letters gets rotated around the center of the arc
						double circ_rad = center.distanceTo(endpt);
						double letterarc = distance / circ_rad;
						baseAngle -= (theta * direction);
						line1.setStartpoint(center);
						endpt.x = center.x + cos(baseAngle) * (ellipse->getMajorRadius() + 1);
						endpt.y = center.y + sin(baseAngle) * (ellipse->getMajorRadius() + 1);
						line1.setEndpoint(endpt);
						rsvs1 = RS_Information::getIntersection(&e1, &line1, true);
						if (rsvs1.size() > 0)
							endpt = rsvs1[0];
						angle = ellipse->getEllipseAngle(endpt);
						angle0 = ellipse->getTangentDirection(endpt).angle() - M_PI;
						endpt.x = endpt.x + cos(baseAngle) * offset;
						endpt.y = endpt.y + sin(baseAngle) * offset;
						iLetter->setInsertionPoint(endpt);
						iLetter->rotate(iLetter->getInsertionPoint(), -angle0 * direction);		// was -angle
						lastrotpt = iLetter->getInsertionPoint();

					}
					if (direction == 1)
						letter = ((RS_EntityContainer *)inner_tent)->nextEntity();
					else
						letter = ((RS_EntityContainer *)inner_tent)->prevEntity();
				}

			}
			if (eType == RS2::EntityArc || eType == RS2::EntityCircle)
			{
				if (eType == RS2::EntityArc)
				{
					radius = ((RS_Arc *)shapeEntity)->getRadius();
					center = ((RS_Arc *)shapeEntity)->getCenter();
				}
				else
				{
					radius = ((RS_Circle *)shapeEntity)->getRadius();
					center = ((RS_Circle *)shapeEntity)->getCenter();
				}
				double
					textInsertionAngle(RS_Math::correctAngle(atan2(anchorPoint.y - center.y, anchorPoint.x - center.x)));
				totalAngle = 0.0;
				if (data.above)
					radius += fabs(offset);
				else
					radius -= fabs(offset);
				RS_Entity
					*saved_ent(0);
				int
					direction_multiplier(direction);
				if (HAlign == 1)
				{
					direction_multiplier = -1;
					if (textEntity1->rtti() == RS2::EntityAlignedText)
						lastpt = ((RS_AlignedText *)textEntity1)->getInsertionPoint();
					else if (textEntity1->rtti() == RS2::EntityMText)
						lastpt = ((RS_MText *)textEntity1)->getInsertionPoint();
					else
						lastpt = ((RS_Text *)textEntity1)->getInsertionPoint();

					double
						saved_angle(99999.0),
						tempAngle,
						delta;
					RS_Vector
						inPt;
					// compute angle to center of circle/arc
					textInsertionAngle = RS_Math::correctAngle(atan2(lastpt.y - center.y, lastpt.x - center.x));

					// Find letter closest to text insertion point
					letter = ((RS_EntityContainer *)inner_tent)->firstEntity();
					while (letter)
					{
						// compute angle to center of circle/arc
						inPt = ((RS_Insert *)letter)->getInsertionPoint();
						tempAngle = RS_Math::correctAngle(atan2(inPt.y - center.y, inPt.x - center.x));
						delta = RS_Math::getAngleDifference(tempAngle, textInsertionAngle);
						if (delta < saved_angle)
						{
							saved_angle = delta;
							saved_ent = letter;
						}
						letter = ((RS_EntityContainer *)inner_tent)->nextEntity();
					}
					lastpt = ((RS_Insert *)saved_ent)->getInsertionPoint();
					lastrotpt = lastpt;
					first = false;
				}
				if (direction == 1)
					letter = ((RS_EntityContainer *)inner_tent)->firstEntity();
				else
					letter = ((RS_EntityContainer *)inner_tent)->lastEntity();
				while (letter)
				{
					if (HAlign == 1 && letter == saved_ent)
						direction_multiplier = 1;
					else
					{
						iLetter = (RS_Insert *)letter;
						if (first)
						{
							lastpt = iLetter->getInsertionPoint();
							lastrotpt = lastpt;
							first = false;
						}
						// this is where each of the letters gets rotated around the center of the arc
						RS_Vector
							tempPt,
							cPt;
						double
							distance2center,
							letterAngle;
						iLetter->rotate(iLetter->getInsertionPoint(), -rotateAngle);
						iLetter->calculateBorders();
						cPt.x = (iLetter->getMax().x + iLetter->getMin().x) / 2.0;
						cPt.y = iLetter->getInsertionPoint().y;
						iLetter->rotate(iLetter->getInsertionPoint(), rotateAngle);
						cPt.rotate(iLetter->getInsertionPoint(), rotateAngle);

						pt = iLetter->getInsertionPoint();
						tempPt = lastrotpt;
						distance2center = lastpt.distanceTo(cPt);
						distance = lastpt.distanceTo(pt);
						angle = distance / radius;
						letterAngle = distance2center / radius;
						
						iLetter->setInsertionPoint(tempPt.rotate(center, -angle * direction_multiplier));
						iLetter->rotate(iLetter->getInsertionPoint(), -letterAngle * direction_multiplier);
					}
					if (direction == 1)
						letter = ((RS_EntityContainer *)inner_tent)->nextEntity();
					else
						letter = ((RS_EntityContainer *)inner_tent)->prevEntity();
				}
			}
			inner_tent = ((RS_EntityContainer *)textEntity1)->nextEntity();
		}
	}

	addEntity(data.textEntity);
    forcedCalculateBorders();

    RS_DEBUG->print("RS_AlignedText::update: OK");
}



RS_Vector RS_AlignedText::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
	if (dist) {
		*dist = data.insertionPoint.distanceTo(coord);
	}
	return data.insertionPoint;
}


RS_VectorSolutions RS_AlignedText::getRefPoints() const{
	RS_AlignedText
		*local = (RS_AlignedText *)this;
	return (local->getTextEntity()->getRefPoints());
}

void RS_AlignedText::move(const RS_Vector& offset) {
    RS_EntityContainer::move(offset);
	data.insertionPoint.move(offset);
//	getTextEntity()->move(offset);
//    update();
}



void RS_AlignedText::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
	data.insertionPoint.rotate(center, angleVector);
//	getTextEntity()->rotate(center, angle);
//    update();
}
void RS_AlignedText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
	data.insertionPoint.rotate(center, angleVector);
//	getTextEntity()->rotate(center, angleVector);
//    update();
}



void RS_AlignedText::scale(const RS_Vector& center, const RS_Vector& factor) {
	data.insertionPoint.scale(center, factor);
	getTextEntity()->scale(center, factor);
    update();
}



void RS_AlignedText::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
	data.insertionPoint.mirror(axisPoint1, axisPoint2);
	getTextEntity()->mirror(axisPoint1, axisPoint2);
    update();
}



bool RS_AlignedText::hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) {
    return false;
}



/**
 * Implementations must stretch the given range of the entity
 * by the given offset.
 */
void RS_AlignedText::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
#if 0
    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
#endif
}


/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_AlignedText& p) {
    os << " Text: " << p.getData() << "\n";
    return os;
}

void RS_AlignedText::draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset)
{
	getTextEntity()->draw(painter, view, patternOffset);
	getGeometryEntity()->draw(painter, view, patternOffset);

    if (!(painter && view)) {
        return;
    }

    //if (!view->isPrintPreview() && !view->isPrinting())
    //{
    //    if (view->isPanning() || view->toGuiDY(getHeight()) < 4)
    //    {
    //        painter->drawRect(view->toGui(getMin()), view->toGui(getMax()));
    //        return;
    //    }
    //}

    foreach (auto e, entities)
    {
        view->drawEntity(painter, e);
    }
}
