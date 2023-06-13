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


#ifndef RS_BLOCK_H
#define RS_BLOCK_H

#include "rs_document.h"

/**
 * Holds the data that defines a block.
 */
struct RS_BlockData {
	RS_BlockData() = default;

	RS_BlockData(const QString& name,
	           const RS_Vector& basePoint,
			   bool frozen);

	bool isValid() const;

	/**
	 * Block name. Acts as an id.
	 */
	QString name;
	/**
	 * Base point of the Block. Usually 0/0 since blocks can be moved around
	 * using the insertion point of Insert entities.
	 */
	RS_Vector basePoint;

	bool frozen {false};              //!< Frozen flag
    mutable bool visibleInBlockList {true};   //!< Visible in block list
    mutable bool selectedInBlockList {false}; //!< selected in block list
};



/**
 * A block is a group of entities. A block unlike an other entity
 * container has a base point which defines the offset of the
 * block. Note that although technically possible, a block should
 * never be part of the entity tree of a graphic. Blocks are 
 * stored in a separate list inside the graphic document (a block list).
 * The graphic can contain RS_Insert entities that refer to such 
 * blocks.
 *
 * blocks are documents and can therefore be handled by graphic views.
 *
 * @author Andrew Mustun
 */
class RS_Block : public RS_Document {

	friend class RS_BlockList;

public:
    /**
     * @param parent The graphic this block belongs to.
     * @param blockData defining data of the block.
     */
    RS_Block(RS_EntityContainer* parent, const RS_BlockData& d);

    RS_Entity* clone() const override;

    /** @return RS2::EntityBlock */
    RS2::EntityType rtti() const override{
        return RS2::EntityBlock;
    }

    /**
     * @return Name of this block (the name is an Id for this block).
     */
    QString getName() const {
		return data.name;
    }

    /**
     * @return base point of this block.
     */
    RS_Vector getBasePoint() const {
        return data.basePoint;
    }

    RS_LayerList* getLayerList() override;
    RS_BlockList* getBlockList() override;

    /**
     * Reimplementation from RS_Document. Does nothing.
     */
    void newDoc() override {
        // do nothing
    }

    /**
     * Reimplementation from RS_Document. Saves the parent graphic document.
     */
    bool save(bool isAutoSave = false) override;

    /**
     * Reimplementation from RS_Document. Does nothing.
     */
    bool saveAs(const QString& filename, RS2::FormatType type, bool force = false) override;

    /**
     * Reimplementation from RS_Document. Does nothing.
     */
    bool open(const QString& , RS2::FormatType) override {
        // do nothing
        return false;
    }
    bool loadTemplate(const QString& , RS2::FormatType) override {
        // do nothing
        return false;
    }

    friend std::ostream& operator << (std::ostream& os, const RS_Block& b);

    /** 
	 * sets a new name for the block. Only called by blocklist to
	 * assure that block names stay unique.
	 */
    void setName(QString newName) {
        data.name = std::move(newName);
    }
    
	/**
     * @retval true if this block is frozen (invisible)
     * @retval false if this block isn't frozen (visible)
     */
    bool isFrozen() const {
        return data.frozen;
    }

    /**
     * Toggles the visibility of this block.
     * Freezes the block if it's not frozen, thaws the block otherwise
     */
    void toggle() {
		data.frozen = !data.frozen;
    }

    /**
     * (De-)freezes this block.
     *
     * @param freeze true: freeze, false: defreeze
     */
    void freeze(bool freeze) {
		data.frozen = freeze;
    }
	
    /**
     * Sets the parent documents modified status to 'm'.
     */
	virtual void setModified(bool m);

    /**
     * Sets only this block modified status to 'm'
     * without touching parent document.
     */
    void setModifiedFlag(bool m) { modified = m; }

    /**
     * Sets the visibility of the Block in block list
     *
     * @param v true: visible, false: invisible
     */
    void visibleInBlockList(bool v);

    /**
     * Returns the visibility of the Block in block list
     */
    bool isVisibleInBlockList() const;

    /**
     * Sets selection state of the block in block list
     *
     * @param v true: selected, false: deselected
     */
    void selectedInBlockList(bool v) const;

    /**
     * Returns selection state of the block in block list
     */
    bool isSelectedInBlockList() const;

    /**
     * Block may contain inserts of other blocks.
     * Find name of the nested block that contain the insert
     * of specified block.
     *
     * @param bName name of the block the nested insert references to
     *
     * @return block name chain to the block that contain searched insert
     */
    QStringList findNestedInsert(const QString& bName);

protected:
	//! Block data
	RS_BlockData data;
};


#endif
