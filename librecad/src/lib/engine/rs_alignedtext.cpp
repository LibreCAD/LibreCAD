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
	// keep text entity's layer, just to have a valid layer for the object
	setLayer(data.textEntity->getLayer());
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

double RS_AlignedText::setArcParams(RS_Vector &_nearestPoint)
{
	RS_Entity
		*shapeEntity = this->data.entity;
	RS2::EntityType
		eType(shapeEntity->rtti());
	double
		shapeAngle(0.0);
	switch (eType)
	{
		case RS2::EntityArc:
		{
			RS_Arc
				*arc = dynamic_cast<RS_Arc *>(shapeEntity);
			shapeAngle = arc->getTangentDirection(_nearestPoint).angle() - M_PI;
			double
				pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
			if (pointDistance > arc->getRadius())
				data.above = true;
			else
				data.above = false;
		}
		break;
		case RS2::EntityCircle:
		{
			RS_Circle
				*arc = dynamic_cast<RS_Circle *>(shapeEntity);
			shapeAngle = arc->getTangentDirection(_nearestPoint).angle() - M_PI;
			double
				pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
			if (pointDistance > arc->getRadius())
				data.above = true;
			else
				data.above = false;
		}
		break;
		case RS2::EntityEllipse:
		{
			RS_Ellipse
				*arc = dynamic_cast<RS_Ellipse *>(shapeEntity);
			shapeAngle = arc->getTangentDirection(_nearestPoint).angle() - M_PI;
			double
				pointDistance(arc->getCenter().distanceTo(data.insertionPoint));
			if (pointDistance > arc->getCenter().distanceTo(_nearestPoint))
				data.above = true;
			else
				data.above = false;
		}
		break;
	}
	return (shapeAngle);
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

	RS_Entity
		*textEntity1 = data.textEntity,
		*shapeEntity = data.entity;
	int
		HAlign;
	RS_Vector
		offsetVector,
		anchorPoint,
		nearestPoint;
	double
		textAngle,
		totalAngle,
		pointangle,
		rotateAngle,
		shapeAngle,
		offset(data.offset);
	RS2::EntityType
		eType(shapeEntity->rtti()),
		tType(textEntity1->rtti());

	textEntity1->update();
	nearestPoint = shapeEntity->getNearestPointOnEntity(data.insertionPoint);

	if (eType == RS2::EntityArc || eType == RS2::EntityCircle || eType == RS2::EntityEllipse)
		shapeAngle = setArcParams(nearestPoint);
	else if (eType == RS2::EntityLine)
	{
		shapeAngle = RS_Math::correctAngle(shapeEntity->getStartpoint().angleTo(shapeEntity->getEndpoint()));
		pointangle = RS_Math::correctAngle(shapeEntity->getStartpoint().angleTo(data.insertionPoint) - shapeAngle);
		data.above = (pointangle >= 0.0 && pointangle <= M_PI);
	}
	// move text to start at insertionPoint
	if (tType == RS2::EntityAlignedText)
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
		RS_Entity *inner_tent;
		// MText has an extra level of containers above the letters
		if (tType == RS2::EntityMText)
			inner_tent = ((RS_EntityContainer *)textEntity1)->firstEntity();
		else
			// Text contains the letters directly
			inner_tent = textEntity1;
		while (inner_tent)
		{
			RS_Entity
				*saved_ent(0),
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
				direction_multiplier,
				direction(1);		// Left edge orientation
			if (HAlign == 2)		// Right edge orientation
				direction = -1;
			direction_multiplier = direction;
			double
				saved_angle(99999.0),
				tempAngle,
				delta;
			RS_Vector
				inPt;
			RS_Ellipse
				*ellipse;
			double
				baseAngle,
				baseDist,
				textInsertionAngle;
			RS_Vector
				cPt;
			if (eType == RS2::EntityEllipse)
			{
				ellipse = dynamic_cast<RS_Ellipse *>(shapeEntity->clone());
				RS_Vector
					tempPt;
				center = ellipse->getCenter();
				if (data.above)
					radius += fabs(offset);
				else
					radius -= fabs(offset);
			}
			else if (eType == RS2::EntityCircle || eType == RS2::EntityArc)
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
				textInsertionAngle = RS_Math::correctAngle(atan2(anchorPoint.y - center.y, anchorPoint.x - center.x));
				totalAngle = 0.0;
				if (data.above)
					radius += fabs(offset);
				else
					radius -= fabs(offset);
			}
			if (eType == RS2::EntityEllipse || eType == RS2::EntityArc || eType == RS2::EntityCircle)
			{
				if (HAlign == 1)
				{
					direction_multiplier = -1;
					if (tType == RS2::EntityAlignedText)
						lastpt = ((RS_AlignedText *)textEntity1)->getInsertionPoint();
					else if (tType == RS2::EntityMText)
						lastpt = ((RS_MText *)textEntity1)->getInsertionPoint();
					else
						lastpt = ((RS_Text *)textEntity1)->getInsertionPoint();

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
			}
			if (eType == RS2::EntityEllipse)
			{
				if (direction == 1)
					letter = ((RS_EntityContainer *)inner_tent)->firstEntity();
				else if (direction == -1)
					letter = ((RS_EntityContainer *)inner_tent)->lastEntity();
				ellipse->moveStartpoint(nearestPoint);
				ellipse->moveEndpoint(nearestPoint);
				radius = center.distanceTo(nearestPoint);
				baseAngle = ellipse->getEllipseAngle(nearestPoint);
				baseDist = ellipse->getEllipseLength(baseAngle);
				while (letter)
				{
					iLetter = (RS_Insert *)letter;
					if (HAlign == 1 && letter == saved_ent)
						direction_multiplier = 1;
					if (first)
					{
						lastpt = iLetter->getInsertionPoint();
						lastrotpt = lastpt;
						first = false;
					}
					pt = iLetter->getInsertionPoint();
					double
						distance = lastpt.distanceTo(pt),
						minorRadius = ellipse->getMinorRadius(),
						ellipseDistance,
						tempAngle,
						minAngle,
						maxAngle,
						eTol(0.005);
					
					tempAngle = distance / minorRadius;
					ellipseDistance = ellipse->getEllipseLength(baseAngle - tempAngle * direction_multiplier);
					double circum = ellipse->getLength();
					if (ellipseDistance > circum / 2.0)
						ellipseDistance = circum - ellipseDistance;
					minAngle = 0.0;
					maxAngle = tempAngle * 2.0;
					while (fabs(ellipseDistance - distance) > eTol)
					{
						if (ellipseDistance > distance)
						{
							maxAngle = tempAngle;
							tempAngle = (minAngle + tempAngle) / 2.0;
						}
						else
						{
							minAngle = tempAngle;
							tempAngle = (maxAngle + tempAngle) / 2.0;
						}
						ellipseDistance = ellipse->getEllipseLength(baseAngle - tempAngle * direction_multiplier);
						if (ellipseDistance > circum / 2.0)
							ellipseDistance = circum - ellipseDistance;
					}
					RS_Vector
						tempPt,
						endpt;
					RS_Line
						line1;
					RS_Ellipse e1(nullptr, ellipse->getData());
					RS_VectorSolutions rsvs1;

					iLetter->rotate(iLetter->getInsertionPoint(), -rotateAngle);
					iLetter->calculateBorders();
					cPt.x = (iLetter->getMax().x + iLetter->getMin().x) / 2.0;
					cPt.y = iLetter->getInsertionPoint().y;
					iLetter->rotate(iLetter->getInsertionPoint(), rotateAngle);
					cPt.rotate(iLetter->getInsertionPoint(), rotateAngle);

					double
						distance2center,
						letterAngle;
					pt = iLetter->getInsertionPoint();
					tempPt = lastrotpt;
					distance2center = lastpt.distanceTo(cPt);
					distance = lastpt.distanceTo(pt);
					angle = distance / radius;
					letterAngle = distance2center / radius;

					iLetter->setInsertionPoint(tempPt.rotate(center, -tempAngle * direction_multiplier));
					line1.setStartpoint(center);
					line1.setEndpoint(iLetter->getInsertionPoint());
					endpt.x = line1.getEndpoint().x + cos(line1.getAngle1()) * ellipse->getMajorRadius();
					endpt.y = line1.getEndpoint().y + sin(line1.getAngle1()) * ellipse->getMajorRadius();
					endpt.valid = true;
					line1.setEndpoint(endpt);
					rsvs1 = RS_Information::getIntersection(&e1, &line1, true);
					endpt = rsvs1[0];
					double
						radius2 = endpt.distanceTo(center),
						diff(radius2 - radius);

					endpt = iLetter->getInsertionPoint();
					endpt.x = endpt.x + cos(line1.getAngle1()) * diff;
					endpt.y = endpt.y + sin(line1.getAngle1()) * diff;
					iLetter->setInsertionPoint(endpt);

					iLetter->rotate(iLetter->getInsertionPoint(), -letterAngle * direction_multiplier);

					if (direction == 1)
						letter = ((RS_EntityContainer *)inner_tent)->nextEntity();
					else
						letter = ((RS_EntityContainer *)inner_tent)->prevEntity();
				}
				delete (ellipse);
			}
			if (eType == RS2::EntityArc || eType == RS2::EntityCircle)
			{
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
			if (tType == RS2::EntityMText)
				inner_tent = ((RS_EntityContainer *)textEntity1)->nextEntity();
			else
				inner_tent = 0;
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
void RS_AlignedText::stretch(const RS_Vector& /* firstCorner */, const RS_Vector& /* secondCorner */, const RS_Vector& /* offset */) {
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
