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
}

void LC_UndoSection::undoableAdd(RS_Entity* e) const {
    m_document->undoableAdd(e);
}

void LC_UndoSection::addUndoable(RS_Undoable* u) const {
    m_document->addUndoable(u);
}

void LC_UndoSection::undoableReplace(RS_Entity* entityToDelete, RS_Entity* entityToAdd) const {
     m_document->undoableDelete(entityToDelete);
     m_document->undoableAdd(entityToAdd);
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
                const auto layer = e->getLayer(true);
                if (!layer->isLocked()) {
                      m_document->undoableDelete(e);
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
    const auto graphic = m_document->getGraphic();
    RS_Layer *activeLayer = setActiveLayer ? graphic->getActiveLayer() : nullptr;
    const RS_Pen activePen      = setActivePen ? graphic->getActivePen() : RS_Pen();
    for (const auto ent: entitiesToInsert) {
        undoableAdd(ent);
        if (setActiveLayer) {
            ent->setLayer(activeLayer);
        }
        if (setActivePen){
            ent->setPen(activePen);
        }
        const auto rtti = ent->rtti();
        if (rtti == RS2::EntityInsert || RS2::isDimensionalEntity(rtti) ||
            RS2::isTextEntity(rtti) || rtti == RS2::EntityHatch || rtti == RS2::EntityImage) { // fixme - spline
            ent->update();
        }
    }
}
