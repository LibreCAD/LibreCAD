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
#include<cmath>
#include "rs_insert.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_debug.h"

RS_InsertData::RS_InsertData(const QString& _name,
							 RS_Vector _insertionPoint,
							 RS_Vector _scaleFactor,
							 double _angle,
							 int _cols, int _rows, RS_Vector _spacing,
							 RS_BlockList* _blockSource ,
							 RS2::UpdateMode _updateMode ):
	name(_name)
  ,insertionPoint(_insertionPoint)
  ,scaleFactor(_scaleFactor)
  ,angle(_angle)
  ,cols(_cols)
  ,rows(_rows)
  ,spacing(_spacing)
  ,blockSource(_blockSource)
  ,updateMode(_updateMode)
{
}

std::ostream& operator << (std::ostream& os,
									 const RS_InsertData& d) {
	   os << "(" << d.name.toLatin1().data() << ")";
	   return os;
   }
/**
 * @param parent The graphic this block belongs to.
 */
RS_Insert::RS_Insert(RS_EntityContainer* parent,
                     const RS_InsertData& d)
        : RS_EntityContainer(parent), data(d) {

		block = nullptr;

    if (data.updateMode!=RS2::NoUpdate) {
        update();
        //calculateBorders();
    }
}


RS_Entity* RS_Insert::clone() const{
	RS_Insert* i = new RS_Insert(*this);
	i->setOwner(isOwner());
	i->initId();
	i->detach();
	return i;
}


/**
 * Updates the entity buffer of this insert entity. This method
 * needs to be called whenever the block this insert is based on changes.
 */
void RS_Insert::update() {

        RS_DEBUG->print("RS_Insert::update");
        RS_DEBUG->print("RS_Insert::update: name: %s", data.name.toLatin1().data());
//        RS_DEBUG->print("RS_Insert::update: insertionPoint: %f/%f",
//                data.insertionPoint.x, data.insertionPoint.y);

        if (updateEnabled==false) {
                return;
        }

    clear();

    RS_Block* blk = getBlockForInsert();
	if (!blk) {
		//return nullptr;
				RS_DEBUG->print("RS_Insert::update: Block is nullptr");
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

        /*QListIterator<RS_Entity> it = createIterator();
    RS_Entity* e;
	while ( (e = it.current())  ) {
        ++it;*/

        RS_DEBUG->print("RS_Insert::update: cols: %d, rows: %d",
                data.cols, data.rows);
        RS_DEBUG->print("RS_Insert::update: block has %d entities",
                blk->count());
//int i_en_counts=0;
		for(auto e: *blk){
        for (int c=0; c<data.cols; ++c) {
//            RS_DEBUG->print("RS_Insert::update: col %d", c);
            for (int r=0; r<data.rows; ++r) {
//                i_en_counts++;
//                RS_DEBUG->print("RS_Insert::update: row %d", r);

                if (e->rtti()==RS2::EntityInsert &&
                    data.updateMode!=RS2::PreviewUpdate) {

//                                        RS_DEBUG->print("RS_Insert::update: updating sub-insert");
					static_cast<RS_Insert*>(e)->update();
                }

//                                RS_DEBUG->print("RS_Insert::update: cloning entity");

                RS_Entity* ne;
                if ( (data.scaleFactor.x - data.scaleFactor.y)>1.0e-6) {
                    if (e->rtti()== RS2::EntityArc) {
						RS_Arc* a= static_cast<RS_Arc*>(e);
						ne = new RS_Ellipse{this,
						{a->getCenter(), {a->getRadius(), 0.},
								1, a->getAngle1(), a->getAngle2(),
								a->isReversed()}
					};
                        ne->setLayer(e->getLayer());
                        ne->setPen(e->getPen(false));
                    } else if (e->rtti()== RS2::EntityCircle) {
						RS_Circle* a= static_cast<RS_Circle*>(e);
						ne = new RS_Ellipse{this,
						{ a->getCenter(), {a->getRadius(), 0.}, 1, 0., 2.*M_PI, false}
					};
                        ne->setLayer(e->getLayer());
                        ne->setPen(e->getPen(false));
                    } else
                        ne = e->clone();
                } else
                    ne = e->clone();
                ne->initId();
                ne->setUpdateEnabled(false);
                // if entity layer are 0 set to insert layer to allow "1 layer control" bug ID #3602152
                RS_Layer *l= ne->getLayer();//special fontchar block don't have
				if (l  && ne->getLayer()->getName() == "0")
                    ne->setLayer(this->getLayer());
                ne->setParent(this);
                ne->setVisible(getFlag(RS2::FlagVisible));

//                                RS_DEBUG->print("RS_Insert::update: transforming entity");

                // Move:
//                                RS_DEBUG->print("RS_Insert::update: move 1");
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
//                                RS_DEBUG->print("RS_Insert::update: move 2");
                ne->move(blk->getBasePoint()*-1);
                // Scale:
//                                RS_DEBUG->print("RS_Insert::update: scale");
                ne->scale(data.insertionPoint, data.scaleFactor);
                // Rotate:
//                                RS_DEBUG->print("RS_Insert::update: rotate");
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
//                                        RS_DEBUG->print("RS_Insert::update: updating new entity");
                                        ne->update();
                                }

//                                RS_DEBUG->print("RS_Insert::update: adding new entity");
                appendEntity(ne);
//                std::cout<<"done # of entity: "<<i_en_counts<<std::endl;
            }
        }
    }
    calculateBorders();

        RS_DEBUG->print("RS_Insert::update: OK");
}



