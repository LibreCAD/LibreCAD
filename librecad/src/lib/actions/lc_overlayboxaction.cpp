/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_overlayboxaction.h"
#include "rs_overlaybox.h"

LC_OverlayBoxAction::LC_OverlayBoxAction(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, container, graphicView, actionType) {}

void LC_OverlayBoxAction::drawOverlayBox(const RS_Vector &corner1, const RS_Vector &corner2) {
    auto* ob = new RS_OverlayBox(corner1, corner2, &overlayBoxOptions);
    addOverlay(ob, RS2::OverlayGraphics::OverlayEffects);
}

void LC_OverlayBoxAction::initFromSettings() {
    RS_Snapper::initFromSettings();
    overlayBoxOptions.loadSettings();
}
