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

#include "lc_selectwindowoptions.h"
#include "rs_actionselectwindow.h"
#include "ui_lc_selectwindowoptions.h"

LC_SelectWindowOptions::LC_SelectWindowOptions()
:LC_ActionOptionsWidgetBase(RS2::ActionSelectWindow, "Select", "Window"), ui(new Ui::LC_SelectWindowOptions){
    ui->setupUi(this);
    connect(ui->cbAll, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onAllToggled);
    connect(ui->cbLine, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbArc, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbCircle, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbEllipse, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbPoint, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbText, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbMText, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbSpline, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbInsert, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbImage, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbHatch, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbDimension, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    connect(ui->cbWipeout, &QCheckBox::toggled, this, &LC_SelectWindowOptions::onTypeToggled);
    ui->cbWipeout->setVisible(false);
}

LC_SelectWindowOptions::~LC_SelectWindowOptions(){
    delete ui;
}

void LC_SelectWindowOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_SelectWindowOptions::doSaveSettings() {
    save("All", ui->cbAll->isChecked());
    save("Line", ui->cbLine->isChecked());
    save("Arc", ui->cbArc->isChecked());
    save("Circle", ui->cbCircle->isChecked());
    save("Point", ui->cbPoint->isChecked());
    save("Ellipse", ui->cbEllipse->isChecked());
    save("Spline", ui->cbSpline->isChecked());
    save("Polyline", ui->cbPolyline->isChecked());
    save("Text", ui->cbText->isChecked());
    save("MText", ui->cbMText->isChecked());
    save("Image", ui->cbImage->isChecked());
    save("Hatch", ui->cbHatch->isChecked());
    save("Wipeout", ui->cbWipeout->isChecked());
    save("Dimension", ui->cbDimension->isChecked());
    save("Insert", ui->cbInsert->isChecked());
}

void LC_SelectWindowOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<RS_ActionSelectWindow *>(a);
    bool all;
    bool line = false;
    bool arc  = false;
    bool point = false;
    bool polyline  = false;
    bool ellipse  = false;
    bool circle = false;
    bool spline = false;
    bool hatch  = false;
    bool image  = false;
    bool text = false;
    bool mtext = false;
    bool wipeout = false;
    bool dimension = false;
    bool insert = false;
    QList<RS2::EntityType> entityTypes;
    if (update){
        all = m_action->isSelectAllEntityTypes();
        entityTypes = m_action->getEntityTypesToSelect();
    }
    else{
        all = loadBool("All", true);
        line = loadBool("Line",false);
        arc = loadBool("Arc", false);
        circle = loadBool("Circle", false);
        point = loadBool("Point", false);
        ellipse= loadBool("Ellipse",false);
        spline = loadBool("Spline",false);
        polyline = loadBool("Polyline", false);
        text = loadBool("Text", false);
        mtext = loadBool("MText", false);
        image = loadBool("Image", false);
        hatch = loadBool("Hatch", false);
        wipeout= loadBool("Wipeout", false);
        dimension = loadBool("Dimension", false);
        insert = loadBool("Insert", false);

        if (line){
            entityTypes << RS2::EntityLine;
        }
        if (arc){
            entityTypes << RS2::EntityArc;
        }
        if (circle){
            entityTypes << RS2::EntityCircle;
        }
        if (point){
            entityTypes << RS2::EntityPoint;
        }
        if (polyline){
            entityTypes << RS2::EntityPolyline;
        }
        if (ellipse){
            entityTypes << RS2::EntityEllipse;
        }
        if (spline){
            entityTypes << RS2::EntitySpline;
            entityTypes << RS2::EntitySplinePoints;
            entityTypes << RS2::EntityParabola;
        }
        if (image){
            entityTypes << RS2::EntityImage;
        }
        if (hatch){
            entityTypes << RS2::EntityHatch;
        }
        if (insert){
            entityTypes << RS2::EntityInsert;
        }
        if (mtext){
            entityTypes << RS2::EntityMText;
        }
        if (text){
            entityTypes << RS2::EntityText;
        }
        if (wipeout){
//            entityTypes << RS2::EntityText;
        }
        if (dimension){
            entityTypes << RS2::EntityDimRadial;
            entityTypes << RS2::EntityDimArc;
            entityTypes << RS2::EntityDimDiametric;
            entityTypes << RS2::EntityDimLeader;
            entityTypes << RS2::EntityDimLinear;
            entityTypes << RS2::EntityDimOrdinate;
            entityTypes << RS2::EntityTolerance;
            entityTypes << RS2::EntityDimAngular;
            entityTypes << RS2::EntityDimAligned;
        }
    }
    setSelectAllToActionAndView(all);
    setEntityTypesToActinAndView(entityTypes);
}

void LC_SelectWindowOptions::onAllToggled([[maybe_unused]]bool value) {
    setSelectAllToActionAndView(ui->cbAll->isChecked());
}

