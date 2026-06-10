/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_visual_snap_manager.h"

#include "lc_graphicviewport.h"
#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_line.h"
#include "lc_ref_snap_mark.h"
#include "lc_relative_point_data.h"
#include "rs_creation.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_preview.h"
#include "lc_property_editor_utils.h"
#include "lc_visual_snap_solution.h"

struct LC_VisualSnapEntityHolder;


LC_VisualSnapManager::LC_VisualSnapManager(RS_Snapper* snapper) : m_solutionVisualizer(snapper, &m_options), m_snapper{snapper}, m_solutionSolver(snapper, &m_options) {
    m_timer.setSingleShot(true);
    m_options.load();
    const auto actionContext = m_snapper->getActionContext();
    const auto graphicView = actionContext->getGraphicView();
    m_snapData = graphicView->getVisualSnapData();
    connect(&m_timer, &QTimer::timeout, this, &LC_VisualSnapManager::performDelayedOperation);
}

LC_VisualSnapManager::~LC_VisualSnapManager() {
    clear();
}

void LC_VisualSnapManager::solveAndVisualizeSolution(RS_Preview* preview, LC_Highlight* highlight) {
    if (m_solution != nullptr) {
        lock();
        auto visualSnapSolution = m_solution.get();
        if (!visualSnapSolution->valid) {
            const RS_Vector wcsPos = visualSnapSolution->wcsPoint;
            solveVisualSnap(wcsPos);
            visualSnapSolution = m_solution.get();
        }
        visualizeSolution(preview, highlight, *visualSnapSolution);
        unlock();
    }
}

LC_VisualSnapSolution* LC_VisualSnapManager::solveVisualSnap(const RS_Vector& wcsPos) {
    const auto solution = new LC_VisualSnapSolution();
    solution->snapData = m_snapData;
    solveVisualSnap(wcsPos, *solution);
    m_solution.reset(solution);
    return solution;
}

