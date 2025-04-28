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

#include "rs_insert.h"

#include<iostream>

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_pen.h"

class RS_Circle;
class RS_Arc;

namespace {

// Minimum scaling factor allowed
constexpr double MIN_Scale_Factor = 1.0e-6;

// update the entity pen according to the blockPen
RS_Pen updatePen(RS_Pen&& pen, const RS_Pen& blockPen)
{
    // color from block (free floating):
    if (pen.getColor()==RS_Color(RS2::FlagByBlock)) {
        pen.setColor(blockPen.getColor());
    }

    // line width from block (free floating):
    if (pen.getWidth()==RS2::WidthByBlock) {
        pen.setWidth(blockPen.getWidth());
    }

    // line type from block (free floating):
    if (pen.getLineType()==RS2::LineByBlock) {
        pen.setLineType(blockPen.getLineType());
    }

    return pen;
}

}
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
  ,updateMode(_updateMode){
}

RS_InsertData::RS_InsertData(const RS_InsertData &other):
   name(other.name)
  ,insertionPoint(other.insertionPoint)
  ,scaleFactor(other.scaleFactor)
  ,angle(other.angle)
  ,cols(other.cols)
  ,rows(other.rows)
  ,spacing(other.spacing)
  ,blockSource(other.blockSource)
  ,updateMode(other.updateMode){
}

std::ostream& operator << (std::ostream& os,
									 const RS_InsertData& d) {
	   os << "(" << d.name.toLatin1().data() << ")";
	   return os;
   }
/**
 * @param parent The graphic this m_block belongs to.
 */
RS_Insert::RS_Insert(RS_EntityContainer* parent,
                     const RS_InsertData& d)
        : RS_EntityContainer(parent)
       , m_data(d)
{

    if (m_data.updateMode!=RS2::NoUpdate) {
        RS_Insert::update();
        //calculateBorders();
    }
}

RS_Entity* RS_Insert::clone() const{
	auto i = new RS_Insert(*this);
	i->setOwner(isOwner());
	i->detach();
	return i;
}

/**
 * Updates the entity buffer of this insert entity. This method
 * needs to be called whenever the block this insert is based on changes.
 */