void LC_SelectWindowOptions::onTypeToggled([[maybe_unused]]bool value) {
    QList<RS2::EntityType> entityTypes;
    if (ui->cbLine->isChecked()){
        entityTypes << RS2::EntityLine;
    }
    if (ui->cbArc->isChecked()){
        entityTypes << RS2::EntityArc;
    }
    if (ui->cbCircle->isChecked()){
        entityTypes << RS2::EntityCircle;
    }
    if (ui->cbPoint->isChecked()){
        entityTypes << RS2::EntityPoint;
    }
    if (ui->cbPolyline->isChecked()){
        entityTypes << RS2::EntityPolyline;
    }
    if (ui->cbEllipse->isChecked()){
        entityTypes << RS2::EntityEllipse;
    }
    if (ui->cbSpline->isChecked()){
        entityTypes << RS2::EntitySpline;
        entityTypes << RS2::EntitySplinePoints;
        entityTypes << RS2::EntityParabola;
    }
    if (ui->cbImage->isChecked()){
        entityTypes << RS2::EntityImage;
    }
    if (ui->cbHatch->isChecked()){
        entityTypes << RS2::EntityHatch;
    }
    if (ui->cbInsert->isChecked()){
        entityTypes << RS2::EntityInsert;
    }
    if (ui->cbMText->isChecked()){
        entityTypes << RS2::EntityMText;
    }
    if (ui->cbText->isChecked()){
        entityTypes << RS2::EntityText;
    }
    if (ui->cbWipeout->isChecked()){
//            entityTypes << RS2::EntityText;
    }
    if (ui->cbDimension->isChecked()) {
        entityTypes << RS2::EntityDimRadial;
        entityTypes << RS2::EntityDimArc;
        entityTypes << RS2::EntityDimDiametric;
        entityTypes << RS2::EntityDimLeader;
        entityTypes << RS2::EntityDimLinear;
        entityTypes << RS2::EntityDimOrdinate;
        entityTypes << RS2::EntityTolerance;
        entityTypes << RS2::EntityDimAngular;
        entityTypes << RS2::EntityDimAligned;
    }
    m_action->setEntityTypesToSelect(entityTypes);
  /*  if (entityTypes.isEmpty()){
        setSelectAllToActionAndView(true);
    }*/
}

void LC_SelectWindowOptions::setEntityTypesToActinAndView(QList<RS2::EntityType> entityTypes) {
    m_action->setEntityTypesToSelect(entityTypes);
    ui->cbLine->setChecked(false);
    ui->cbArc->setChecked(false);
    ui->cbCircle->setChecked(false);
    ui->cbPoint->setChecked(false);
    ui->cbPolyline->setChecked(false);
    ui->cbEllipse->setChecked(false);
    ui->cbSpline->setChecked(false);
    ui->cbText->setChecked(false);
    ui->cbMText->setChecked(false);
    ui->cbHatch->setChecked(false);
    ui->cbImage->setChecked(false);
    ui->cbInsert->setChecked(false);
    ui->cbDimension->setChecked(false);
    ui->cbWipeout->setChecked(false);
    for (auto t: entityTypes){
        switch (t){
            case RS2::EntityLine:
                ui->cbLine->setChecked(true);
                break;
            case RS2::EntityArc:
                ui->cbArc->setChecked(true);
                break;
            case RS2::EntityEllipse:
                ui->cbEllipse->setChecked(true);
                break;
            case RS2::EntityCircle:
                ui->cbCircle->setChecked(true);
                break;
            case RS2::EntityPolyline:
                ui->cbPolyline->setChecked(true);
                break;
            case RS2::EntityPoint:
                ui->cbPoint->setChecked(true);
                break;
            case RS2::EntityInsert:
                ui->cbInsert->setChecked(true);
                break;
            case RS2::EntityImage:
                ui->cbImage->setChecked(true);
                break;
            case RS2::EntityHatch:
                ui->cbHatch->setChecked(true);
                break;
            case RS2::EntitySpline:
            case RS2::EntitySplinePoints:
            case RS2::EntityParabola:
                ui->cbSpline->setChecked(true);
                break;
            case RS2::EntityDimLeader:
            case RS2::EntityDimAligned:
            case RS2::EntityDimArc:
            case RS2::EntityDimAngular:
            case RS2::EntityDimLinear:
            case RS2::EntityDimOrdinate:
            case RS2::EntityTolerance:
            case RS2::EntityDimDiametric:
            case RS2::EntityDimRadial:
                ui->cbDimension->setChecked(true);
                break;
            default:
                break;
        }
    }
}


void LC_SelectWindowOptions::setSelectAllToActionAndView(bool value) {
    ui->cbAll->setChecked(value);
    m_action->setSelectAllEntityTypes(value);
    bool enable = !value;
    enableEntityTypes(enable);
    if (!value){
        onTypeToggled(value);
    }
}

void LC_SelectWindowOptions::enableEntityTypes(bool enable) const {
    ui->cbLine->setEnabled(enable);
    ui->cbArc->setEnabled(enable);
    ui->cbCircle->setEnabled(enable);
    ui->cbPoint->setEnabled(enable);
    ui->cbPolyline->setEnabled(enable);
    ui->cbEllipse->setEnabled(enable);
    ui->cbSpline->setEnabled(enable);
    ui->cbText->setEnabled(enable);
    ui->cbMText->setEnabled(enable);
    ui->cbHatch->setEnabled(enable);
    ui->cbImage->setEnabled(enable);
    ui->cbInsert->setEnabled(enable);
    ui->cbDimension->setEnabled(enable);
    ui->cbWipeout->setEnabled(enable);
}
