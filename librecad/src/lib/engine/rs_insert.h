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


#ifndef RS_INSERT_H
#define RS_INSERT_H

#include "rs_entitycontainer.h"

class RS_BlockList;

/**
 * Holds the data that defines an insert.
 */
struct RS_InsertData {
    /**
     * Default constructor.
     */
    RS_InsertData() = default;

	/**
	 * @param name The name of the block used as an identifier.
	 * @param insertionPoint Insertion point of the block.
	 * @param scaleFactor Scale factor in x / y.
	 * @param angle Rotation angle.
	 * @param cols Number of cols if we insert a whole array.
	 * @param rows Number of rows if we insert a whole array.
	 * @param spacing Spacing between rows and cols.
	 * @param blockSource Source for the block to insert if other than parent.
	 *    Normally blocks are requested from the entity's parent but the
	 *    block can also come from another resource. RS_Text uses that
	 *    to share the blocks (letters) from a font.
	 * @param updateMode RS2::Update will update the insert entity instantly
	 *    RS2::NoUpdate will not update the insert. You can update
	 *	  it later manually using the update() method. This is
	 *    often the case since you might want to adjust attributes
	 *    after creating an insert entity.
	 */
	RS_InsertData(const QString& name,
				  RS_Vector insertionPoint,
				  RS_Vector scaleFactor,
				  double angle,
				  int cols, int rows, RS_Vector spacing,
                  RS_BlockList* blockSource = nullptr,
				  RS2::UpdateMode updateMode = RS2::Update);

	QString name;
	RS_Vector insertionPoint;
	RS_Vector scaleFactor;
    double angle=0.;
    int cols=0, rows=0;
	RS_Vector spacing;
    RS_BlockList* blockSource = nullptr;
    RS2::UpdateMode updateMode{};
};

std::ostream& operator << (std::ostream& os, const RS_InsertData& d);

/**
 * An insert inserts a block into the drawing at a certain location
 * with certain attributes (angle, scale, ...).
 * Inserts don't really contain other entities internally. They just
 * refer to a block. However, to the outside world they act exactly
 * like EntityContainer.
 *
 * @author Andrew Mustun
 */
class RS_Insert : public RS_EntityContainer {
public:
    RS_Insert(RS_EntityContainer* parent,
              const RS_InsertData& d);

    RS_Entity* clone() const override;

    /** @return RS2::EntityInsert */
    RS2::EntityType rtti() const  override{
        return RS2::EntityInsert;
    }

    /** @return Copy of data that defines the insert. **/
    RS_InsertData getData() const{
        return data;
    }

        /**
         * Reimplementation of reparent. Invalidates block cache pointer.
         */
    void reparent(RS_EntityContainer* parent)  override{
                RS_Entity::reparent(parent);
                block = nullptr;
    }

	RS_Block* getBlockForInsert() const;

    void update() override;

    QString getName() const {
        return data.name;
    }

    void setName(const QString& newName) {
        data.name = newName;
        update();
    }

    RS_Vector getInsertionPoint() const {
        return data.insertionPoint;
    }
    void setInsertionPoint(const RS_Vector& i) {
        data.insertionPoint = i;
    }

    RS_Vector getScale() const {
        return data.scaleFactor;
    }

    void setScale(const RS_Vector& s) {
        data.scaleFactor = s;
    }

    double getAngle() const {
        return data.angle;
    }
    void setAngle(double a) {
        data.angle = a;
    }

    int getCols() const {
        return data.cols;
    }

    void setCols(int c) {
        data.cols = c;
    }

    int getRows() const {
        return data.rows;
    }

    void setRows(int r) {
        data.rows = r;
    }

    RS_Vector getSpacing() const {
        return data.spacing;
    }

    void setSpacing(const RS_Vector& s) {
        data.spacing = s;
    }

    bool isVisible() const override;

    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getMiddlePoint(void) const  override{
        return {};
    }
    RS_Vector getNearestRef(const RS_Vector& coord,
                            double* dist = nullptr) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    friend std::ostream& operator << (std::ostream& os, const RS_Insert& i);

    void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

protected:
    RS_InsertData data{};
    mutable RS_Block* block = nullptr;
};


#endif
