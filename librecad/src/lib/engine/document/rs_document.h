/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_DOCUMENT_H
#define RS_DOCUMENT_H

#include "lc_selectedset.h"
#include "rs_entitycontainer.h"
#include "rs_pen.h"
#include "rs_undo.h"

class LC_GraphicViewport;
class LC_TextStyleList;
class LC_DimStylesList;
class RS_GraphicView;
class LC_UCSList;
class LC_ViewList;
class RS_BlockList;
class RS_LayerList;
class RS_ActionInterface;

struct LC_DocumentModificationBatch {
    bool success{false}; // generic result of applying the batch to document
    QList<RS_Entity*> entitiesToAdd; // list of entities that should be added to the document
    QList<RS_Entity*> entitiesToDelete; // list of entities that should be marked as deleted in the document

    bool setActiveLayer{true}; // flag that indicates that for entities to be added currently active layer should be set
    bool setActivePen{true}; // flag that indicates that for entities to be added currently active pen should be set

    ~LC_DocumentModificationBatch() = default;

    void dontSetActiveLayerAndPen() {
        setActiveLayer = false;
        setActivePen = false;
    }

    void setActiveLayerAndPen(const bool setLayer, const bool setPen) {
        setActiveLayer = setLayer;
        setActivePen = setPen;
    }

    void add(RS_Entity* entity) {
        entitiesToAdd.append(entity);
    }

    void remove(const QList<RS_Entity*>& list) {
        entitiesToDelete.append(list);
    }

    void remove(RS_Entity* e) {
        entitiesToDelete.append(e);
    }

    void replace(RS_Entity* original, RS_Entity* clone) {
        entitiesToDelete.append(original);
        entitiesToAdd.append(clone);
    }

    void clear() {
        entitiesToAdd.clear();
        entitiesToDelete.clear();
    }
};

inline void operator +=(LC_DocumentModificationBatch& ctx, RS_Entity* e) {
    ctx.entitiesToAdd.append(e);
}

inline void operator +=(LC_DocumentModificationBatch& ctx, QList<RS_Entity*>& list) {
    ctx.entitiesToAdd.append(list);
}

inline void operator +=(LC_DocumentModificationBatch& ctx, const QList<RS_Entity*>& list) {
    ctx.entitiesToAdd.append(list);
}

inline void operator -=(LC_DocumentModificationBatch& ctx, RS_Entity* e) {
    ctx.entitiesToDelete.append(e);
}

inline void operator -=(LC_DocumentModificationBatch& ctx, QList<RS_Entity*>& list) {
    ctx.entitiesToDelete.append(list);
}

inline void operator -=(LC_DocumentModificationBatch& ctx, const QList<RS_Entity*>& list) {
    ctx.entitiesToDelete.append(list);
}

class LC_DocumentModificationListener {
public:
    virtual ~LC_DocumentModificationListener() = default;
    virtual void graphicModified(const RS_Graphic* g, bool modified) = 0;
    virtual void undoStateChanged(const RS_Document* g, bool undoAvailable, bool redoAvailable) = 0;
};

/**
 * Base class for documents. Documents can be either graphics or
 * blocks and are typically shown in graphic views. Documents hold
 * an active pen for drawing in the Document, a file name and they
 * know whether they have been modified or not.
 *
 * @author Andrew Mustun
 */
class RS_Document : public RS_EntityContainer, public RS_Undo {
public:
    explicit RS_Document(RS_EntityContainer* parent = nullptr);
    ~RS_Document() override;

    struct LC_SelectionInfo {
        unsigned entitiesCount = 0;
        double totalLength = 0.0;
    };

    using FunUndoable = std::function<bool(LC_DocumentModificationBatch& ctx)>;
    using FunSelection = std::function<void(LC_DocumentModificationBatch& ctx, RS_Document* doc)>;

    virtual RS_LayerList* getLayerList() = 0;
    virtual RS_BlockList* getBlockList() = 0;
    virtual LC_DimStylesList* getDimStyleList() = 0;

    virtual LC_ViewList* getViewList() {
        return nullptr;
    }

    virtual LC_UCSList* getUCSList() {
        return nullptr;
    }

