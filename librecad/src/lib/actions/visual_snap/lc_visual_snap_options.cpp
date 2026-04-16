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

#include "lc_visual_snap_options.h"

#include "rs_settings.h"

void LC_VisualSnapOptions::load() {
    LC_GROUP("Snap");
    {
        vertexSizeNormal = LC_GET_INT("VSVertexSize", 6);
        vertexSizeProjected = LC_GET_INT("VSProjectedSnapSize", 4);
        vertexSizeHighlighted = LC_GET_INT("VSHighlightedVertexSize", 10);
        delayMsSnapVertex = LC_GET_INT("VSSnapPointAddingDelay", 300);
        delayMsProjectedSnap = LC_GET_INT("VSVertexAddingDelay", 1200);
        delayMsDocumentEntity = LC_GET_INT("VSDocEntityAddingDelay", 1500);

        createAngleStepRaysForVertexes = LC_GET_BOOL("VSAngleSnapStepRaysVertexes", true);
        createAngleStepRaysForEntitiesEndpoints = LC_GET_BOOL("VSAngleSnapStepRaysRelative", true);
        createVertexVertexDistanceCircles = LC_GET_BOOL("VSVertexVertexDistanceCircles", true);
        createVertexVertexDistanceCirclesTangents = LC_GET_BOOL("VSVertexVertexDistanceTangents", true);

        autoAddSnappedPointToVisualSnap = LC_GET_BOOL("VSSnapAutoAddSnapPoint", true);
        autoAddGuidesForLastSnapOnly = LC_GET_BOOL("VSSnapAutoAddLastSnapPointOnly", true);
        manualVertexAddingRequiresCTRL = LC_GET_BOOL("VSSnapManualAddingWithCTRL", false);
        guidingEntitiesSnapDistance = LC_GET_INT("VSGuidingEntitiesCatchDistance", 24);

        showGuidingEntitiesLabels = LC_GET_BOOL("VSGuidingEntitiesShowLabels", true);
        guidingEntitiesFontSize = LC_GET_INT("VSGuidingEntityLabelFontSize", 10);

        allowClearingVisualSnapByRMB = LC_GET_BOOL("VSClearSolutionByRMB", false);

        baseLabelOffsetPx = LC_GET_INT("VSGuidingLabelOffsetPx", 50);
        showNotSnappableGuides = LC_GET_BOOL("VSShowNotSnappableGuides", false);
    }
    LC_GROUP_END();

    LC_GROUP("InfoOverlayCursor");
    {
        QString fontName = LC_GET_STR("FontName", "Helvetica");
        guidingEntitiesFont = QFont(fontName, guidingEntitiesFontSize);

        guidingEntitiesFontActive = QFont(fontName, guidingEntitiesFontSize + 2);
        guidingEntitiesFontActive.setBold(true);
    }
}
