/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 * Licensed under GPL-2.0-or-later.
 */
#include "lc_curvesplit.h"

#include <algorithm>
#include <cmath>

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_math.h"

namespace lc::curvesplit {
namespace {
double orientedProgress(const double start, const double angle, const bool reversed) {
    return RS_Math::correctAngle(reversed ? start - angle : angle - start);
}

}

bool isSupported(const RS_AtomicEntity& entity) {
    switch (entity.rtti()) {
    case RS2::EntityLine:
    case RS2::EntityArc:
    case RS2::EntityCircle:
    case RS2::EntityEllipse:
        return true;
    default:
        return false;
    }
}

bool isClosed(const RS_AtomicEntity& entity) {
    if (entity.rtti() == RS2::EntityCircle) {
        return true;
    }
    if (entity.rtti() == RS2::EntityEllipse) {
        return !static_cast<const RS_Ellipse&>(entity).isEllipticArc();
    }
    return false;
}

double paramOf(const RS_AtomicEntity& entity, const RS_Vector& pointOnEntity) {
    switch (entity.rtti()) {
    case RS2::EntityLine: {
        const auto& line = static_cast<const RS_Line&>(entity);
        const RS_Vector direction = line.getEndpoint() - line.getStartpoint();
        const double squaredLength = RS_Vector::dotP(direction, direction);
        return squaredLength > RS_TOLERANCE2
                   ? RS_Vector::dotP(pointOnEntity - line.getStartpoint(), direction) / squaredLength
                   : 0.0;
    }
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(entity);
        return orientedProgress(arc.getAngle1(), (pointOnEntity - arc.getCenter()).angle(), arc.isReversed());
    }
    case RS2::EntityCircle:
        return RS_Math::correctAngle((pointOnEntity - entity.getCenter()).angle());
    case RS2::EntityEllipse: {
        const auto& ellipse = static_cast<const RS_Ellipse&>(entity);
        if (!ellipse.isEllipticArc()) {
            return RS_Math::correctAngle(ellipse.getEllipseAngle(pointOnEntity));
        }
        return orientedProgress(ellipse.getAngle1(), ellipse.getEllipseAngle(pointOnEntity), ellipse.isReversed());
    }
    default:
        return 0.0;
    }
}

RS_Vector tangentAt(const RS_AtomicEntity& entity, const RS_Vector& pointOnEntity) {
    if (entity.rtti() == RS2::EntityLine) {
        const auto& line = static_cast<const RS_Line&>(entity);
        return line.getEndpoint() - line.getStartpoint();
    }
    if (entity.rtti() == RS2::EntityArc || entity.rtti() == RS2::EntityCircle) {
        const RS_Vector radial = pointOnEntity - entity.getCenter();
        const bool reversed = entity.rtti() == RS2::EntityArc && static_cast<const RS_Arc&>(entity).isReversed();
        return reversed ? RS_Vector{radial.y, -radial.x} : RS_Vector{-radial.y, radial.x};
    }
    if (entity.rtti() == RS2::EntityEllipse) {
        const auto& ellipse = static_cast<const RS_Ellipse&>(entity);
        const double angle = ellipse.getEllipseAngle(pointOnEntity);
        const RS_Vector major = ellipse.getMajorP();
        const RS_Vector minor{-major.y * ellipse.getRatio(), major.x * ellipse.getRatio()};
        RS_Vector tangent = major * -std::sin(angle) + minor * std::cos(angle);
        if (ellipse.isReversed()) {
            tangent = tangent * -1.0;
        }
        return tangent;
    }
    return {};
}

std::unique_ptr<RS_AtomicEntity> subCurve(const RS_AtomicEntity& entity,
                                         [[maybe_unused]] const double tA,
                                         const RS_Vector& pA,
                                         [[maybe_unused]] const double tB,
                                         const RS_Vector& pB) {
    switch (entity.rtti()) {
    case RS2::EntityLine:
        return std::make_unique<RS_Line>(pA, pB);
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(entity);
        const bool reversed = arc.isReversed();
        const double a1 = (pA - arc.getCenter()).angle();
        const double a2 = (pB - arc.getCenter()).angle();
        return std::make_unique<RS_Arc>(RS_ArcData{arc.getCenter(), arc.getRadius(), a1, a2, reversed});
    }
    case RS2::EntityCircle: {
        const auto& circle = static_cast<const RS_Circle&>(entity);
        const double a1 = (pA - circle.getCenter()).angle();
        const double a2 = (pB - circle.getCenter()).angle();
        return std::make_unique<RS_Arc>(RS_ArcData{circle.getCenter(), circle.getRadius(), a1, a2, false});
    }
    case RS2::EntityEllipse: {
        const auto& ellipse = static_cast<const RS_Ellipse&>(entity);
        RS_EllipseData data = ellipse.getData();
        data.setAngle1(ellipse.getEllipseAngle(pA));
        data.setAngle2(ellipse.getEllipseAngle(pB));
        return std::make_unique<RS_Ellipse>(nullptr, data);
    }
    default:
        return nullptr;
    }
}
}
