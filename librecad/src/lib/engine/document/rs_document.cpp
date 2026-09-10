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

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "lc_undosection.h"
#include "rs_debug.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace {

using BPoint = bg::model::point<double, 2, bg::cs::cartesian>;
using BBox = bg::model::box<BPoint>;

bool isFinitePoint(const RS_Vector& point) {
    return point.valid && std::isfinite(point.x) && std::isfinite(point.y);
}

bool isFiniteBox(const RS_Vector& min, const RS_Vector& max) {
    return isFinitePoint(min) && isFinitePoint(max) && min.x <= max.x && min.y <= max.y
        && std::isfinite(max.x - min.x) && std::isfinite(max.y - min.y);
}

BBox makeBox(const RS_Vector& min, const RS_Vector& max) {
    return {{min.x, min.y}, {max.x, max.y}};
}

bool hasUnboundedSnapGeometry(const RS_Entity& entity) {
    switch (entity.rtti()) {
        case RS2::EntityConstructionLine:
        case RS2::EntityRefConstructionLine:
        case RS2::EntitySnapConstructionLine:
            return true;
        case RS2::EntityLine:
            if (entity.isConstruction()) {
                return true;
            }
            break;
        default:
            break;
    }

    if (!entity.isContainer()) {
        return false;
    }
    for (const RS_Entity* child : static_cast<const RS_EntityContainer&>(entity)) {
        if (child != nullptr && hasUnboundedSnapGeometry(*child)) {
            return true;
        }
    }
    return false;
}

} // namespace

struct RS_Document::SnapIndex {
    struct Entry {
        RS_Entity* entity = nullptr;
        size_t drawOrder = 0;

        bool operator==(const Entry& other) const {
            return entity == other.entity && drawOrder == other.drawOrder;
        }
    };

    using Value = std::pair<BBox, Entry>;
    using Tree = bgi::rtree<Value, bgi::quadratic<16>>;

    Tree tree;
    std::vector<Entry> fallback;

    void rebuild(const RS_Document& document) {
        std::vector<Value> values;
        fallback.clear();

        size_t drawOrder = 0;
        for (RS_Entity* entity : document) {
            Entry entry{entity, drawOrder++};
            if (entity == nullptr || entity->isDeleted()) {
                continue;
            }

            RS_Vector min = entity->getMin();
            RS_Vector max = entity->getMax();
            if (RS2::isTextEntity(entity->rtti())) {
                for (const RS_Vector& ref : entity->getRefPoints()) {
                    if (!isFinitePoint(ref)) {
                        continue;
                    }
                    if (!isFiniteBox(min, max)) {
                        min = ref;
                        max = ref;
                    } else {
                        min.x = std::min(min.x, ref.x);
                        min.y = std::min(min.y, ref.y);
                        max.x = std::max(max.x, ref.x);
                        max.y = std::max(max.y, ref.y);
                    }
                }
            }

            if (!hasUnboundedSnapGeometry(*entity) && isFiniteBox(min, max)) {
                values.emplace_back(makeBox(min, max), entry);
            } else {
                fallback.push_back(entry);
            }
        }

        tree = Tree(values.begin(), values.end());
    }

    QList<RS_Entity*> query(const RS_Vector& coord, const double range) const {
        std::vector<Entry> entries;
        if (isFinitePoint(coord) && std::isfinite(range) && range < RS_MAXDOUBLE / 2.0) {
            const RS_Vector delta{std::max(range, 0.0), std::max(range, 0.0)};
            std::vector<Value> matches;
            tree.query(bgi::intersects(makeBox(coord - delta, coord + delta)), std::back_inserter(matches));
            entries.reserve(matches.size() + fallback.size());
            for (const Value& value : matches) {
                entries.push_back(value.second);
            }
        } else {
            entries.reserve(tree.size() + fallback.size());
            for (auto it = tree.begin(); it != tree.end(); ++it) {
                entries.push_back(it->second);
            }
        }
        entries.insert(entries.end(), fallback.begin(), fallback.end());
        std::sort(entries.begin(), entries.end(), [](const Entry& lhs, const Entry& rhs) {
            return lhs.drawOrder < rhs.drawOrder;
        });

        QList<RS_Entity*> result;
        result.reserve(static_cast<qsizetype>(entries.size()));
        for (const Entry& entry : entries) {
            result.append(entry.entity);
        }
        return result;
    }
};

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

bool RS_Document::undo() {
    const bool result = RS_Undo::undo();
    if (result) {
        invalidateSnapIndex();
    }
    return result;
}

bool RS_Document::redo() {
    const bool result = RS_Undo::redo();
    if (result) {
        invalidateSnapIndex();
    }
    return result;
}

void RS_Document::addEntity(const RS_Entity* entity) {
    invalidateSnapIndex();
    entity->m_parent = this;
    RS_EntityContainer::addEntity(entity);
}

void RS_Document::appendEntity(RS_Entity* entity) {
    invalidateSnapIndex();
    RS_EntityContainer::appendEntity(entity);
}

void RS_Document::prependEntity(RS_Entity* entity) {
    invalidateSnapIndex();
    RS_EntityContainer::prependEntity(entity);
}

void RS_Document::moveEntity(const int index, QList<RS_Entity*>& entList) {
    invalidateSnapIndex();
    RS_EntityContainer::moveEntity(index, entList);
}

void RS_Document::insertEntity(const int index, RS_Entity* entity) {
    invalidateSnapIndex();
    RS_EntityContainer::insertEntity(index, entity);
}

bool RS_Document::removeEntity(RS_Entity* entity) {
    invalidateSnapIndex();
    return RS_EntityContainer::removeEntity(entity);
}

void RS_Document::setEntityAt(const int index, RS_Entity* entity) {
    invalidateSnapIndex();
    RS_EntityContainer::setEntityAt(index, entity);
}

void RS_Document::clear() {
    invalidateSnapIndex();
    RS_EntityContainer::clear();
}

void RS_Document::calculateBorders() {
    RS_EntityContainer::calculateBorders();
    invalidateSnapIndex();
}

QList<RS_Entity*> RS_Document::getSnapCandidates(const RS_Vector& coord, const double range) const {
    if (!m_snapIndex) {
        m_snapIndex = std::make_unique<SnapIndex>();
        m_snapIndex->rebuild(*this);
    }
    return m_snapIndex->query(coord, range);
}

void RS_Document::invalidateSnapIndex() {
    m_snapIndex.reset();
}

/**
 * Overwritten to set modified flag when undo cycle finished with undoable(s).
 */
void RS_Document::endUndoCycle(){
    if (hasUndoable()) {
        setModified(true);
    }
    // keep the selection listeners silent while RS_Undo::endUndoCycle()
    // prunes the obsolete redo cycles (removeEntity() may unselect), then
    // fire the single deferred selection-changed notification
    RS_Undo::endUndoCycle();
    m_selectedSet->enableListeners();
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
    m_savedAutoUpdateBordersBulk = getAutoUpdateBorders();
    setAutoUpdateBorders(false);
}

void RS_Document::endBulkUndoablesCleanup() {
    setAutoUpdateBorders(m_savedAutoUpdateBordersBulk);
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
    for (const auto listener: std::as_const(m_modificationListeners)) {
        if (listener != nullptr) {
            listener->undoStateChanged(this, undoAvailable, redoAvailable);
        }
    }
}
