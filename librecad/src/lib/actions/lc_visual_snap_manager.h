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

#include <QMutex>
#include <QTimer>
#include <boost/geometry/algorithms/centroid.hpp>

#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_line.h"
#include "rs_arc.h"
#include "rs_line.h"

class LC_CoordinatesMapper;
class RS_Snapper;

struct LC_VisualSnapVertex {
    RS2::SnapType snapType;
    RS_Vector wcsSnapCoordinate{false};
    LC_RefSnapConstructionLine* refLine{nullptr};
    RS_Arc* refArc{nullptr};
    RS_Circle* refCircle{nullptr};
    RS_Vector ucsSnapCoord{false};

    // temporary flags for displaying
    bool flagProcessed{false};
    bool flagHighlighted{false};

    LC_VisualSnapVertex(RS2::SnapType snapType, const RS_Vector& wcsSnapCoordinate, RS_Entity* entity)
        : snapType(snapType), wcsSnapCoordinate(wcsSnapCoordinate) {
        if (entity != nullptr) {
            if (entity->is(RS2::EntityLine)) {
                const auto line = static_cast<RS_Line*>(entity);
                refLine = new LC_RefSnapConstructionLine(line->getStartpoint(), line->getEndpoint());
            }
            else if (entity->is(RS2::EntityArc)) {
                const auto arc = static_cast<RS_Arc*>(entity);
                refArc = new RS_Arc(nullptr, arc->getData());
            }
            else if (entity->is(RS2::EntityCircle)) {
                const auto circle = static_cast<RS_Circle*>(entity);
                refCircle = new RS_Circle(nullptr, circle->getData());
            }
        }
    }

    LC_VisualSnapVertex(const RS_Vector& start, const RS_Vector& end) : snapType{RS2::SnapType::FREE}, wcsSnapCoordinate(start){
        refLine = new LC_RefSnapConstructionLine(start, end);
        refCircle = new LC_RefSnapCircle(start, start.distanceTo(end));
    }

    ~LC_VisualSnapVertex() {
        delete refLine;
        delete refArc;
        delete refCircle;
    };
};

struct EntityHolder {
    bool processed{false};
    RS_Entity* entity;
    RS2::VisualSnapGuideEntityType snapEntityType;
    double wcsRayAngleRad {-1.0};

    EntityHolder(RS_Entity* ent, RS2::VisualSnapGuideEntityType snapType) : entity{ent}, snapEntityType{snapType} {
    }

    EntityHolder(RS_Entity* ent, RS2::VisualSnapGuideEntityType snapType, double rayAngle) : entity{ent}, snapEntityType{snapType}, wcsRayAngleRad{rayAngle} {
    }
};

struct PointHolder {
    RS_Vector pos;
    RS2::VisualSnapGuideEntityType snapEntityType;

    PointHolder(const RS_Vector& pos, RS2::VisualSnapGuideEntityType snapEntityType)
        : pos(pos), snapEntityType(snapEntityType) {
    }
};

struct VisualSnapSolution {
    std::list<EntityHolder> guideEntities;
    RS2::LC_VisualSnapEntersectionPoint foundSnapPointInfo;
    RS_Vector foundSnapPoint{false};
    RS_Vector wcsPoint{false};
    RS_Vector restrictedPoint{false};
    RS2::SnapType restrictedOriginalSnapType{RS2::SnapType::GRID};
    std::list<RS_Vector> snapCandidatesToShow;
    std::list<RS_Vector> middlePoints;
    bool valid = true;

    bool hasFoundSnapPoint() const {
        return foundSnapPoint.valid;
    }

    void setFoundSnapPoint(RS_Vector& snapPoint, const RS2::LC_VisualSnapEntersectionPoint pointInfo) {
        foundSnapPoint = snapPoint;
        foundSnapPointInfo = pointInfo;
    }

