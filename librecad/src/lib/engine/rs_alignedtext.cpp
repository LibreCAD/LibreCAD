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
#include "rs_graphic.h"
#include "rs_modification.h"

#include "qt_windows.h"

double min(double _val1, double _val2)
{
	if (_val2 < _val1)
		return (_val2);
	else
		return (_val1);
}

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
			RS2::UpdateMode _updateMode,
			int _directionMultiplier):
	textEntity(_textEntity)
	,entity(_entity)
	,insertionPoint(_insertionPoint)
	,offset(_offset)
	,above(_above)
	,updateMode(_updateMode)
	,directionMultiplier(_directionMultiplier)
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
	// keep entity's layer, just to have a valid layer for the object
	setLayer(data.entity->getLayer());
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
			if (arc->isReversed())
			{
				shapeAngle += M_PI;
				data.directionMultiplier *= -1;
				data.above = !data.above;
			}
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

double RS_AlignedText::computeEllipseAngle(RS_Vector &_insertPoint, RS_Vector &_lastPoint, RS_Ellipse *_ellipse, double _baseAngle, int _direction_multiplier)
{
	double
		distance = _lastPoint.distanceTo(_insertPoint),
		minorRadius = _ellipse->getMinorRadius(),
		ellipseDistance,
		ellipseAngle,
		minAngle,
		maxAngle,
		eTol(0.005);

	double circum = _ellipse->getLength();
	distance = fmod(distance, circum);
	ellipseAngle = distance / minorRadius;
	if (ellipseAngle > 2 * M_PI)
		ellipseAngle = (2 * M_PI) - 0.0001;
	if (distance < 0.00001)
		ellipseDistance = 0.0;
	else if (_direction_multiplier < 0)
		ellipseDistance = _ellipse->getEllipseLength(_baseAngle, _baseAngle - ellipseAngle * _direction_multiplier);
	else
		ellipseDistance = _ellipse->getEllipseLength(_baseAngle - ellipseAngle * _direction_multiplier, _baseAngle);
	minAngle = 0.0;
	maxAngle = min(ellipseAngle * 2.0, 2 * M_PI);
	while (fabs(ellipseDistance - distance) > eTol)
	{
		if (ellipseDistance > distance)
		{
			maxAngle = ellipseAngle;
			ellipseAngle = (minAngle + ellipseAngle) / 2.0;
		}
		else
		{
			minAngle = ellipseAngle;
			ellipseAngle = (maxAngle + ellipseAngle) / 2.0;
		}
		if (_direction_multiplier < 0)
			ellipseDistance = _ellipse->getEllipseLength(_baseAngle,  _baseAngle - ellipseAngle * _direction_multiplier);
		else
			ellipseDistance = _ellipse->getEllipseLength(_baseAngle - ellipseAngle * _direction_multiplier, _baseAngle);
	}
	RS_Vector
		endPt = _ellipse->getCenter();
	ellipseAngle = _ellipse->getCenter().angleTo(_ellipse->getEllipsePoint(_baseAngle - ellipseAngle * _direction_multiplier));
	ellipseAngle = (_baseAngle - ellipseAngle) * _direction_multiplier;

	return (ellipseAngle);
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

	data.directionMultiplier = 1;
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
	if (eType == RS2::EntityEllipse)
	{
		RS_Ellipse
			*nEllipse = dynamic_cast<RS_Ellipse *>(shapeEntity->clone());
		RS_Vector
			nEnd(data.insertionPoint);
		RS_Line
			nLine(nEllipse->getCenter(), nEnd);
		nEnd.x = nEnd.x + cos(nLine.getAngle1()) * nEllipse->getMajorRadius();
		nEnd.y = nEnd.y + sin(nLine.getAngle1()) * nEllipse->getMajorRadius();
		nLine.setEndpoint(nEnd);
		RS_VectorSolutions
			nVs = RS_Information::getIntersection(nEllipse, &nLine, true);
		nearestPoint = nVs[0];
		delete (nEllipse);
	}

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
			direction_multiplier = direction * data.directionMultiplier;
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
					direction_multiplier *= -1;
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
				radius = center.distanceTo(nearestPoint);
				baseAngle = ellipse->getEllipseAngle(nearestPoint);
				baseDist = ellipse->getEllipseLength(baseAngle);

				while (letter)
				{
					iLetter = (RS_Insert *)letter;
					if (HAlign == 1 && letter == saved_ent)
						direction_multiplier *= -1;
					if (first)
					{
						lastpt = iLetter->getInsertionPoint();
						lastrotpt = lastpt;
						first = false;
					}
					pt = iLetter->getInsertionPoint();
					double
						ellipseAngle(computeEllipseAngle(pt, lastpt, ellipse, baseAngle, direction_multiplier));
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

					pt = iLetter->getInsertionPoint();
					tempPt = lastrotpt;
					distance = lastpt.distanceTo(pt);
					angle = distance / radius;

					iLetter->setInsertionPoint(tempPt.rotate(center, -ellipseAngle * direction_multiplier));

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

					// Rotate center point to position it relative to the letter's insertion point
					tempPt = cPt - pt + lastrotpt;

					cPt = tempPt.rotate(center, -ellipseAngle * direction_multiplier);
					line1.setStartpoint(center);
					line1.setEndpoint(cPt);
					endpt.x = line1.getEndpoint().x + cos(line1.getAngle1()) * ellipse->getMajorRadius();
					endpt.y = line1.getEndpoint().y + sin(line1.getAngle1()) * ellipse->getMajorRadius();
					endpt.valid = true;
					line1.setEndpoint(endpt);
					rsvs1 = RS_Information::getIntersection(&e1, &line1, true);
					cPt = rsvs1[0];
					tempAngle = e1.getTangentDirection(cPt).angle();

					// end of center point rotation
					iLetter->rotate(iLetter->getInsertionPoint(), -iLetter->getAngle());
					iLetter->rotate(iLetter->getInsertionPoint(), (tempAngle - M_PI));
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
						direction_multiplier *= -1;
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
	data.insertionPoint.move(offset);
	getTextEntity()->move(offset);
    update();
}



void RS_AlignedText::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
	data.insertionPoint.rotate(center, angleVector);
	getTextEntity()->rotate(center, angle);
    update();
}
void RS_AlignedText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
	data.insertionPoint.rotate(center, angleVector);
	getTextEntity()->rotate(center, angleVector);
    update();
}



