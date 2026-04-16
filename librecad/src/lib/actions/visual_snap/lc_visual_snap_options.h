
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

#ifndef LC_VISUALSNAPOPTIONS_H
#define LC_VISUALSNAPOPTIONS_H

#include <QFont>

struct LC_VisualSnapOptions {
    int delayMsSnapVertex = 300;
    int delayMsProjectedSnap = 1200;
    int delayMsDocumentEntity = 1500;

    int vertexSizeNormal = 6;
    int vertexSizeProjected = 4;
    int vertexSizeHighlighted = 10;

    bool createAngleStepRaysForVertexes = true;
    bool createAngleStepRaysForEntitiesEndpoints = true;
    bool createVertexVertexDistanceCircles = true;
    bool createVertexVertexDistanceCirclesTangents = true;

    bool autoAddSnappedPointToVisualSnap = false;;
    bool autoAddGuidesForLastSnapOnly = true;

    bool manualVertexAddingRequiresCTRL = false;
    int guidingEntitiesSnapDistance = 24;

    bool allowClearingVisualSnapByRMB = false;

    bool showGuidingEntitiesLabels = true;
    int guidingEntitiesFontSize = 10;

    int baseLabelOffsetPx = 50;
    QFont guidingEntitiesFont;
    QFont guidingEntitiesFontActive;

    bool showNotSnappableGuides = false;

    void load();
};



#endif