    void addGuideEntity(RS_Entity* entity, const RS2::VisualSnapGuideEntityType entityType) {
        const EntityHolder holder(entity, entityType);
        guideEntities.push_back(holder);
    }

    void addGuideEntity(RS_Entity* entity, const RS2::VisualSnapGuideEntityType entityType, double wcsRayAngle) {
        const EntityHolder holder(entity, entityType, wcsRayAngle);
        guideEntities.push_back(holder);
    }

    void addSnapCandidate(const RS_Vector& v) {
        snapCandidatesToShow.push_back(v);
    }
};

struct VisualSnapOptions {
    int delayMsSnapVertex = 300;
    int delayMsProjectedSnap = 1200;
    int delayMsDocumentEntity = 1500;

    int vertexSizeNormal = 6;
    int vertexSizeProjected = 4;
    int vertexSizeHighlighted = 10;

    bool createAngleStepRaysForVertexes = true;
    bool createAngleStepRaysForEntitiesEndpoints = true;
    bool createVertexVertexDistanceCircles = true;

    bool autoAddSnappedPointToVisualSnap = true;
    bool manualVertexAddingRequiresCTRL = false;
    int guidingEntitiesSnapDistance = 24;

    void load();
};

class LC_VisualSnapManager : public QObject {
    Q_OBJECT public:
    explicit LC_VisualSnapManager(RS_Snapper* snapper);
    ~LC_VisualSnapManager() override;

    void tryProcessVertexDelayed(RS2::SnapType snapType, const RS_Vector& wcsSnapPoint, const RS_Vector& wcsGraphPoint, RS_Entity* entity1,
                                 RS_Entity* entity2);
    void processEntityDelayed(RS_Entity* entity);
    void skipDelayedOperations();

    void addSnappedPointAsVertex(const RS_Vector& v, RS2::SnapType type, RS_Entity* entity, bool clearOther = false);
    void addGuidesForBasePoint(const RS_Vector& snapPoint, const RS_Vector& graphPoint, const RS_Vector& relZero);
    bool isNotInVisualSnap(RS_Entity* e) const;
    void clear();

    void previewVertex(RS_EntityContainer* preview, const LC_VisualSnapVertex* point, bool remove) const;
    bool hasVertexesOrEntities();

    void setViewport(LC_GraphicViewport* viewport) {
        m_viewport = viewport;
    }

    VisualSnapSolution* solveVisualSnap(const RS_Vector& wcsPos);
    VisualSnapSolution* getCurrentSolution() const;
    void previewSolution(RS_EntityContainer* preview);
    void refreshSolutionPreview(RS_EntityContainer* preview);

    void previewOrdinaryRestrictions(RS_EntityContainer* preview) const;
    void removeLastAddition();

    void setSnapRange(double range);

    void updateOptions() {
        m_options.load();
    }

    bool isManualVertexAddWithCTRL() const {
        return m_options.manualVertexAddingRequiresCTRL;
    }

    bool isAutoAddSnappedPoint() const {
        return m_options.autoAddSnappedPointToVisualSnap;
    }


