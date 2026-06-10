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

#ifndef LC_VISUALSNAPMANAGER_H
#define LC_VISUALSNAPMANAGER_H

#include <QTimer>

#include "lc_relative_point_data.h"
#include "lc_visual_snap_data.h"
#include "lc_visual_snap_options.h"
#include "lc_visual_snap_solution_solver.h"
#include "lc_visual_snap_solution_visualizer.h"
#include "rs_line.h"


class LC_CoordinatesMapper;
class RS_Snapper;
class RS_Preview;
class LC_Highlight;

class LC_VisualSnapManager : public QObject {
    Q_OBJECT public:
    explicit LC_VisualSnapManager(RS_Snapper* snapper);
    ~LC_VisualSnapManager() override;

    void tryProcessVertexDelayed(RS2::SnapType snapType, const RS_Vector& wcsSnapPoint, const RS_Vector& wcsGraphPoint, RS_Entity* entity1,
                                 RS_Entity* entity2);
    void processEntityDelayed(RS_Entity* entity);
    void skipDelayedOperations();
    void replaceVertexIfAny(LC_VisualSnapVertex* vertex);

    void addSnappedPointAsVertex(const RS_Vector& v, RS2::SnapType type, RS_Entity* entity, bool clearOther = false);
    void addGuidesForBasePoint(const RS_Vector& snapPoint, const RS_Vector& graphPoint, const RS_Vector& relZero);
    bool isNotInVisualSnap(RS_Entity* e) const;
    void clear();

    void invalidateSolution() const;

    void previewVertex(RS_EntityContainer* preview, const LC_VisualSnapVertex* point, bool remove) const;
    bool hasVisualSnap(bool ignoreLastSnapData = false) const;

    void setViewport(LC_GraphicViewport* viewport) {
        m_viewport = viewport;
        m_solutionSolver.setViewport(viewport);
        m_solutionVisualizer.setViewport(viewport);
    }

    void setClearSnapData(bool enable){m_clearSnapData = enable;}

    LC_VisualSnapSolution* solveVisualSnap(const RS_Vector& wcsPos);
    LC_VisualSnapSolution* getCurrentSolution() const;
    void solveAndVisualizeSolution(RS_Preview* preview, LC_Highlight *highlight);
    void refreshSolutionVisualization(RS_Preview* preview, LC_Highlight* highlight);
    void visualizeOrdinaryRestrictions(RS_Preview* preview, LC_Highlight* highlight) const;
    void removeLastAddition() const;

    void setSnapRange(double range);
    void doSaveLastSnappedPoint(const RS_Vector& v) const;

    void updateOptions() {
        m_options.load();
    }

    bool isManualVertexAddWithCTRL() const {
        return m_options.manualVertexAddingRequiresCTRL;
    }

    bool isAutoAddSnappedPoint() const {
        return m_options.autoAddSnappedPointToVisualSnap;
    }

    void saveLastSnappedPoint(const RS_Vector& v) const;
    RS_Vector getLastSnappedPoint()const;
    void addRelativePointInfo(const LC_RelativePositionData* relativePositionData) const;
    void addGuidingPoint(const RS_Vector& snapPoint, const RS_Vector& graphPoint, const RS_Vector& relZero, bool hasLength, bool hasAngle,
                         bool hasDx, bool hasDy, bool hasNormal) const;

    void updateAndPreviewSolution(RS_Preview* preview, LC_Highlight* highlight, const RS_Vector& wcsPos);
    bool isClearVisualSnapByRMB() const {return m_options.allowClearingVisualSnapByRMB;}
    void lockData(bool performLock) const;
    bool isDataLocked() const;
protected:
    bool removeVertex(LC_VisualSnapVertex* vertex) const;
    void registerVertex(LC_VisualSnapVertex* vertex, bool removeExistingInSamePosition = true);
    void registerVertex();
    void registerDocumentEntity();
    void addVertexDelayed(RS2::SnapType snapType, const RS_Vector& coord, RS_Entity* entity);
    void solveVisualSnap(const RS_Vector& wcsPos, LC_VisualSnapSolution& solution) const;
    void visualizeSolution(RS_Preview* preview, LC_Highlight* highlight, LC_VisualSnapSolution& solution) const;

    bool isLineIsNotHorizontalOrVerticalInUCS(const RS_Vector& startPoint, const RS_Vector& endPoint) const;
    bool isLineIsNotHorizontalOrVerticalInUCS(const RS_Vector& startPoint, const RS_Vector& endPoint, bool& horizontal, bool& vertical) const;
    void performDelayedOperation();
    void doSkipDelayedAdditions();
    void registerEntityEndpoints(RS_Entity* entity);
    void storeEntityRef(RS_Entity* snapEntity, RS_Entity* documentViewSnapEntity,  unsigned long long entityId) const;
    void storeVertexRef(LC_VisualSnapVertex* vertex) const;
    LC_VisualSnapVertex* createVisualSnapVertex(RS2::SnapType snap, const RS_Vector& coord, RS_Entity* entity);
    int getVisualSnapVertexAddingDelay() const;
    int getSnapVertexAddingDelay() const;
    int getDocumentEntityAddingDelay() const;
    void visualizeSolutionForPoint(RS_Preview* preview, LC_Highlight* highlight, const RS_Vector& wcsPos);

    void lock() const {m_snapData->lock();}
    void unlock() const {m_snapData->unlock();}

    LC_VisualSnapOptions m_options;
    LC_VisualSnapData* m_snapData {nullptr};
    std::unique_ptr<LC_VisualSnapSolution> m_solution;
    bool m_clearSnapData {true};

    LC_VisualSnapSolutionVisualizer m_solutionVisualizer;
    RS_Entity* m_documentEntityToProcess{nullptr};
    QTimer m_timer;
    LC_GraphicViewport* m_viewport = nullptr;
    RS_Snapper* m_snapper{nullptr};
    double m_wcsSnapRange{0.0};
    LC_VisualSnapVertex* m_vertexToProcess{nullptr};
    LC_VisualSnapSolutionSolver m_solutionSolver;
};

#endif
