// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#ifndef LC_POLYGONSELECTION_H
#define LC_POLYGONSELECTION_H

#include <QPolygonF>
#include "rs.h"

class RS_Document;
class LC_GraphicViewport;

class LC_PolygonSelection {
public:
    static void select(RS_Document* document, LC_GraphicViewport* viewport,
                       const QPolygonF& boundary, RS2::EntityType type, bool crossing, bool selecting);
};
#endif
