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

#include <cstddef>

#include "lc_insert_transform.h"
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

    RS_InsertData(const RS_InsertData &other);
    RS_InsertData& operator=(const RS_InsertData& other) = default;

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

    /**
     * @brief usableScale a scale factor safe to store and to write out.
     * Zero, sub-tolerance, infinite and NaN factors all leave the block
     * transform singular or meaningless, and other CAD applications reject
     * such an INSERT (#1428); they become 1. Negative factors are mirrors
     * and are kept.
     */
    static double usableScale(double factor);

    //! \brief usableScale the same rule on every axis: any degenerate
    //! component makes the whole block transform unusable, not just z.
    static RS_Vector usableScale(const RS_Vector& scale);

	QString name;
	RS_Vector insertionPoint;
	RS_Vector scaleFactor;
    RS_Vector extrusion {0.0, 0.0, 1.0};
    double angle=0.;
    // DXF groups 70/71 are optional and default to one ordinary INSERT.
    // Keep the in-memory default aligned with that contract so a default
    // payload never masquerades as an empty MINSERT array.
    int cols=1, rows=1;
	RS_Vector spacing;
    RS_BlockList* blockSource = nullptr;
    RS2::UpdateMode updateMode{};
};

std::ostream& operator << (std::ostream& os, const RS_InsertData& d);

/**
 * Bounds work performed while expanding a BLOCK reference.  Callers that
 * process untrusted or unusually deep drawings can provide a stricter
 * budget to update(const RS_InsertExpansionBudget&).
 */
struct RS_InsertExpansionBudget {
    static constexpr std::size_t DefaultMaxNestingDepth = 256U;
    static constexpr std::size_t DefaultMaxDerivedEntities = 1000000U;
    // This is separate from derived entities: an empty BLOCK can otherwise
    // make a huge MINSERT grid consume unbounded traversal work.
    static constexpr std::size_t DefaultMaxArrayInstances = DefaultMaxDerivedEntities;

    std::size_t maxNestingDepth {DefaultMaxNestingDepth};
    std::size_t maxDerivedEntities {DefaultMaxDerivedEntities};
    std::size_t maxArrayInstances {DefaultMaxArrayInstances};

    [[nodiscard]] bool isValid() const noexcept {
        return maxNestingDepth > 0U && maxDerivedEntities > 0U
               && maxArrayInstances > 0U;
    }
};

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

    /** @return Copy of m_data that defines the insert. **/
    RS_InsertData getData() const{
        return m_data;
    }

        /**
         * Reimplementation of reparent. Invalidates m_block cache pointer.
         */
    void reparent(RS_EntityContainer* parent)  override{
                RS_Entity::reparent(parent);
                invalidateBlockCache();
    }

    RS_Block* getBlockForInsert() const;

    void update() override;
    void update(const RS_InsertExpansionBudget& budget);
    void calculateBorders() override;

    QString getName() const {
        return m_data.name;
    }

    void setName(const QString& newName) {
        if (m_data.name == newName)
            return;
        m_data.name = newName;
        invalidateBlockCache();
        update();
    }

    RS_Vector getInsertionPoint() const {
        return m_data.insertionPoint;
    }
    void setInsertionPoint(const RS_Vector& i) {
        m_data.insertionPoint = i;
    }

    RS_Vector getScale() const {
        return m_data.scaleFactor;
    }

    void setScale(const RS_Vector& s) {
        m_data.scaleFactor = s;
    }

    double getAngle() const {
        return m_data.angle;
    }
    void setAngle(double a) {
        m_data.angle = a;
    }

    int getCols() const {
        return m_data.cols;
    }

    void setCols(int c) {
        m_data.cols = c;
    }

    int getRows() const {
        return m_data.rows;
    }

    void setRows(int r) {
        m_data.rows = r;
    }

    RS_Vector getSpacing() const {
        return m_data.spacing;
    }

    void setSpacing(const RS_Vector& s) {
        m_data.spacing = s;
    }

    [[nodiscard]] LC_InsertSourceEditStatus lastSourceEditStatus() const noexcept {
        return m_lastSourceEditStatus;
    }

    [[nodiscard]] bool lastSourceEditSucceeded() const noexcept {
        return m_lastSourceEditStatus == LC_InsertSourceEditStatus::Ok;
    }

    bool isVisible() const override;

    RS_VectorSolutions getRefPoints() const override;

    RS_Vector getMiddlePoint() const override {
        return {};
    }

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    friend std::ostream& operator <<(std::ostream& os, const RS_Insert& i);

protected:
    void applySourceEdit(const LC_InsertTransform& edit, const char* operation);
    void setLastSourceEditStatus(LC_InsertSourceEditStatus status) noexcept {
        m_lastSourceEditStatus = status;
    }

    void invalidateBlockCache() const noexcept {
        m_block = nullptr;
        m_blockList = nullptr;
        m_blockListGeneration = 0U;
    }

    RS_InsertData m_data;
    LC_InsertSourceEditStatus m_lastSourceEditStatus {LC_InsertSourceEditStatus::Ok};
    mutable RS_Block* m_block = nullptr;
    mutable const RS_BlockList* m_blockList = nullptr;
    mutable std::size_t m_blockListGeneration = 0U;

    RS_Vector doGetNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;
};


#endif
