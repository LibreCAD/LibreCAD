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

#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

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
}

RS_Entity* RS_AlignedText::clone() const{
	RS_AlignedText* t = new RS_AlignedText(*this);
	t->setOwner(isOwner());
	t->initId();
	t->detach();
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

	getTextEntity()->update();

#if 0
// replace this with most of the code from "RS_Modification::ShapeText"?
#endif
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
	getTextEntity()->move(offset);
//    update();
}



void RS_AlignedText::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_EntityContainer::rotate(center, angleVector);
	data.insertionPoint.rotate(center, angleVector);
	getTextEntity()->rotate(center, angle);
//    update();
}
void RS_AlignedText::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_EntityContainer::rotate(center, angleVector);
	data.insertionPoint.rotate(center, angleVector);
	getTextEntity()->rotate(center, angleVector);
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
