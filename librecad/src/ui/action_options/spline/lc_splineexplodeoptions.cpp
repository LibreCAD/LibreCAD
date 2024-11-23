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

#include "lc_splineexplodeoptions.h"
#include "ui_lc_splineexplodeoptions.h"

LC_SplineExplodeOptions::LC_SplineExplodeOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawSplineExplode, "Draw", "SplineExplode")
    , ui(new Ui::LC_SplineExplodeOptions), action{nullptr}{
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::toggled, this, &LC_SplineExplodeOptions::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::toggled, this, &LC_SplineExplodeOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::toggled, this, &LC_SplineExplodeOptions::cbUseCurrentLayerClicked);
    connect(ui->cbCustomSegmentsCount, &QCheckBox::toggled, this, &LC_SplineExplodeOptions::cbCustomSegmentCountClicked);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_SplineExplodeOptions::cbPolylineClicked);
    connect(ui->sbSegmentsCount, &QSpinBox::valueChanged, this, &LC_SplineExplodeOptions::sbSegmentsCountValueChanged);
}

LC_SplineExplodeOptions::~LC_SplineExplodeOptions(){
    delete ui;
}

void LC_SplineExplodeOptions::doSaveSettings() {
    save("UseCustomSegmentsCount", ui->cbCustomSegmentsCount->isChecked());
    save("CustomSegmentsCount", ui->sbSegmentsCount->value());
    save("ToPolyline", ui->cbPolyline->isChecked());
    save("UseCurrentLayer", ui->cbLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_SplineExplodeOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionSplineExplode *>(a);
    bool toPolyline;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    bool keepOriginal;
    bool useCustomSegmentsCount;
    int customSegmentsCount;
    segmentsCountFromDrawing = action->getSegmentsCountFromDrawing();
    if (update){
        toPolyline = action->isToPolyline();
        useCurrentAttributes = action->isUseCurrentAttributes();
        useCurrentLayer = action->isUseCurrentLayer();
        keepOriginal = action->isKeepOriginals();
        useCustomSegmentsCount = action->isUseCustomSegmentsCount();
        customSegmentsCount = action->getCustomSegmentsCount();
    }
    else{
        toPolyline = loadBool("ToPolyline", false);
        useCurrentLayer = loadBool("UseCurrentLayer", true);
        useCurrentAttributes = loadBool("UseCurrentAttributes", true);
        keepOriginal = loadBool("KeepOriginals", false);
        useCustomSegmentsCount = loadBool("UseCustomSegmentsCount", false);
        customSegmentsCount = loadInt("CustomSegmentsCount", 8);
    }
    setPolylineToActionAndView(toPolyline);
    setSegmentsCountValueToActionAndView(customSegmentsCount);
    setUseCustomSegmentCount(useCustomSegmentsCount);
    setKeepOriginalsToActionAndView(keepOriginal);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
}

void LC_SplineExplodeOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_SplineExplodeOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_SplineExplodeOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_SplineExplodeOptions::cbPolylineClicked(bool val) {
    setPolylineToActionAndView(val);
}

void LC_SplineExplodeOptions::sbSegmentsCountValueChanged(int value) {
    setSegmentsCountValueToActionAndView(value);
}

void LC_SplineExplodeOptions::cbCustomSegmentCountClicked(bool val) {
    setUseCustomSegmentCount(val);
}


void LC_SplineExplodeOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_SplineExplodeOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_SplineExplodeOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_SplineExplodeOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbLayer->setChecked(val);
}

void LC_SplineExplodeOptions::setUseCustomSegmentCount(bool val) {
    action->setUseCustomSegmentsCount(val);
    ui->cbCustomSegmentsCount->setChecked(val);
    ui->sbSegmentsCount->setEnabled(val);
    if (!val){
        ui->sbSegmentsCount->setValue(segmentsCountFromDrawing);
    }
}

void LC_SplineExplodeOptions::setPolylineToActionAndView(bool val) {
    action->setUsePolyline(val);
    ui->cbPolyline->setChecked(val);
}

void LC_SplineExplodeOptions::setSegmentsCountValueToActionAndView(int value) {
    action->setSegmentsCountValue(value);
    ui->sbSegmentsCount->setValue(value);
}
