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

#ifndef LC_VISUALSNAPSOLUTION_H
#define LC_VISUALSNAPSOLUTION_H

#include "lc_ref_snap_arc.h"
#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"

class LC_VisualSnapData;

struct LC_VisualSnapVertex {

    RS2::SnapType snapType;
    RS_Vector wcsSnapCoordinate{false};

    LC_RefSnapConstructionLine* refLine{nullptr}; // fixme - to unique ptr
    LC_RefSnapArc* refArc{nullptr};
    LC_RefSnapCircle* refCircle{nullptr};

    // temporary flags for displaying
    bool flagProcessed{false};
    bool flagHighlighted{false};

    unsigned long long  entityId = 0ULL;

    LC_VisualSnapVertex(RS2::SnapType snapType, const RS_Vector& wcsSnapCoordinate, RS_Entity* entity)
        : snapType(snapType), wcsSnapCoordinate(wcsSnapCoordinate) {
        if (entity != nullptr) {
            entityId = entity->getId();
            if (entity->is(RS2::EntityLine)) {
                const auto line = static_cast<RS_Line*>(entity);
                refLine = new LC_RefSnapConstructionLine(line->getStartpoint(), line->getEndpoint());
            }
            else if (entity->is(RS2::EntityArc)) {
                const auto arc = static_cast<RS_Arc*>(entity);
                refArc = new LC_RefSnapArc(arc->getData());
            }
            else if (entity->is(RS2::EntityCircle)) {
                const auto circle = static_cast<RS_Circle*>(entity);
                refCircle = new LC_RefSnapCircle(circle->getCenter(), circle->getRadius());
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
    }
};

struct LC_VisualSnapEntityHolder {
    bool processed{false};
    RS_Entity* entity {nullptr};
    RS2::VisualSnapGuideEntityType snapEntityType = RS2::VSNAP_NONE;
    double wcsRayAngleRad {RS_MINDOUBLE};

    LC_VisualSnapEntityHolder() = default;

    LC_VisualSnapEntityHolder(RS_Entity* ent, RS2::VisualSnapGuideEntityType snapType) : entity{ent}, snapEntityType{snapType} {
    }

    LC_VisualSnapEntityHolder(RS_Entity* ent, RS2::VisualSnapGuideEntityType snapType, double rayAngle) : entity{ent}, snapEntityType{snapType}, wcsRayAngleRad{rayAngle} {
    }
};

struct LC_VisualSnapPointHolder {
    RS_Vector pos;
    RS2::VisualSnapGuideEntityType snapEntityType;

    LC_VisualSnapPointHolder(const RS_Vector& pos, RS2::VisualSnapGuideEntityType snapEntityType)
        : pos(pos), snapEntityType(snapEntityType) {
    }
};

struct LC_VisualSnapDocumentEntityRef {
    std::unique_ptr<RS_Entity> guidingEntity{nullptr};
    std::unique_ptr<RS_Entity> documentViewEntity{nullptr};
    bool processed = false;
    unsigned long long originalEntityId = 0ULL;

    LC_VisualSnapDocumentEntityRef(RS_Entity* ent, RS_Entity* docEntity, unsigned long long id)
        : documentViewEntity{docEntity}, originalEntityId{id} {
        guidingEntity.reset(ent);
    }

    ~LC_VisualSnapDocumentEntityRef() {
        guidingEntity.reset(nullptr);
        documentViewEntity.reset(nullptr);
    }
};

struct LC_VisualSnapItem {
    bool isVertexItem = true;
    std::unique_ptr<LC_VisualSnapVertex> vertex{nullptr};
    std::unique_ptr<LC_VisualSnapDocumentEntityRef> docEntityRef{nullptr};

    LC_VisualSnapItem(RS_Entity* snapEntity, RS_Entity* docEntity, unsigned long long id) {
        isVertexItem = false;
        docEntityRef.reset(new LC_VisualSnapDocumentEntityRef(snapEntity, docEntity, id));
    }

    explicit LC_VisualSnapItem(LC_VisualSnapVertex* v) {
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
        return docEntityRef->guidingEntity.get();
    }
};

struct LC_VisualSnapSolution {
    std::list<LC_VisualSnapEntityHolder> guidingEntities;
    RS2::LC_VisualSnapIntersectionInfo foundSnapPointInfo; // fixme - rename class
    RS_Vector foundSnapPoint{false};
    RS_Vector wcsPoint{false};
    RS_Vector restrictedPoint{false};
    RS_Vector previousSnapPoint{false};
    RS2::SnapType restrictedOriginalSnapType{RS2::SnapType::GRID};
    std::list<RS_Vector> snapCandidatesToShow;
    std::list<RS_Vector> middlePoints;
    bool valid = true;
    LC_VisualSnapData* snapData {nullptr};

    ~LC_VisualSnapSolution() {
        for (const auto& h:guidingEntities) {
            delete h.entity;
        }
    }

    bool hasFoundSnapPoint() const {
        return foundSnapPoint.valid;
    }

    void setFoundSnapPoint(RS_Vector& snapPoint, const RS2::LC_VisualSnapIntersectionInfo& pointInfo) {
        foundSnapPoint = snapPoint;
        foundSnapPointInfo = pointInfo;
    }

    void addGuidingEntity(LC_RefSnapLine* line, double wcsRayAngle = RS_MINDOUBLE) {
        const RS2::VisualSnapGuideEntityType guideType = line->getGuideType();
        const LC_VisualSnapEntityHolder holder(line, guideType, wcsRayAngle);
        guidingEntities.push_back(holder);
    }

    void addGuidingEntity(LC_RefSnapConstructionLine* line, double wcsRayAngle = RS_MINDOUBLE) {
        const RS2::VisualSnapGuideEntityType guideType = line->getGuideType();
        const LC_VisualSnapEntityHolder holder(line, guideType, wcsRayAngle);
        guidingEntities.push_back(holder);
    }

    void addGuidingEntity(RS_Entity* e, RS2::VisualSnapGuideEntityType guideType) {
        const LC_VisualSnapEntityHolder holder(e, guideType);
        guidingEntities.push_back(holder);
    }
    void addGuidingEntity(LC_RefSnapCircle* circle) {
        const RS2::VisualSnapGuideEntityType guideType = circle->getGuideType();
        const LC_VisualSnapEntityHolder holder(circle, guideType);
        guidingEntities.push_back(holder);
    }

    void addSnapCandidate(const RS_Vector& v) {
        snapCandidatesToShow.push_back(v);
    }
};



#endif