void RS_AlignedText::scale(const RS_Vector& center, const RS_Vector& factor) {
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
	getGeometryEntity()->draw(painter, view, patternOffset);
	getTextEntity()->draw(painter, view, patternOffset);

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

#if 0
// This has gnerally the right idea, but not sure how to get 'undo' to work properly for it
RS_Block *RS_AlignedText::createBlock(bool _createBlock)
{
	RS_Entity
		*textContainer(0),
		*letter,
		*textEntity = data.textEntity;
	RS2::EntityType
		tType = textEntity->rtti();
	RS_Insert
		*iLetter;
	RS_Block
		*b(0);
	RS_Graphic
		*graphic = parent->getGraphic();
	RS_Insert
		*i(0);

	if (tType == RS2::EntityMText)
		textContainer = ((RS_EntityContainer *)textEntity)->firstEntity();
	else
		textContainer = textEntity;
	std::vector<RS_Entity *> addList;

	RS_Modification
		modify(*((RS_EntityContainer *)textContainer), NULL, false);
	if (textContainer)
	{
		if (modify.getExplodedText(textEntity, addList))
		{
			letter = ((RS_EntityContainer *)textContainer)->firstEntity();
			iLetter = (RS_Insert *)letter;

			int
				count(1);
			char
				baseName[50];
			strcpy(baseName, "textBlock");
			QString
				blockName;
			blockName = baseName;
			while (graphic->findBlock(blockName) != 0)
				blockName = baseName + QString("%1").arg(count++);
			RS_BlockData bd = RS_BlockData(blockName, iLetter->getInsertionPoint(), false);
			b = new RS_Block(graphic, bd);
			b->visibleInBlockList(false);
			b->reparent(graphic);
			b->setLayer(textContainer->getLayer());
			b->setPen(textContainer->getPen());
			graphic->addBlock(b);
			
			if (_createBlock)
			{
				// create insert object for the paste block
				RS_Vector vfactor(1.0, 1.0);
				double angle(0.0);
				RS_InsertData di = RS_InsertData(b->getName(), iLetter->getInsertionPoint(), vfactor, angle, 1, 1, RS_Vector(0.0, 0.0));
				i = new RS_Insert(graphic, di);
				i->setLayer(textContainer->getLayer());
				i->setPen(textContainer->getPen());
				i->reparent(graphic);
				graphic->addEntity(i);
				// end of insert object for paste block
			}

			for (RS_Entity* e : addList)
			{
				if (e)
				{
					b->addEntity(e);
					e->setLayer(textContainer->getLayer());
					e->setPen(textContainer->getPen(false));
					e->setSelected(false);
					e->reparent(b);
				}
			}
			if (i)
			{
				i->update();
				i->setSelected(false);
			}
		}
	}
	return (b);
}
#endif

bool RS_AlignedText::getUnlinkedText(RS_EntityContainer *container, std::vector<RS_Entity *>&addList)
{
	bool
		result(false);

	RS_Modification
		modify(*container, NULL, false);

	result = modify.getUnlinkedText(this, addList, false);

	return (result);
}
