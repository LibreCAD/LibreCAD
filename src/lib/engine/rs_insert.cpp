/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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


#include "rs_insert.h"

#include "rs_block.h"
#include "rs_graphic.h"


/**
 * @param parent The graphic this block belongs to.
 */
RS_Insert::RS_Insert(RS_EntityContainer* parent,
                     const RS_InsertData& d)
        : RS_EntityContainer(parent), data(d) {

	block = NULL;

    if (data.updateMode!=RS2::NoUpdate) {
        update();
        //calculateBorders();
    }
}


/**
 * Destructor.
 */
RS_Insert::~RS_Insert() {}


/**
 * Updates the entity buffer of this insert entity. This method
 * needs to be called whenever the block this insert is based on changes.
 */
void RS_Insert::update() {

	RS_DEBUG->print("RS_Insert::update");
	RS_DEBUG->print("RS_Insert::update: name: %s", data.name.latin1());
	RS_DEBUG->print("RS_Insert::update: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);

	if (updateEnabled==false) {
		return;
	}

    clear();

    RS_Block* blk = getBlockForInsert();
    if (blk==NULL) {
        //return NULL;
		RS_DEBUG->print("RS_Insert::update: Block is NULL");
        return;
    }

    if (isUndone()) {
		RS_DEBUG->print("RS_Insert::update: Insert is in undo list");
        return;
    }

	if (fabs(data.scaleFactor.x)<1.0e-6 || fabs(data.scaleFactor.y)<1.0e-6) {
		RS_DEBUG->print("RS_Insert::update: scale factor is 0");
		return;
	}
	
    RS_Pen tmpPen;

	/*RS_PtrListIterator<RS_Entity> it = createIterator();
    RS_Entity* e;
    while ( (e = it.current()) != NULL ) {
        ++it;*/

	RS_DEBUG->print("RS_Insert::update: cols: %d, rows: %d", 
		data.cols, data.rows);
	RS_DEBUG->print("RS_Insert::update: block has %d entities", 
		blk->count());
	
    for (RS_Entity* e=blk->firstEntity(); e!=NULL; e=blk->nextEntity()) {
        for (int c=0; c<data.cols; ++c) {
            RS_DEBUG->print("RS_Insert::update: col %d", c);
            for (int r=0; r<data.rows; ++r) {
                RS_DEBUG->print("RS_Insert::update: row %d", r);

                if (e->rtti()==RS2::EntityInsert && 
                    data.updateMode!=RS2::PreviewUpdate) {

					RS_DEBUG->print("RS_Insert::update: updating sub-insert");
                    ((RS_Insert*)e)->update();
                }
				
				RS_DEBUG->print("RS_Insert::update: cloning entity");

                RS_Entity* ne = e->clone();
                ne->initId();
				ne->setUpdateEnabled(false);
                ne->setParent(this);
                ne->setVisible(getFlag(RS2::FlagVisible));
				
				RS_DEBUG->print("RS_Insert::update: transforming entity");

                // Move:
				RS_DEBUG->print("RS_Insert::update: move 1");
				if (fabs(data.scaleFactor.x)>1.0e-6 && 
				    fabs(data.scaleFactor.y)>1.0e-6) {
	                ne->move(data.insertionPoint +
    	                     RS_Vector(data.spacing.x/data.scaleFactor.x*c,
        	                           data.spacing.y/data.scaleFactor.y*r));
				}
				else {
	                ne->move(data.insertionPoint);
				}
                // Move because of block base point:
				RS_DEBUG->print("RS_Insert::update: move 2");
                ne->move(blk->getBasePoint()*-1);
                // Scale:
				RS_DEBUG->print("RS_Insert::update: scale");
                ne->scale(data.insertionPoint, data.scaleFactor);
                // Rotate:
				RS_DEBUG->print("RS_Insert::update: rotate");
                ne->rotate(data.insertionPoint, data.angle);
                // Select:
                ne->setSelected(isSelected());

                // individual entities can be on indiv. layers
                tmpPen = ne->getPen(false);

                // color from block (free floating):
                if (tmpPen.getColor()==RS_Color(RS2::FlagByBlock)) {
                    tmpPen.setColor(getPen().getColor());
                }

                // line width from block (free floating):
                if (tmpPen.getWidth()==RS2::WidthByBlock) {
                    tmpPen.setWidth(getPen().getWidth());
                }

                // line type from block (free floating):
                if (tmpPen.getLineType()==RS2::LineByBlock) {
                    tmpPen.setLineType(getPen().getLineType());
                }

                // now that we've evaluated all flags, let's strip them:
                // TODO: strip all flags (width, line type)
                //tmpPen.setColor(tmpPen.getColor().stripFlags());

                ne->setPen(tmpPen);
				
				ne->setUpdateEnabled(true);
				
				if (data.updateMode!=RS2::PreviewUpdate) {
					RS_DEBUG->print("RS_Insert::update: updating new entity");
					ne->update();
				}

				RS_DEBUG->print("RS_Insert::update: adding new entity");
                addEntity(ne);
            }
        }
    }
    calculateBorders();

	RS_DEBUG->print("RS_Insert::update: OK");
}



/**
 * @return Pointer to the block associated with this Insert or
 *   NULL if the block couldn't be found. Blocks are requested
 *   from the blockSource if one was supplied and otherwise from 
 *   the closest parent graphic.
 */
RS_Block* RS_Insert::getBlockForInsert() {
	if (block!=NULL) {
		return block;
	}

    RS_BlockList* blkList;

    if (data.blockSource==NULL) {
        if (getGraphic()!=NULL) {
            blkList = getGraphic()->getBlockList();
        } else {
            blkList = NULL;
        }
    } else {
        blkList = data.blockSource;
    }

    RS_Block* blk = NULL;
    if (blkList!=NULL) {
        blk = blkList->find(data.name);
    }

    if (blk!=NULL) {
    }

	block = blk;

    return blk;
}


/**
 * Is this insert visible? (re-implementation from RS_Entity)
 *
 * @return true Only if the entity and the block and the layer it is on 
 * are visible.
 * The Layer might also be NULL. In that case the layer visiblity 
 * is ignored.
 * The Block might also be NULL. In that case the block visiblity 
 * is ignored.
 */
bool RS_Insert::isVisible() {
    RS_Block* blk = getBlockForInsert();
    if (blk!=NULL) {
        if (blk->isFrozen()) {
            return false;
        }
    }

    return RS_Entity::isVisible();
}


RS_VectorSolutions RS_Insert::getRefPoints() {
	RS_VectorSolutions ret(data.insertionPoint);
	return ret;
}
    


RS_Vector RS_Insert::getNearestRef(const RS_Vector& coord,
                                     double* dist) {

	return getRefPoints().getClosest(coord, dist);
}



void RS_Insert::move(RS_Vector offset) {
	RS_DEBUG->print("RS_Insert::move: offset: %f/%f", 
		offset.x, offset.y);
	RS_DEBUG->print("RS_Insert::move1: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);
    data.insertionPoint.move(offset);
	RS_DEBUG->print("RS_Insert::move2: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::rotate(RS_Vector center, double angle) {
	RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f "
	    "/ center: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y,
		center.x, center.y);
    data.insertionPoint.rotate(center, angle);
    data.angle = RS_Math::correctAngle(data.angle+angle);
	RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::scale(RS_Vector center, RS_Vector factor) {
	RS_DEBUG->print("RS_Insert::scale1: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);
    data.insertionPoint.scale(center, factor);
    data.scaleFactor.scale(RS_Vector(0.0, 0.0), factor);
    data.spacing.scale(RS_Vector(0.0, 0.0), factor);
	RS_DEBUG->print("RS_Insert::scale2: insertionPoint: %f/%f", 
		data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
	
	RS_Vector vec;
	vec.setPolar(1.0, data.angle);
	vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
	data.angle = vec.angle();

	data.scaleFactor.y*=-1;

    update();
}


std::ostream& operator << (std::ostream& os, const RS_Insert& i) {
    os << " Insert: " << i.getData() << std::endl;
    return os;
}