    virtual LC_TextStyleList* getTextStyleList() = 0;

    virtual void initForNewDocument() = 0;

    /**
     * @return true for all document entities (e.g. Graphics or Blocks).
     */
    bool isDocument() const override {
        return true;
    }

    void addEntity(const RS_Entity* entity) override;

    void select(const QList<RS_Entity*>& list, const bool select = true) {
        for (const auto e : list) {
            e->doSelectInDocument(select, this);
        }
    }

    bool hasSelection() const;
    bool isSingleEntitySelected() const;

    void unselect(RS_Entity* entity) {
        entity->doSelectInDocument(false, this);
    }

    bool collectSelected(QList<RS_Entity*>& entitiesList) const;
    LC_SelectionInfo getSelectionInfo(/*bool deep, */ const QList<RS2::EntityType>& types = {}) const;
    virtual bool collectSelected(QList<RS_Entity*>& collect, bool deep, const QList<RS2::EntityType>& types = {});

    /**
     * @return Currently active drawing pen.
     */
    RS_Pen getActivePen() const {
        return m_activePen;
    }

    /**
     * Sets the currently active drawing pen to p.
     */
    void setActivePen(const RS_Pen& p) {
        m_activePen = p;
    }

    /**
     * Sets the documents modified status to 'm'.
     */
    virtual void setModified(const bool m) {
        m_modified = m;
    }

    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    virtual bool isModified() const {
        return m_modified;
    }

    void setGraphicView(RS_GraphicView* g) {
        m_gv = g;
    }

    RS_GraphicView* getGraphicView() const {
        return m_gv;
    } // fixme - sand -- REALLY BAD DEPENDANCE TO UI here, REWORK!

    LC_SelectedSet* getSelection() const {
        return m_selectedSet.get();
    }

    RefInfo getNearestSelectedRefInfo(const RS_Vector& coord, double* dist) const override;

    // high-level document modification functions. All changes (except initial reading) should go via these functions in order to support undo propertly!
    bool undoableModify(LC_GraphicViewport* viewport, const FunUndoable& funModification, const FunSelection& funSelection);
    bool undoableModify(LC_GraphicViewport* viewport, const FunUndoable& funModification);

    void setModificationListener(LC_DocumentModificationListener * listener) {m_modificationListener = listener;}

protected:
    /** Flag set if the document was modified and not yet saved. */
    bool m_modified = false;
    /** Active pen. */
    RS_Pen m_activePen;

    //used to read/save current view
    RS_GraphicView* m_gv = nullptr; // fixme - sand -- REALLY BAD DEPENDANCE TO UI here, REWORK!

    std::unique_ptr<LC_SelectedSet> m_selectedSet;

    void startBulkUndoablesCleanup() override;
    void endBulkUndoablesCleanup() override;

    /**
     * Overwritten to set modified flag when undo cycle finished with undoable(s).
     */
    void endUndoCycle() override;
    void startUndoCycle() override;

    void select(RS_Entity* entity, const bool select = true) {
        if (entity->getFlag(RS2::FlagSelected) != select) {
            entity->doSelectInDocument(select, this);
        }
    }

    void undoableAdd(RS_Entity* entity, const bool undoable = true) {
        addEntity(entity);
        if (undoable) {
            addUndoable(entity);
        }
    }

    void undoableDelete(RS_Entity* e) {
        if (e->getFlag(RS2::FlagSelected)) {
            unselect(e);
        }
        e->setFlag(RS2::FlagDeleted);
        addUndoable(e);
    }

    /**
     * Removes an entity from the entity container. Implementation
     * from RS_Undo.
     */
    void removeUndoable(RS_Undoable* u) override {
        if (u != nullptr && u->undoRtti() == RS2::UndoableEntity && u->isDeleted()) {
            removeEntity(static_cast<RS_Entity*>(u));
        }
    }

    void fireUndoStateChanged(bool undoAvailable, bool redoAvailable) const override;

    bool m_inBulkUndoableCleanup = false;
    bool m_savedAutoUpdateBorders = false;

    LC_DocumentModificationListener* m_modificationListener = nullptr;

    friend class LC_UndoSection;
    friend class RS_Selection;
};
#endif