/**
 * @return Pointer to the block associated with this Insert or
 *   nullptr if the block couldn't be found. Blocks are requested
 *   from the blockSource if one was supplied and otherwise from
 *   the closest parent graphic.
 */
RS_Block* RS_Insert::getBlockForInsert() const{
	RS_Block* blk = nullptr;
		if (block) {
			blk=block;
			return blk;
        }

    RS_BlockList* blkList;

	if (!data.blockSource) {
		if (getGraphic()) {
            blkList = getGraphic()->getBlockList();
        } else {
			blkList = nullptr;
        }
    } else {
        blkList = data.blockSource;
    }

	if (blkList) {
        blk = blkList->find(data.name);
    }

	if (blk) {
    }

        block = blk;

    return blk;
}


/**
 * Is this insert visible? (re-implementation from RS_Entity)
 *
 * @return true Only if the entity and the block and the layer it is on
 * are visible.
 * The Layer might also be nullptr. In that case the layer visibility
 * is ignored.
 * The Block might also be nullptr. In that case the block visibility
 * is ignored.
 */
bool RS_Insert::isVisible() const
{
    RS_Block* blk = getBlockForInsert();
	if (blk) {
        if (blk->isFrozen()) {
            return false;
        }
    }

    return RS_Entity::isVisible();
}


RS_VectorSolutions RS_Insert::getRefPoints() const
{
	return RS_VectorSolutions{data.insertionPoint};
}



RS_Vector RS_Insert::getNearestRef(const RS_Vector& coord,
									 double* dist) const{

        return getRefPoints().getClosest(coord, dist);
}



void RS_Insert::move(const RS_Vector& offset) {
        RS_DEBUG->print("RS_Insert::move: offset: %f/%f",
                offset.x, offset.y);
        RS_DEBUG->print("RS_Insert::move1: insertionPoint: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y);
    data.insertionPoint.move(offset);
        RS_DEBUG->print("RS_Insert::move2: insertionPoint: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::rotate(const RS_Vector& center, const double& angle) {
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
void RS_Insert::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
        RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f "
            "/ center: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y,
                center.x, center.y);
    data.insertionPoint.rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle+angleVector.angle());
        RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::scale(const RS_Vector& center, const RS_Vector& factor) {
        RS_DEBUG->print("RS_Insert::scale1: insertionPoint: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y);
    data.insertionPoint.scale(center, factor);
    data.scaleFactor.scale(RS_Vector(0.0, 0.0), factor);
    data.spacing.scale(RS_Vector(0.0, 0.0), factor);
        RS_DEBUG->print("RS_Insert::scale2: insertionPoint: %f/%f",
                data.insertionPoint.x, data.insertionPoint.y);
    update();
}



void RS_Insert::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);

		RS_Vector vec = RS_Vector::polar(1.0, data.angle);
        vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
        data.angle = RS_Math::correctAngle(vec.angle()-M_PI);

        data.scaleFactor.x*=-1;

    update();
}


std::ostream& operator << (std::ostream& os, const RS_Insert& i) {
    os << " Insert: " << i.getData() << std::endl;
    return os;
}

