/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD contributors
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
**********************************************************************/
#ifndef RS_WIPEOUT_H
#define RS_WIPEOUT_H

#include <vector>

#include "rs_atomicentity.h"

/**
 * Holds the data that defines a wipeout polygon.
 *
 * Note: WIPEOUT does NOT carry a per-entity frame-display flag in the
 * file format (per ODA spec §20.4.80 — group 290 there is the IMAGE
 * Clip mode, not a frame flag).  Whether the polygon outline is rendered
 * is a global drawing setting (WIPEOUTFRAME in the WIPEOUTVARIABLES
 * object), out of scope here; outline is drawn unconditionally for now.
 */
struct RS_WipeoutData {
    RS_WipeoutData() = default;
    explicit RS_WipeoutData(std::vector<RS_Vector> verts)
        : vertices(std::move(verts)) {}

    /** Polygon vertices in WCS, ordered around the polygon. */
    std::vector<RS_Vector> vertices;
};

/**
 * Class for a WIPEOUT entity — a polygon filled with the viewport background
 * color, used to mask underlying entities.  Format-wise WIPEOUT inherits from
 * IMAGE in DXF/DWG, but in LibreCAD it's modeled as its own atomic entity
 * because it carries no raster data.
 */
class RS_Wipeout : public RS_AtomicEntity {
public:
    RS_Wipeout(RS_EntityContainer* parent, RS_WipeoutData d);

    RS_Entity* clone() const override;

    /** @return RS2::EntityWipeout */
    RS2::EntityType rtti() const override {
        return RS2::EntityWipeout;
    }

    const RS_WipeoutData& getData() const { return data; }
    const std::vector<RS_Vector>& getVertices() const { return data.vertices; }

    void calculateBorders() override;
    void draw(RS_Painter* painter) override;

    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist = nullptr) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
                                      bool onEntity = true,
                                      double* dist = nullptr,
                                      RS_Entity** entity = nullptr) const override;
    RS_Vector getNearestCenter(const RS_Vector& coord,
                               double* dist = nullptr) const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord,
                               double* dist = nullptr,
                               int middlePoints = 1) const override;
    RS_Vector getNearestDist(double distance,
                             const RS_Vector& coord,
                             double* dist = nullptr) const override;
    double getDistanceToPoint(const RS_Vector& coord,
                              RS_Entity** entity = nullptr,
                              RS2::ResolveLevel level = RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear([[maybe_unused]] double k) override { return *this; }

protected:
    RS_WipeoutData data;
};

#endif // RS_WIPEOUT_H