void RS_Insert::update() {

    RS_DEBUG->print("RS_Insert::update");
    RS_DEBUG->print("RS_Insert::update: name: %s", m_data.name.toLatin1().data());
    //        RS_DEBUG->print("RS_Insert::update: insertionPoint: %f/%f",
    //                data.insertionPoint.x, data.insertionPoint.y);

    if (updateEnabled==false) {
        return;
    }

    clear();

    RS_Block* blk = getBlockForInsert();
    if (blk == nullptr) {
        RS_DEBUG->print("RS_Insert::update: Block is nullptr");
        return;
    }

    if (isUndone()) {
        RS_DEBUG->print("RS_Insert::update: Insert is in undo list");
        return;
    }

    if (std::abs(m_data.scaleFactor.x)<MIN_Scale_Factor || std::abs(m_data.scaleFactor.y)<MIN_Scale_Factor) {
        RS_DEBUG->print("RS_Insert::update: scale factor is 0");
        return;
    }

    RS_DEBUG->print("RS_Insert::update: cols: %d, rows: %d",
                    m_data.cols, m_data.rows);
    RS_DEBUG->print("RS_Insert::update: block has %d entities",
                    blk->count());
        for(auto* e: *blk){
            for (int c=0; c<m_data.cols; ++c) {
//            RS_DEBUG->print("RS_Insert::update: col %d", c);
                for (int r=0; r<m_data.rows; ++r) {
//                i_en_counts++;
//                RS_DEBUG->print("RS_Insert::update: row %d", r);

                    if (e->rtti()==RS2::EntityInsert &&
                            m_data.updateMode!=RS2::PreviewUpdate) {

//                                        RS_DEBUG->print("RS_Insert::update: updating sub-insert");
                        e->update();
                }

//                                RS_DEBUG->print("RS_Insert::update: cloning entity");

                    RS_Entity* ne = nullptr;
                    if ( (m_data.scaleFactor.x - m_data.scaleFactor.y)>MIN_Scale_Factor) {
                        if (e->rtti()== RS2::EntityArc) {
                            auto a= static_cast<RS_Arc*>(e);
                            ne = new RS_Ellipse{this,
                            {a->getCenter(), {a->getRadius(), 0.},
                                    1, a->getAngle1(), a->getAngle2(),
                                    a->isReversed()}};
                            ne->setLayer(e->getLayer());
                            ne->setPen(e->getPen(false));
                        } else if (e->rtti()== RS2::EntityCircle) {
                            auto a= static_cast<RS_Circle*>(e);
                            ne = new RS_Ellipse{this,
                            { a->getCenter(), {a->getRadius(), 0.}, 1, 0., 2.*M_PI, false}};
                            ne->setLayer(e->getLayer());
                            ne->setPen(e->getPen(false));
                        } else {
                            ne = e->clone();
                        }
                    } else {
                        ne = e->clone();
                    }
                    ne->setUpdateEnabled(false);
                // if entity layer are 0 set to insert layer to allow "1 layer control" bug ID #3602152
                    RS_Layer *l= ne->getLayer();//special fontchar block don't have
                    if (l != nullptr  && ne->getLayer()->getName() == "0")
                    ne->setLayer(getLayer());
                    ne->setParent(this);
                    ne->setVisible(getFlag(RS2::FlagVisible));

//                                RS_DEBUG->print("RS_Insert::update: transforming entity");

                // Move:
//                                RS_DEBUG->print("RS_Insert::update: move 1");
                    if (std::abs(m_data.scaleFactor.x)>MIN_Scale_Factor &&
                            std::abs(m_data.scaleFactor.y)>MIN_Scale_Factor) {
                        ne->move(m_data.insertionPoint +
                                 RS_Vector(m_data.spacing.x/m_data.scaleFactor.x*c,
                                           m_data.spacing.y/m_data.scaleFactor.y*r));
                    }
                    else {
                        ne->move(m_data.insertionPoint);
                    }
                // Move because of block base point:
//                                RS_DEBUG->print("RS_Insert::update: move 2");
                    ne->move(blk->getBasePoint()*(-1));
                // Scale:
//                                RS_DEBUG->print("RS_Insert::update: scale");
                    ne->scale(m_data.insertionPoint, m_data.scaleFactor);
                // Rotate:
//                                RS_DEBUG->print("RS_Insert::update: rotate");
                    ne->rotate(m_data.insertionPoint, m_data.angle);

                   // RS_DEBUG->print(RS_Debug::D_ERROR, "ne: angle: %lg\n", data.angle);
                // Select:
                    ne->setSelected(isSelected());

                // individual entities can be on indiv. layers
                    RS_Pen tmpPen = updatePen(ne->getPen(false), getPen());
                // now that we've evaluated all flags, let's strip them:
                // TODO: strip all flags (width, line type)
                //tmpPen.setColor(tmpPen.getColor().stripFlags());
                    ne->setPen(tmpPen);

                    ne->setUpdateEnabled(true);

                // insert must be updated even in preview mode
                    if (m_data.updateMode != RS2::PreviewUpdate
                            || ne->rtti() == RS2::EntityInsert) {
                        //RS_DEBUG->print("RS_Insert::update: updating new entity");
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
 * @return Pointer to the m_block associated with this Insert or
 *   nullptr if the m_block couldn't be found. Blocks are requested
 *   from the blockSource if one was supplied and otherwise from
 *   the closest parent graphic.
 */
RS_Block* RS_Insert::getBlockForInsert() const{
    if (m_block != nullptr) {
        return m_block;
    }

    RS_BlockList* blkList = nullptr;

    if (!m_data.blockSource) {
        if (getGraphic()) {
            blkList = getGraphic()->getBlockList();
        }
    } else {
        blkList = m_data.blockSource;
    }

    RS_Block* blk = nullptr;
    if (blkList != nullptr) {
        blk = blkList->find(m_data.name);
    }

    m_block = blk;

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
bool RS_Insert::isVisible() const{
    RS_Block* blk = getBlockForInsert();
    if (blk != nullptr) {
        if (blk->isFrozen()) {
            return false;
        }
    }

    return RS_Entity::isVisible();
}

RS_VectorSolutions RS_Insert::getRefPoints() const{
    return RS_VectorSolutions{m_data.insertionPoint};
}

RS_Vector RS_Insert::getNearestRef(const RS_Vector& coord,
									 double* dist) const{
        return getRefPoints().getClosest(coord, dist);
}

void RS_Insert::move(const RS_Vector& offset) {
    RS_DEBUG->print("RS_Insert::move: offset: %f/%f", offset.x, offset.y);
    RS_DEBUG->print("RS_Insert::move1: insertionPoint: %f/%f", m_data.insertionPoint.x, m_data.insertionPoint.y);
    m_data.insertionPoint.move(offset);
    RS_DEBUG->print("RS_Insert::move2: insertionPoint: %f/%f", m_data.insertionPoint.x, m_data.insertionPoint.y);
    update();
}

void RS_Insert::rotate(const RS_Vector& center, double angle) {
    RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f / center: %f/%f",
                    m_data.insertionPoint.x, m_data.insertionPoint.y,
                    center.x, center.y);
    m_data.insertionPoint.rotate(center, angle);
    m_data.angle = RS_Math::correctAngle(m_data.angle + angle);
    RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f", m_data.insertionPoint.x, m_data.insertionPoint.y);
    update();
}

void RS_Insert::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_DEBUG->print("RS_Insert::rotate1: insertionPoint: %f/%f "
                    "/ center: %f/%f",
                    m_data.insertionPoint.x, m_data.insertionPoint.y,
                    center.x, center.y);
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.angle = RS_Math::correctAngle(m_data.angle + angleVector.angle());
    RS_DEBUG->print("RS_Insert::rotate2: insertionPoint: %f/%f",
                    m_data.insertionPoint.x, m_data.insertionPoint.y);
    update();
}

void RS_Insert::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_DEBUG->print("RS_Insert::scale1: insertionPoint: %f/%f",
                    m_data.insertionPoint.x, m_data.insertionPoint.y);
    m_data.insertionPoint.scale(center, factor);
    m_data.scaleFactor.scale(RS_Vector(0.0, 0.0), factor);
    m_data.spacing.scale(RS_Vector(0.0, 0.0), factor);
    RS_DEBUG->print("RS_Insert::scale2: insertionPoint: %f/%f",
                    m_data.insertionPoint.x, m_data.insertionPoint.y);
    update();

}

void RS_Insert::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.insertionPoint.mirror(axisPoint1, axisPoint2);
    RS_Vector vec = RS_Vector::polar(1.0, m_data.angle);
    vec.mirror(RS_Vector(0.0, 0.0), axisPoint2 - axisPoint1);
    m_data.angle = RS_Math::correctAngle(vec.angle() - M_PI);
    m_data.scaleFactor.x *= -1;
    update();
}

std::ostream& operator << (std::ostream& os, const RS_Insert& i) {
    os << " Insert: " << i.getData() << std::endl;
    return os;
}
