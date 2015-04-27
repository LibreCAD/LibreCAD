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
	~RS_InsertData() = default;

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
				  RS_BlockList* blockSource = NULL,
				  RS2::UpdateMode updateMode = RS2::Update);

	QString name;
	RS_Vector insertionPoint;
	RS_Vector scaleFactor;
	double angle;
	int cols, rows;
	RS_Vector spacing;
	RS_BlockList* blockSource;
	RS2::UpdateMode updateMode;
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
	virtual ~RS_Insert() = default;

	virtual RS_Entity* clone() const;

    /** @return RS2::EntityInsert */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityInsert;
    }

    /** @return Copy of data that defines the insert. **/
    RS_InsertData getData() const {
        return data;
    }

        /**
         * Reimplementation of reparent. Invalidates block cache pointer.
         */
    virtual void reparent(RS_EntityContainer* parent) {
                RS_Entity::reparent(parent);
                block = NULL;
    }

	RS_Block* getBlockForInsert() const;

    virtual void update();

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

	virtual bool isVisible() const;

	virtual RS_VectorSolutions getRefPoints() const;
    virtual RS_Vector getMiddlePoint(void) const{
            return RS_Vector(false);
    }
    virtual RS_Vector getNearestRef(const RS_Vector& coord,
									 double* dist = nullptr) const;

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

    friend std::ostream& operator << (std::ostream& os, const RS_Insert& i);

protected:
    RS_InsertData data;
	mutable RS_Block* block;
};


#endif
