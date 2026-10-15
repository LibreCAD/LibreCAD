// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#include "lc_polygonselection.h"

#include <algorithm>
#include <memory>
#include "rs_entitycontainer.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_image.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_math.h"

namespace {
RS_Vector vector(const QPointF& point) { return {point.x(), point.y()}; }
QPointF point(const RS_Vector& value) { return {value.x, value.y}; }

struct Region {
    const QPolygonF& polygon;
    const QRectF bounds;
    std::vector<std::unique_ptr<RS_Line>> edges;
    explicit Region(const QPolygonF& boundary) : polygon(boundary), bounds(boundary.boundingRect().normalized()) {
        edges.reserve(boundary.size());
        for (int i = 0; i < boundary.size(); ++i)
            edges.push_back(std::make_unique<RS_Line>(nullptr, vector(boundary[i]), vector(boundary[(i + 1) % boundary.size()])));
    }
};

bool inside(const Region& region, const RS_Vector& value) {
    const auto& polygon = region.polygon;
    if (!region.bounds.adjusted(-RS_TOLERANCE, -RS_TOLERANCE, RS_TOLERANCE, RS_TOLERANCE).contains(point(value))) return false;
    if (!value.valid) return false;
    if (polygon.containsPoint(point(value), Qt::OddEvenFill)) return true;
    for (const auto& edge : region.edges) {
        double distance = 0;
        edge->getNearestPointOnEntity(value, true, &distance);
        if (distance <= RS_TOLERANCE) return true;
    }
    return false;
}

bool matches(const RS_Entity* entity, const Region& region, bool crossing) {
    const auto& polygon = region.polygon;
    if (!entity->isVisible() || entity->isLocked()) return false;
    const auto& bounds = region.bounds;
    const auto min = entity->getMin(), max = entity->getMax();
    if (max.x < bounds.left() || min.x > bounds.right() || max.y < bounds.top() || min.y > bounds.bottom()) return false;
    if (entity->isContainer() && static_cast<const RS_EntityContainer*>(entity)->count() > 0) {
        bool any = false;
        for (const auto* child : *static_cast<const RS_EntityContainer*>(entity)) {
            if (!child->isVisible()) continue;
            const bool matched = matches(child, region, crossing);
            if (crossing && matched) return true;
            if (!crossing && !matched) return false;
            any = true;
        }
        return !crossing && any;
    }

    // Lines are partitioned at boundary intersections. Testing each interval
    // handles concave lassos and endpoints lying on the boundary exactly.
    QList<double> cuts{0.0, 1.0};
    const auto start = entity->getStartpoint(), end = entity->getEndpoint();
    const auto delta = end - start;
    const double squaredLength = delta.squared();
    bool intersects = false;
    std::function<RS_Vector(double)> curvePoint;
    std::function<double(const RS_Vector&)> curveParameter;
    double span = 2 * M_PI;
    if (entity->rtti() == RS2::EntityCircle || entity->rtti() == RS2::EntityArc) {
        const auto center = entity->getCenter();
        double startAngle = 0, direction = 1, radius;
        if (entity->rtti() == RS2::EntityArc) {
            const auto* arc = static_cast<const RS_Arc*>(entity);
            startAngle = arc->getAngle1();
            direction = arc->isReversed() ? -1 : 1;
            span = arc->getAngleLength();
            radius = arc->getRadius();
        } else radius = static_cast<const RS_Circle*>(entity)->getRadius();
        curvePoint = [=](double parameter) { return center + RS_Vector::polar(radius, startAngle + direction * parameter); };
        curveParameter = [=](const RS_Vector& hit) { return RS_Math::correctAngle(direction * (center.angleTo(hit) - startAngle)); };
    } else if (entity->rtti() == RS2::EntityEllipse) {
        const auto* ellipse = static_cast<const RS_Ellipse*>(entity);
        const double startAngle = ellipse->isEllipticArc() ? ellipse->getAngle1() : 0;
        const double direction = ellipse->isReversed() ? -1 : 1;
        span = ellipse->getAngleLength();
        curvePoint = [=](double parameter) { return ellipse->getEllipsePoint(startAngle + direction * parameter); };
        curveParameter = [=](const RS_Vector& hit) { return RS_Math::correctAngle(direction * (ellipse->getEllipseAngle(hit) - startAngle)); };
    }
    QList<double> curveCuts{0.0, span};
    for (const auto& edge : region.edges) {
        const auto intersections = RS_Information::getIntersection(entity, edge.get(), true);
        for (size_t j = 0; j < intersections.getNumber(); ++j) {
            const auto hit = intersections.get(j);
            if (!hit.valid) continue;
            intersects = true;
            if (curveParameter) {
                const double parameter = curveParameter(hit);
                if (parameter <= span + RS_TOLERANCE_ANGLE) curveCuts.append(std::clamp(parameter, 0.0, span));
            }
            if (entity->rtti() == RS2::EntityLine && squaredLength > RS_TOLERANCE2)
                cuts.append(std::clamp(((hit - start).x * delta.x + (hit - start).y * delta.y) / squaredLength, 0.0, 1.0));
        }
    }
    if (crossing && intersects) return true;
    if (curvePoint) {
        if (!crossing && (!inside(region, curvePoint(0)) || !inside(region, curvePoint(span)))) return false;
        std::sort(curveCuts.begin(), curveCuts.end());
        for (int i = 1; i < curveCuts.size(); ++i) {
            const bool enclosed = inside(region, curvePoint((curveCuts[i - 1] + curveCuts[i]) / 2));
            if (crossing && enclosed) return true;
            if (!crossing && !enclosed) return false;
        }
        return !crossing;
    }
    if (entity->rtti() == RS2::EntityLine) {
        if (!crossing && (!inside(region, start) || !inside(region, end))) return false;
        if (crossing && (inside(region, start) || inside(region, end))) return true;
        std::sort(cuts.begin(), cuts.end());
        for (int i = 1; i < cuts.size(); ++i) {
            const bool enclosed = inside(region, start + delta * ((cuts[i - 1] + cuts[i]) / 2));
            if (crossing && enclosed) return true;
            if (!crossing && !enclosed) return false;
        }
        return !crossing;
    }

    // Images are filled rectangles, so use their perimeter instead of the
    // nearest-point query, which can return an interior point.
    if (entity->rtti() == RS2::EntityImage) {
        const auto* image = static_cast<const RS_Image*>(entity);
        const auto corners = image->getCorners();
        if (corners.getNumber() != 4) return false;
        for (size_t i = 0; i < 4; ++i) {
            RS_Line edge(nullptr, corners.get(i), corners.get((i + 1) % 4));
            const bool matched = matches(&edge, region, crossing);
            if (crossing && matched) return true;
            if (!crossing && !matched) return false;
        }
        if (!crossing) return true;
        for (const auto& lassoPoint : polygon) {
            const auto query = vector(lassoPoint);
            const auto nearest = image->getNearestPointOnEntity(query, true);
            if (nearest.valid && nearest.distanceTo(query) <= RS_TOLERANCE) return true;
        }
        return false;
    }

    // Exact curve/edge intersections plus a point on the curve distinguish
    // enclosed circles/arcs from a lasso wholly inside a closed curve.
    const auto nearest = entity->getNearestPointOnEntity(vector(polygon.first()), true);
    if (nearest.valid) return inside(region, nearest) && (crossing || !intersects);

    // Entities without an analytic boundary (e.g. images) use their extent box.
    const QPolygonF extent{point(min), QPointF(max.x, min.y), point(max), QPointF(min.x, max.y)};
    bool all = true, any = false;
    for (const auto& corner : extent) {
        const bool enclosed = inside(region, vector(corner));
        all &= enclosed;
        any |= enclosed;
    }
    return crossing ? any || extent.containsPoint(polygon.first(), Qt::OddEvenFill) : all;
}
}

void LC_PolygonSelection::select(RS_Document* document, LC_GraphicViewport* viewport,
                                const QPolygonF& boundary, RS2::EntityType type, bool crossing, bool selecting) {
    if (!document || !viewport || boundary.size() < 3) return;
    const Region region(boundary);
    RS_Selection selection(document, viewport);
    selection.performBulkSelection([&](RS_EntityContainer* container, LC_GraphicViewport*, RS_Document* doc) {
        QList<RS_Entity*> result;
        for (auto* entity : *container)
            if (!entity->isDeleted() && (type == RS2::EntityUnknown || type == entity->rtti()) && matches(entity, region, crossing))
                result.append(entity);
        if (selecting && !LC_GET_ONE_BOOL("Selection", "Additivity", true)) {
            QList<RS_Entity*> previous;
            document->collectSelected(previous);
            doc->select(previous, false);
        }
        doc->select(result, selecting);
    });
}