void LC_VisualSnapManager::solveVisualSnap(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const {
    m_solutionSolver.solveVisualSnap(wcsPos, solution);
}

void LC_VisualSnapManager::tryProcessVertexDelayed(const RS2::SnapType snapType, const RS_Vector& wcsSnapPoint,
                                                   const RS_Vector& wcsGraphPoint, RS_Entity* entity1,
                                                   [[maybe_unused]] RS_Entity* entity2) {
    if (snapType == RS2::ENDPOINT || snapType == RS2::ENTITY || snapType == RS2::MIDDLE || snapType == RS2::INTERSECTION || snapType ==
        RS2::CENTER || snapType == RS2::DISTANCE || snapType == RS2::VISUAL_SNAP) {
        const double dist = wcsSnapPoint.distanceTo(wcsGraphPoint);
        if (dist < m_wcsSnapRange) {
            addVertexDelayed(snapType, wcsSnapPoint, entity1); // fixme complete and support second entity for intersection (if point is intersection point)?
        }
    }
}

void LC_VisualSnapManager::processEntityDelayed(RS_Entity* entity) {
    doSkipDelayedAdditions();
    m_documentEntityToProcess = entity;
    const int interval = getDocumentEntityAddingDelay();
    m_timer.setInterval(interval);
    m_timer.start();
}

void LC_VisualSnapManager::skipDelayedOperations() {
    lock();
    doSkipDelayedAdditions();
    unlock();
}

void LC_VisualSnapManager::replaceVertexIfAny(LC_VisualSnapVertex* const vertex) {
    removeVertex(vertex);
    storeVertexRef(vertex);
    m_snapper->onVisualSnapPointRegistered(vertex, false);
    m_solution.reset(nullptr);
}

void LC_VisualSnapManager::addSnappedPointAsVertex(const RS_Vector& v, const RS2::SnapType type, RS_Entity* entity, const bool clearOther) {
    if (clearOther) {
        clear();
    }
    lock();
    if (m_options.autoAddGuidesForLastSnapOnly) {
        m_snapData->removeVertexWithLastSnappedData();
    }
    const auto vertex = createVisualSnapVertex(type, v, entity);
    replaceVertexIfAny(vertex);
    doSaveLastSnappedPoint(v);
    unlock();
}

void LC_VisualSnapManager::addGuidesForBasePoint(const RS_Vector& snapPoint, [[maybe_unused]] const RS_Vector& graphPoint,
                                                 const RS_Vector& relZero) {
    lock();

    RS_Vector basePoint;
    if (m_snapData->isLastSnappedPointValid()) {
        basePoint = m_snapData->getLastSnappedPoint();
    }
    else {
        basePoint = relZero;
    }

    // const RS_Vector currentPoint = snapPoint;
    // const auto lastSnapVertex = new LC_VisualSnapVertex(basePoint, currentPoint);
    // registerVertex(lastSnapVertex, false);
    const auto currentPointVertex = new LC_VisualSnapVertex(snapPoint, basePoint);
    registerVertex(currentPointVertex, false);
    doSaveLastSnappedPoint(snapPoint);
    unlock();
}

bool LC_VisualSnapManager::isNotInVisualSnap(RS_Entity* e) const {
    const unsigned long long entityId = e->getId();
    return m_snapData->doesNotContainsDocEntityWithId(entityId);
}

void LC_VisualSnapManager::clear() {
    lock();
    doSkipDelayedAdditions();
    if (m_clearSnapData) {
        m_snapData->clear();
    }
    m_solution.reset(nullptr);
    m_snapper->onVisualSnapEntityRegistered(nullptr);
    unlock();
}

void LC_VisualSnapManager::previewVertex(RS_EntityContainer* preview, const LC_VisualSnapVertex* point, const bool remove) const {
    if (remove) {
        auto entities = preview->getEntityList();
        RS_Entity* entityToRemove = nullptr;
        for (const auto e : entities) {
            if (e->rtti() == RS2::EntitySnapMark) {
                const auto mark = static_cast<LC_RefSnapMark*>(e);
                if (LC_LineMath::isNotMeaningfulDistance(point->wcsSnapCoordinate, mark->getPos())) {
                    entityToRemove = mark;
                    break;
                }
            }
        }
        if (entityToRemove != nullptr) {
            preview->removeEntity(entityToRemove);
        }

        const auto mark = new LC_RefSnapMark(nullptr, point->wcsSnapCoordinate, m_options.vertexSizeHighlighted,
                                             LC_RefSnapMark::HIGHLIGHTED_REMOVED);
        preview->addEntity(mark);
    }
    else {
        const auto mark = new LC_RefSnapMark(nullptr, point->wcsSnapCoordinate, m_options.vertexSizeHighlighted,
                                             LC_RefSnapMark::HIGHLIGHTED);
        preview->addEntity(mark);
    }
}

bool LC_VisualSnapManager::hasVisualSnap(bool ignoreLastSnapData) const {
    lock();
    bool result = !m_snapData->isEmpty();
    if (!result) {
        if (m_solution != nullptr) {
            result = !m_solution->guidingEntities.empty();
        }
    }
    if (!result && !ignoreLastSnapData) {
        result = m_snapData->isLastSnappedPointValid() && (m_options.autoAddSnappedPointToVisualSnap && m_options.autoAddGuidesForLastSnapOnly);
    }
    unlock();
    return result;
}

LC_VisualSnapSolution* LC_VisualSnapManager::getCurrentSolution() const {
    return m_solution.get();
}

void LC_VisualSnapManager::refreshSolutionVisualization(RS_Preview* preview, LC_Highlight* highlight) {
    lock();
    if (m_solution != nullptr) {
        const RS_Vector wcsPoint = m_solution->wcsPoint;
        solveVisualSnap(wcsPoint);
        visualizeSolution(preview, highlight, *m_solution);
    }
    unlock();
}

void LC_VisualSnapManager::visualizeOrdinaryRestrictions(RS_Preview* preview, LC_Highlight* highlight) const {
    m_solutionVisualizer.visualizeOrdinaryRestrictions(preview, highlight);
}

void LC_VisualSnapManager::removeLastAddition() const {
    lock();
    if (m_snapData->removeLastAddition()) {
        invalidateSolution();
    }
    unlock();
}

void LC_VisualSnapManager::setSnapRange(const double range) {
    const double graphSnapRange = m_snapper->toGraphDX(m_options.guidingEntitiesSnapDistance);
    const double snapRangeToUse = std::min(graphSnapRange, range);
    m_wcsSnapRange = snapRangeToUse;
    m_solutionSolver.setSnapRange(snapRangeToUse);
}

void LC_VisualSnapManager::doSaveLastSnappedPoint(const RS_Vector& v) const {
    if (m_solution != nullptr) {
        m_solution->valid = false;
    }
    m_snapData->saveLastSnappedPoint(v);
}

void LC_VisualSnapManager::saveLastSnappedPoint(const RS_Vector& v) const {
    lock();
    doSaveLastSnappedPoint(v);
    unlock();
}

RS_Vector LC_VisualSnapManager::getLastSnappedPoint() const {
    return m_snapData->getLastSnappedPoint();
}

void LC_VisualSnapManager::addRelativePointInfo(const LC_RelativePositionData* relativePositionData) const {
    m_snapData->addRelativePositionInfo(relativePositionData);
    if (m_solution != nullptr) {
        m_solution->valid = false;
    }
}

void LC_VisualSnapManager::addGuidingPoint(const RS_Vector& snapPoint, [[maybe_unused]] const RS_Vector& graphPoint,
                                           const RS_Vector& relZero, const bool hasLength, const bool hasAngle, const bool hasDx,
                                           const bool hasDy, const bool hasNormal) const {
    lock();
    RS_Vector basePoint = snapPoint;

    if (m_snapData->isLastSnappedPointValid()) {
        basePoint = m_snapData->getLastSnappedPoint();
    }
    else {
        basePoint = relZero;
    }

    const RS_Vector currentPoint = snapPoint;

    LC_RelativePositionData data;

    data.wcsBasePoint = basePoint;
    data.wcsProjection = currentPoint;
    data.explicitAngle = hasAngle;
    data.explicitLength = hasLength;
    data.explicitDX = hasDx;
    data.explicitDY = hasDy;
    data.showLengthNormal = hasNormal;
    data.wcsAngle = basePoint.angleTo(currentPoint);
    data.length = basePoint.distanceTo(currentPoint);
    data.valid = true;
    m_snapData->addRelativePositionInfo(&data);
    unlock();
}

void LC_VisualSnapManager::invalidateSolution() const {
    if (m_solution != nullptr) {
        m_solution->valid = false;
    }
}

void LC_VisualSnapManager::lockData(bool performLock) const {
    lock();
    m_snapData->lockContent(performLock);
    unlock();
}


bool LC_VisualSnapManager::isDataLocked() const {
    lock();
    const bool locked = m_snapData->isContentLocked();
    unlock();
    return locked;
}

bool LC_VisualSnapManager::removeVertex(LC_VisualSnapVertex* vertex) const {
    return m_snapData->forEachVertexTillFound([vertex, this](LC_VisualSnapVertex* v)-> bool {
        if (LC_LineMath::isNotMeaningfulDistance(vertex->wcsSnapCoordinate, v->wcsSnapCoordinate) && vertex->entityId == v->entityId) {
            // remove existing if this is requested

            m_snapper->onVisualSnapPointRegistered(vertex, true);
            m_snapData->removeVertex(v);
            invalidateSolution();
            return true;
        }
        return false;
    });
}

void LC_VisualSnapManager::registerVertex(LC_VisualSnapVertex* vertex, const bool removeExistingInSamePosition) {
    // check if vertex exists - remove it if it is so
    if (removeExistingInSamePosition) {
        if (removeVertex(vertex)) {
            delete vertex;
            return;
        }
    }

    // normal adding, vertex does not exists
    storeVertexRef(vertex);
    m_snapper->onVisualSnapPointRegistered(vertex, false);
    m_solution.reset(nullptr);
}

void LC_VisualSnapManager::registerVertex() {
    registerVertex(m_vertexToProcess);
    m_vertexToProcess = nullptr;
}

void LC_VisualSnapManager::registerDocumentEntity() {
    if (isNotInVisualSnap(m_documentEntityToProcess)) {
        // adding entity to visual snap
        const RS2::EntityType type = m_documentEntityToProcess->rtti();
        switch (type) {
            case RS2::EntityLine: {
                const auto docEntity = static_cast<RS_Line*>(m_documentEntityToProcess);
                const auto start = docEntity->getStartpoint();
                const auto end = docEntity->getEndpoint();
                const unsigned long long entityId = docEntity->getId();
                const auto snapEntity = new LC_RefSnapLine(start, end);
                snapEntity->setOriginalId(entityId);
                const auto docViewEntity = new LC_RefSnapLine(start, end);
                docViewEntity->setPen(docEntity->getPen(true));
                docViewEntity->setOriginalId(entityId);
                docViewEntity->setFlag(RS2::FlagInVisualSnap);
                storeEntityRef(snapEntity, docViewEntity, entityId);
                registerEntityEndpoints(docEntity);
                m_solution.reset(nullptr);
                break;
            }
            case RS2::EntityCircle: {
                const auto docEntity = static_cast<RS_Circle*>(m_documentEntityToProcess);
                const auto snapEntity = new LC_RefSnapCircle(docEntity->getCenter(), docEntity->getRadius());
                const auto docViewEntity = new LC_RefSnapCircle(docEntity->getCenter(), docEntity->getRadius());
                docViewEntity->setPen(docEntity->getPen(true));
                const unsigned long long entityId = docEntity->getId();
                docViewEntity->setOriginalId(entityId);
                docViewEntity->setFlag(RS2::FlagInVisualSnap);
                storeEntityRef(snapEntity, docViewEntity, entityId);
                const auto centerVertex = createVisualSnapVertex(RS2::CENTER, docEntity->getCenter(), nullptr);
                centerVertex->entityId = entityId;
                registerVertex(centerVertex, true);
                m_solution.reset(nullptr);
                break;
            }
            case RS2::EntityArc: {
                const auto docEntity = static_cast<RS_Arc*>(m_documentEntityToProcess);
                const unsigned long long entityId = docEntity->getId();
                const auto snapEntity = new LC_RefSnapCircle(docEntity->getCenter(), docEntity->getRadius());
                snapEntity->setOriginalId(entityId);
                const auto docViewEntity = new LC_RefSnapArc(docEntity->getData());
                docViewEntity->setPen(docEntity->getPen(true));
                docViewEntity->setOriginalId(entityId);
                docViewEntity->setFlag(RS2::FlagInVisualSnap);
                storeEntityRef(snapEntity, docViewEntity, entityId);
                registerEntityEndpoints(docEntity);
                const auto centerVertex = createVisualSnapVertex(RS2::CENTER, docEntity->getCenter(), nullptr);
                centerVertex->entityId = entityId;
                registerVertex(centerVertex, true);
                m_solution.reset(nullptr);
                break;
            }
            default:
                // note - so far the list of suported entities is limited by exsitence of their LC_RefSnapXX counterparts (they are
                // used for finding intesections as part of snap resolving, as well as for highlighting visual snap entities on preview.. .
                // yet, potentially? any entity may be added there, if more sophisticated intesection check algorytm will be used...
                break;
        }
    }
    else {
        // remove existing entity
        const unsigned long long entityId = m_documentEntityToProcess->getId();
        m_documentEntityToProcess->delFlag(RS2::FlagInVisualSnap);
        m_snapData->removeDocumentEntityWithId(entityId);
        m_solution.reset(nullptr);
        m_snapper->onVisualSnapEntityRegistered(nullptr);
    }
    m_documentEntityToProcess = nullptr;
}

void LC_VisualSnapManager::addVertexDelayed(const RS2::SnapType snapType, const RS_Vector& coord, RS_Entity* entity) {
    doSkipDelayedAdditions();
    m_vertexToProcess = createVisualSnapVertex(snapType, coord, entity);
    int interval = 0;
    if (snapType == RS2::VISUAL_SNAP) {
        interval = getVisualSnapVertexAddingDelay();
    }
    else {
        interval = getSnapVertexAddingDelay();
    }
    m_timer.setInterval(interval);
    m_timer.start();
}

void LC_VisualSnapManager::updateAndPreviewSolution(RS_Preview* preview, LC_Highlight* highlight, const RS_Vector& wcsPos) {
    lock();
    LC_VisualSnapSolution* solution = solveVisualSnap(wcsPos);
    visualizeSolution(preview, highlight, *solution);
    unlock();
}

void LC_VisualSnapManager::visualizeSolutionForPoint(RS_Preview* preview, LC_Highlight* highlight, const RS_Vector& wcsPos) {
    lock();
    LC_VisualSnapSolution solution;
    solveVisualSnap(wcsPos, solution);
    visualizeSolution(preview, highlight, solution);
    unlock();
}

void LC_VisualSnapManager::visualizeSolution(RS_Preview * preview, LC_Highlight* highlight, LC_VisualSnapSolution& solution) const {
    m_solutionVisualizer.visualizeSolution(preview, highlight, solution);
}

bool LC_VisualSnapManager::isLineIsNotHorizontalOrVerticalInUCS(const RS_Vector& startPoint, const RS_Vector& endPoint) const {
    const RS_Vector ucsStart = m_viewport->toUCS(startPoint);
    const RS_Vector ucsEnd = m_viewport->toUCS(endPoint);
    const RS_Vector ucsDelta = ucsStart - ucsEnd;

    const bool horizontal = LC_LineMath::isNotMeaningful(ucsDelta.y);
    const bool vertical = LC_LineMath::isNotMeaningful(ucsDelta.x);
    return !horizontal && !vertical;
}

bool LC_VisualSnapManager::isLineIsNotHorizontalOrVerticalInUCS(const RS_Vector& startPoint, const RS_Vector& endPoint, bool& horizontal,
                                                                bool& vertical) const {
    const RS_Vector ucsStart = m_viewport->toUCS(startPoint);
    const RS_Vector ucsEnd = m_viewport->toUCS(endPoint);
    const RS_Vector ucsDelta = ucsStart - ucsEnd;

    horizontal = LC_LineMath::isNotMeaningful(ucsDelta.y);
    vertical = LC_LineMath::isNotMeaningful(ucsDelta.x);
    return !horizontal && !vertical;
}

void LC_VisualSnapManager::performDelayedOperation() {
    lock();
    if (m_vertexToProcess != nullptr) {
        registerVertex();
    }
    if (m_documentEntityToProcess != nullptr) {
        registerDocumentEntity();
    }
    unlock();
}

void LC_VisualSnapManager::doSkipDelayedAdditions() {
    if (m_vertexToProcess != nullptr || m_documentEntityToProcess != nullptr) {
        m_timer.stop();
        delete m_vertexToProcess;
        m_documentEntityToProcess = nullptr;
        m_vertexToProcess = nullptr;
    }
}

void LC_VisualSnapManager::registerEntityEndpoints(RS_Entity* const entity) {
    const auto startVertex = createVisualSnapVertex(RS2::ENDPOINT, entity->getStartpoint(), entity);
    const auto endVertex = createVisualSnapVertex(RS2::ENDPOINT, entity->getEndpoint(), entity);
    replaceVertexIfAny(startVertex);
    replaceVertexIfAny(endVertex);
}

void LC_VisualSnapManager::storeEntityRef(RS_Entity* const snapEntity, RS_Entity* documentViewSnapEntity,  unsigned long long entityId) const {
    m_snapData->storeEntityRef(snapEntity, documentViewSnapEntity, entityId);
    invalidateSolution();
    m_snapper->onVisualSnapEntityRegistered(snapEntity->clone());
}

void LC_VisualSnapManager::storeVertexRef(LC_VisualSnapVertex* const vertex) const {
    m_snapData->storeVertexRef(vertex);
    invalidateSolution();
}

LC_VisualSnapVertex* LC_VisualSnapManager::createVisualSnapVertex(const RS2::SnapType snap, const RS_Vector& coord, RS_Entity* entity) {
    return new LC_VisualSnapVertex(snap, coord, entity);
}

int LC_VisualSnapManager::getVisualSnapVertexAddingDelay() const {
    return m_options.delayMsProjectedSnap;
}

int LC_VisualSnapManager::getSnapVertexAddingDelay() const {
    return m_options.delayMsSnapVertex;
}

int LC_VisualSnapManager::getDocumentEntityAddingDelay() const {
    return m_options.delayMsDocumentEntity;
}
