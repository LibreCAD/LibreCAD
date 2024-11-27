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

#include "lc_splinefrompolylineoptions.h"
#include "ui_lc_splinefrompolylineoptions.h"

LC_SplineFromPolylineOptions::LC_SplineFromPolylineOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawSplineFromPolyline, "Draw", "SplineFromPolyline")
    , ui(new Ui::LC_SplineFromPolylineOptions),action{nullptr}{
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptions::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptions::cbUseCurrentLayerClicked);
    connect(ui->cbFitPoints, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptions::cbUseFitPointsClicked);
    connect(ui->sbDegree, &QSpinBox::valueChanged, this, &LC_SplineFromPolylineOptions::sbDegreeValueChanged);
    connect(ui->sbMidPoints, &QSpinBox::valueChanged, this, &LC_SplineFromPolylineOptions::sbMidPointsValueChanged);
}

LC_SplineFromPolylineOptions::~LC_SplineFromPolylineOptions(){
    delete ui;
}

void LC_SplineFromPolylineOptions::doSaveSettings() {
    save("Degree", ui->sbDegree->value());
    save("MidPointsCount", ui->sbMidPoints->value());
    save("UseFitPoints", ui->cbFitPoints->isChecked());
    save("UseCurrentLayer", ui->cbLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_SplineFromPolylineOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionSplineFromPolyline *>(a);
    bool useFitPoints;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    bool keepOriginal;
    int midPoints;
    int splineDegree;
    if (update){
        useFitPoints = action->isUseFitPoints();
        useCurrentAttributes = action->isUseCurrentAttributes();
        useCurrentLayer = action->isUseCurrentLayer();
        keepOriginal = action->isKeepOriginals();
        midPoints = action->getSegmentPoints();
        splineDegree = action->getSplineDegree();
    }
    else{
        useFitPoints = loadBool("UseFitPoints", false);
        useCurrentLayer = loadBool("UseCurrentLayer", true);
        useCurrentAttributes = loadBool("UseCurrentAttributes", true);
        keepOriginal = loadBool("KeepOriginals", false);
        midPoints = loadInt("MidPointsCount", 1);
        splineDegree = loadInt("Degree", 3);
    }
    setKeepOriginalsToActionAndView(keepOriginal);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setUseFitPointsToActionAndView(useFitPoints);
    setDegreeToActionAndView(splineDegree);
    setMidPointsToActionAndView(midPoints);
}

void LC_SplineFromPolylineOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_SplineFromPolylineOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void LC_SplineFromPolylineOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void LC_SplineFromPolylineOptions::cbUseFitPointsClicked(bool val) {
    setUseFitPointsToActionAndView(val);
}

void LC_SplineFromPolylineOptions::sbDegreeValueChanged(int value) {
    setDegreeToActionAndView(value);
}

void LC_SplineFromPolylineOptions::sbMidPointsValueChanged(int value) {
    setMidPointsToActionAndView(value);
}

void LC_SplineFromPolylineOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_SplineFromPolylineOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void LC_SplineFromPolylineOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void LC_SplineFromPolylineOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbLayer->setChecked(val);
}

void LC_SplineFromPolylineOptions::setUseFitPointsToActionAndView(bool val) {
    action->setUseFitPoints(val);
    ui->cbFitPoints->setChecked(val);
}

void LC_SplineFromPolylineOptions::setMidPointsToActionAndView(int value) {
   action->setSegmentPoints(value);
   ui->sbMidPoints->setValue(value);
}

void LC_SplineFromPolylineOptions::setDegreeToActionAndView(int value) {
    action->setSplineDegree(value);
    ui->sbDegree->setValue(value);
    ui->cbFitPoints->setEnabled(value == 2);
}
