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

#include "rs_document.h"

#include <set>

#include "lc_undosection.h"
#include "rs_debug.h"

/**
 * Constructor.
 *
 * @param parent Parent of the document. Often that's NULL but
 *        for blocks it's the blocklist.
 */
RS_Document::RS_Document(RS_EntityContainer* parent)
    : RS_EntityContainer{parent}
    , m_activePen {RS_Color{RS2::FlagByLayer}, RS2::WidthByLayer, RS2::LineByLayer}
    , m_selectedSet{std::make_unique<LC_SelectedSet>()}{
    RS_DEBUG->print("RS_Document::RS_Document() ");
}

RS_Document::~RS_Document() {
}

void RS_Document::addEntity(const RS_Entity* entity) {
    entity->m_parent = this;
    RS_EntityContainer::addEntity(entity);
}

/**
 * Overwritten to set modified flag when undo cycle finished with undoable(s).
 */
void RS_Document::endUndoCycle(){
    if (hasUndoable()) {
        setModified(true);
    }
    m_selectedSet->enableListeners();
    RS_Undo::endUndoCycle();
    setAutoUpdateBorders(m_savedAutoUpdateBorders);
    calculateBorders();
}

void RS_Document::startUndoCycle() {
    m_selectedSet->disableListeners();
    RS_Undo::startUndoCycle();
    m_savedAutoUpdateBorders = getAutoUpdateBorders();
    setAutoUpdateBorders(false);
}

RS_EntityContainer::RefInfo RS_Document::getNearestSelectedRefInfo(const RS_Vector& coord, double* dist) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    RS_Entity* closestPointEntity = nullptr;

    QList<RS_Entity*>selection;
    collectSelected(selection);

    for (RS_Entity* en : std::as_const(selection)) {
        if (en->isVisible() && !en->isParentSelected()) {
            double curDist  = 0.; // currently measured distance
            const RS_Vector point = en->getNearestSelectedRef(coord, &curDist);
            if (point.valid && curDist < minDist) {
                closestPoint       = point;
                closestPointEntity = en;
                minDist            = curDist;
                if (dist != nullptr) {
                    *dist = minDist;
                }
            }
        }
    }
    const RefInfo result{closestPoint, closestPointEntity};
    return result;
}

bool RS_Document::undoableModify(LC_GraphicViewport* viewport, const FunUndoable& funModification, const FunSelection& funSelection) {
    const LC_UndoSection undo(this, viewport);
    const bool result = undo.undoableExecute(funModification, funSelection);
    return result;
}

bool RS_Document::undoableModify(LC_GraphicViewport* viewport, const FunUndoable& funModification) {
    return undoableModify(viewport, funModification,  []([[maybe_unused]]LC_DocumentModificationBatch&ctx, [[maybe_unused]]RS_Document* doc){});
}

void RS_Document::startBulkUndoablesCleanup() {
    m_savedAutoUpdateBorders = getAutoUpdateBorders();
    setAutoUpdateBorders(false);
}

void RS_Document::endBulkUndoablesCleanup() {
    setAutoUpdateBorders(m_savedAutoUpdateBorders);
    calculateBorders();
}

bool RS_Document::hasSelection() const {
    return m_selectedSet->hasSelection();
}

bool RS_Document::isSingleEntitySelected() const {
    QList<RS_Entity*> entitiesList;
    collectSelected(entitiesList);
    return entitiesList.size() == 1;
}

bool RS_Document::collectSelected(QList<RS_Entity*>& entitiesList) const {
    const auto selection = getSelection();
    if (selection->isEmpty()) {
        return false;
    }
    return selection->collectSelectedEntities(entitiesList);
}

RS_Document::LC_SelectionInfo RS_Document::getSelectionInfo(const QList<RS2::EntityType> &types) const {
    LC_SelectionInfo result;
    const std::set<RS2::EntityType> type{types.cbegin(), types.cend()};
    QList<RS_Entity*> selection;

    if (collectSelected(selection)) {
        for (const auto e : std::as_const(selection)) {
            if (types.empty() || type.count(e->rtti()) != 0) {
                result.entitiesCount++;
                const double entityLength = e->getLength();
                if (entityLength >= 0.) {
                    result.totalLength += entityLength;
                }
            }
        }
    }
    return result;
}

bool RS_Document::collectSelected(QList<RS_Entity*> &collect, [[maybe_unused]] bool deep, const QList<RS2::EntityType>&types) {
    const auto selection = getSelection();
    if (selection->isEmpty()) {
        return false;
    }
    return selection->collectSelectedEntities(collect, types);
}

void RS_Document::fireUndoStateChanged(const bool undoAvailable, const bool redoAvailable) const {
    if (m_modificationListener != nullptr) {
        m_modificationListener->undoStateChanged(this, undoAvailable, redoAvailable);
    }
}
