/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_drawboundingboxoptions.h"
#include "ui_lc_drawboundingboxoptions.h"

LC_DrawBoundingBoxOptions::LC_DrawBoundingBoxOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawBoundingBox, "Draw", "BoundingBox")
    , ui(new Ui::LC_DrawBoundingBoxOptions){
    ui->setupUi(this);
    connect(ui->cbAsGroup, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptions::onAsGroupToggled);
    connect(ui->cbCornerPointsOnly, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptions::onCornerPointsToggled);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_DrawBoundingBoxOptions::onPolylineToggled);
}

LC_DrawBoundingBoxOptions::~LC_DrawBoundingBoxOptions(){
    delete ui;
}

void LC_DrawBoundingBoxOptions::doSaveSettings() {
    save("AsGroup", ui->cbAsGroup->isChecked());
    save("CornerPoints", ui->cbCornerPointsOnly->isChecked());
    save("Polyline", ui->cbPolyline->isChecked());
}

void LC_DrawBoundingBoxOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionDrawBoundingBox *>(a);
    bool asGroup;
    bool cornerPoints;
    bool polyline;
    if (update){
        asGroup = action->isSelectionAsGroup();
        cornerPoints = action->isCornerPointsOnly();
        polyline = action->isCreatePolyline();
    }
    else{
        asGroup = loadBool("AsGroup", true);
        cornerPoints = loadBool("CornerPoints", false);
        polyline = loadBool("Polyline", false);
    }
    setAsGroupToActionAndView(asGroup);
    setCornerPointsOnlyToActionAndView(cornerPoints);
    setPolylineToActionAndView(polyline);
}

void LC_DrawBoundingBoxOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_DrawBoundingBoxOptions::onAsGroupToggled([[maybe_unused]] bool val) {
    setAsGroupToActionAndView(ui->cbAsGroup->isChecked());
}

void LC_DrawBoundingBoxOptions::onCornerPointsToggled([[maybe_unused]] bool val) {
    setCornerPointsOnlyToActionAndView(ui->cbCornerPointsOnly->isChecked());
}
void LC_DrawBoundingBoxOptions::onPolylineToggled([[maybe_unused]] bool val) {
    setPolylineToActionAndView(ui->cbPolyline->isChecked());
}

void LC_DrawBoundingBoxOptions::setAsGroupToActionAndView(bool group) {
    action->setSelectionAsGroup(group);
    ui->cbAsGroup->setChecked(group);
}

void LC_DrawBoundingBoxOptions::setCornerPointsOnlyToActionAndView(bool val) {
    action->setCornersOnly(val);
    ui->cbCornerPointsOnly->setChecked(val);
    ui->cbPolyline->setEnabled(!val);
}

void LC_DrawBoundingBoxOptions::setPolylineToActionAndView(bool p) {
    action->setCreatePolyline(p);
    ui->cbPolyline->setChecked(p);
}
