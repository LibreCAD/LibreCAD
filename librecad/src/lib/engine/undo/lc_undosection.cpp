/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "lc_undosection.h"

#include "lc_graphicviewport.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_layer.h"

namespace {
// Block references and the entities that belong to one are the only entities told apart by lineage.
bool hasTrackedLineage(const RS_Entity* e) {
    return e->rtti() == RS2::EntityInsert || e->ownerLineage() != 0;
}
}

LC_UndoSection::LC_UndoSection(RS_Document *doc, LC_GraphicViewport* view) :
    m_document( doc),
    m_viewport(view){
    Q_ASSERT(doc != nullptr && view != nullptr);
    m_document->startUndoCycle();
}

LC_UndoSection::~LC_UndoSection(){
    try {
        RS_Undoable* relativeZeroUndoable = m_viewport->getRelativeZeroUndoable();
        if (relativeZeroUndoable != nullptr) {
            m_document->addUndoable(relativeZeroUndoable);
        }
        m_document->endUndoCycle();
    }
    catch (...) {
    }
}

void LC_UndoSection::undoableDelete(RS_Entity* e) const {
    m_document->undoableDelete(e);
    if (e->sourceHandle() != 0) {
        m_freedHandles.insert(e->sourceHandle());
    }
    if (hasTrackedLineage(e)) {
        m_freedLineages.insert(e->lineage());
    }
}

void LC_UndoSection::undoableAdd(RS_Entity* e) const {
    // Only an entity taking the place of one this section deleted keeps its DWG identity, with its
    // children's; anything else is new to the drawing, even if made of copies.
    if (e->sourceHandle() == 0 || !m_freedHandles.remove(e->sourceHandle())) {
        e->clearDwgProvenance(RS_Entity::Identity);
    }
    // The same for what a block reference, or an entity that belongs to one, is in the drawing, read from
    // a file with handles or not. Any other is a copy: clone number n of an entity belongs to clone number n
    // of what the entity belonged to, so a block reference copied with its attribute texts keeps them and
    // one copied alone has none. The clone that takes the place of a deleted original has a number too: of a
    // block reference and its text, one may be deleted and the other, on a locked layer, not.
    if (hasTrackedLineage(e)) {
        const unsigned long long original = e->lineage();
        const int number = ++m_clonesAdded[original];
        if (!m_freedLineages.remove(original)) {
            e->setLineage(copyLineage(original, number));
            if (e->ownerLineage() != 0) {
                e->setOwnerLineage(copyLineage(e->ownerLineage(), number));
            }
        }
    }
    m_document->undoableAdd(e);
}

unsigned long long LC_UndoSection::copyLineage(const unsigned long long lineage, const int number) const {
    unsigned long long& made = m_copyLineages[{lineage, number}];
    if (made == 0) {
        made = RS_Entity::newLineage();
    }
    return made;
}

void LC_UndoSection::addUndoable(RS_Undoable* u) const {
    m_document->addUndoable(u);
}

void LC_UndoSection::undoableReplace(RS_Entity* entityToDelete, RS_Entity* entityToAdd) const {
    undoableDelete(entityToDelete);
    undoableAdd(entityToAdd);
}

bool LC_UndoSection::undoableExecute(const RS_Document::FunUndoable& doUndoable) const {
    return undoableExecute(doUndoable, []([[maybe_unused]]LC_DocumentModificationBatch&ctx, [[maybe_unused]]RS_Document* doc){});
}

bool LC_UndoSection::undoableExecute(const RS_Document::FunUndoable& doUndoable, const RS_Document::FunSelection& doSelection) const {
    LC_DocumentModificationBatch ctx;
    const bool success = doUndoable(ctx);
    ctx.success = success;
    if (success) {
        if (!ctx.entitiesToDelete.isEmpty()) {
            for (const auto e: std::as_const(ctx.entitiesToDelete)) {
                if (e == nullptr) {
                    continue;
                }
                const auto layer = e->getLayer(true);
                // Null layer: treat as unlocked (e.g. mid-import / no graphic).
                if (layer == nullptr || !layer->isLocked()) {
                    undoableDelete(e);
                }
            }
        }
        if (!ctx.entitiesToAdd.isEmpty()) {
            setupAndUndoableAdd(ctx.entitiesToAdd, ctx.setActiveLayer, ctx.setActivePen);
        }
    }
    doSelection(ctx, m_document);
    m_viewport->notifyChanged();
    return success;
}

void LC_UndoSection::setupAndUndoableAdd(const QList<RS_Entity*>& entitiesToInsert, const bool setActiveLayer, const bool setActivePen) const {
    const auto graphic = m_document != nullptr ? m_document->getGraphic() : nullptr;
    RS_Layer *activeLayer = (setActiveLayer && graphic != nullptr) ? graphic->getActiveLayer() : nullptr;
    const RS_Pen activePen = (setActivePen && graphic != nullptr) ? graphic->getActivePen() : RS_Pen();
    for (const auto ent: entitiesToInsert) {
        if (ent == nullptr) {
            continue;
        }
        undoableAdd(ent);
        if (setActiveLayer && graphic != nullptr) {
            ent->setLayer(activeLayer);
        }
        if (setActivePen && graphic != nullptr){
            ent->setPen(activePen);
        }
        const auto rtti = ent->rtti();
        if (rtti == RS2::EntityInsert || RS2::isDimensionalEntity(rtti) ||
            RS2::isTextEntity(rtti) || rtti == RS2::EntityHatch || rtti == RS2::EntityImage) { // fixme - spline
            ent->update();
        }
    }
}