    void saveLastSnappedPoint(const RS_Vector& v);

protected:
    void invalidateSolution();
    void registerVertex(LC_VisualSnapVertex* vertex, bool removeExistingInSamePosition = true);
    void registerVertex();
    void registerDocumentEntity();
    void addVertexDelayed(RS2::SnapType snapType, const RS_Vector& coord, RS_Entity* entity);
    void createGuideEntitiesByDocumentEntities(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    void solveVisualSnap(const RS_Vector& wcsPos, VisualSnapSolution& solut);
    void previewVisualSnapForPoint(RS_EntityContainer* preview, const RS_Vector& wcsPos);
    void previewVisualSnapSolution(RS_EntityContainer* preview, VisualSnapSolution& solution) const;
    void createOrthoRaysForVertexes(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    bool isLineIsNotHorizontalOrVerticalInUCS(RS_Vector startPoint, RS_Vector endPoint) const;
    void createLineRays(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    void createRelativeRays(const RS_Vector& wcsPos, const RS_Vector& startPoint, const RS_Vector& endPoint,
                            VisualSnapSolution& solution) const;
    void createDistanceCircle2Points(const RS_Vector& wcsPos, const RS_Vector& centerPoint, const RS_Vector& circlePoint,
                                     VisualSnapSolution& solution) const;
    void createVertexToVertexLinesAndDistances(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const;
    void createTangentialAndNormalRaysFromArcEndpoints(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    void createTangentialRaysFromVertexesToArcs(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const;
    void createTangentialRaysBetwenCirclesArcs(const RS_Vector& wcsPos, std::vector<PointHolder>& specialPointSnapCandidates, VisualSnapSolution& solution) const;

    void createNormalsFromVertexToEntities(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    LC_RefSnapConstructionLine* tryGuideConstructionLine(const RS_Vector& wcsPos, const RS_Vector& start, const RS_Vector& end,
                                                         double& dist) const;
    void createExplicitlySetDistanceCirclesForVertexes(const RS_Vector& wcsPos, VisualSnapSolution& solution) const;
    void findSnapPoint(const RS_Vector& wcsPos, VisualSnapSolution& solution, const std::vector<PointHolder>& middlePointSnapCandidates) const;
    void clearVertexProcessedFlag() const;
    void performDelayedOperation();
    void doSkipDelayedAdditions();
    void registerEntityEndpoints(RS_Entity* entity);
    void storeEntityRef(RS_Entity* snapEntity, RS_Entity* documentEntity);
    void storeVertexRef(LC_VisualSnapVertex* vertex);
    LC_VisualSnapVertex* createVisualSnapVertex(RS2::SnapType snap, const RS_Vector& coord, RS_Entity* entity);
    int getVisualSnapVertexAddingDelay() const;
    int getSnapVertexAddingDelay() const;
    int getDocumentEntityAddingDelay() const;

    struct DocumentEntityRef {
        std::unique_ptr<RS_Entity> guideEntity;
        RS_Entity* documentEntity{nullptr};

        DocumentEntityRef(RS_Entity* ent, RS_Entity* docEntity)
            : documentEntity{docEntity} {
            guideEntity.reset(ent);
        }

        ~DocumentEntityRef() {
            guideEntity.reset(nullptr);
        }
    };

    struct VisualSnapItem {
        bool isVertexItem = true;
        std::unique_ptr<LC_VisualSnapVertex> vertex{nullptr};
        std::unique_ptr<DocumentEntityRef> entityRef{nullptr};

        VisualSnapItem(RS_Entity* snapEntity, RS_Entity* docEntity) {
            isVertexItem = false;
            entityRef.reset(new DocumentEntityRef(snapEntity, docEntity));
        }

        VisualSnapItem(LC_VisualSnapVertex* v) {
            isVertexItem = true;
            vertex.reset(v);
        }

        LC_VisualSnapVertex* getVertex() const {
            if (isVertexItem) {
                return vertex.get();
            }
            return nullptr;
        }

        RS_Entity* getGuideDocEntity() const {
            if (isVertexItem) {
                return nullptr;
            }
            return entityRef->guideEntity.get();
        }
    };

    VisualSnapOptions m_options;
    std::unique_ptr<VisualSnapSolution> m_solution;
    std::list<std::unique_ptr<VisualSnapItem>> m_itemsList;
    LC_VisualSnapVertex* m_vertexToProcess{nullptr};
    RS_Entity* m_documentEntityToProcess{nullptr};
    RS_Vector m_lastSnappedBasePoint{false};
    QTimer m_timer;
    LC_GraphicViewport* m_viewport = nullptr;
    RS_Snapper* m_snapper{nullptr};
    double m_wcsSnapRange{0.0};
    QMutex m_mutex;
};

#endif
