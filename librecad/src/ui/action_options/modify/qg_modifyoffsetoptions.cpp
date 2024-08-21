/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

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

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "qg_modifyoffsetoptions.h"

#include "rs_math.h"
#include "ui_qg_modifyoffsetoptions.h"
#include "rs_actionmodifyoffset.h"

/*
 *  Constructs a QG_ModifyOffsetOptions
 */
QG_ModifyOffsetOptions::QG_ModifyOffsetOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyOffset, "Draw", "ModifyOffset")
    , ui(new Ui::Ui_ModifyOffsetOptions{}){
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &QG_ModifyOffsetOptions::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &QG_ModifyOffsetOptions::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &QG_ModifyOffsetOptions::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &QG_ModifyOffsetOptions::cbUseCurrentLayerClicked);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &QG_ModifyOffsetOptions::onDistEditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &QG_ModifyOffsetOptions::onNumberOfCopiesValueChanged);
    connect(ui->cbFixedDistance, &QCheckBox::clicked, this, &QG_ModifyOffsetOptions::onFixedDistanceClicked);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ModifyOffsetOptions::~QG_ModifyOffsetOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ModifyOffsetOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_ModifyOffsetOptions::doSaveSettings() {
    save("Distance", ui->leDist->text());
    save("DistanceFixed", ui->cbFixedDistance->isChecked());

    save("UseCurrentLayer", ui->cbCurrentLayer->isChecked());
    save("UseCurrentAttributes", ui->cbCurrentAttr->isChecked());
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
    save("MultipleCopies", ui->cbMultipleCopies->isChecked());
    save("Copies", ui->sbNumberOfCopies->value());
}

void QG_ModifyOffsetOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyOffset *>(a);

    QString dist;
    bool distanceFixed;

    bool useMultipleCopies;
    bool keepOriginals;
    bool useCurrentLayer;
    bool useCurrentAttributes;
    int copiesNumber;

    if (update) {
        dist = fromDouble(action->getDistance());
        distanceFixed = action->isFixedDistance();
        useCurrentLayer = action->isUseCurrentLayer();
        useCurrentAttributes  = action->isUseCurrentAttributes();
        copiesNumber = action->getCopiesNumber();
        keepOriginals = action->isKeepOriginals();
        useMultipleCopies = action->isUseMultipleCopies();
    } else {
        dist = load("Distance", "1.0");
        distanceFixed = loadBool("DistanceFixed", true);
        useCurrentLayer = loadBool("UseCurrentLayer", true);
        useCurrentAttributes = loadBool("UseCurrentAttributes", true);
        keepOriginals = loadBool("KeepOriginals", true);
        useMultipleCopies = loadBool("MultipleCopies", false);
        copiesNumber = loadInt("Copies", 1);
    }
    setDistanceToActionAndView(dist);
    setDistanceFixedToActionAndView(distanceFixed);
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
    setUseCurrentLayerToActionAndView(useCurrentLayer);
    setUseCurrentAttributesToActionAndView(useCurrentAttributes);
    setKeepOriginalsToActionAndView(keepOriginals);
}

void QG_ModifyOffsetOptions::onDistEditingFinished() {
    setDistanceToActionAndView(ui->leDist->text());
}

void QG_ModifyOffsetOptions::onFixedDistanceClicked(bool val) {
    setDistanceFixedToActionAndView(val);
}

void QG_ModifyOffsetOptions::setDistanceFixedToActionAndView(bool val) {
    action->setDistanceFixed(val);
    ui->leDist->setEnabled(val);
    ui->cbFixedDistance->setChecked(val);
}

void QG_ModifyOffsetOptions::setDistanceToActionAndView(QString val) {
    double distance;
    if (toDouble(val, distance, 1.0, false)) {
        action->setDistance(distance);
        ui->leDist->setText(fromDouble(distance));
    }
}

void QG_ModifyOffsetOptions::setCopiesNumberToActionAndView(int number) {
    if (number < 1){
        number = 1;
    }
    action->setCopiesNumber(number);
    ui->sbNumberOfCopies->setValue(number);
}

void QG_ModifyOffsetOptions::setUseMultipleCopiesToActionAndView(bool copies) {
    action->setUseMultipleCopies(copies);
    ui->cbMultipleCopies->setChecked(copies);
    ui->sbNumberOfCopies->setEnabled(copies);
}

void QG_ModifyOffsetOptions::setUseCurrentLayerToActionAndView(bool val) {
    action->setUseCurrentLayer(val);
    ui->cbCurrentLayer->setChecked(val);
}

void QG_ModifyOffsetOptions::setUseCurrentAttributesToActionAndView(bool val) {
    action->setUseCurrentAttributes(val);
    ui->cbCurrentAttr->setChecked(val);
}

void QG_ModifyOffsetOptions::setKeepOriginalsToActionAndView(bool val) {
    action->setKeepOriginals(val);
    ui->cbKeepOriginals->setChecked(val);
}

void QG_ModifyOffsetOptions::cbKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void QG_ModifyOffsetOptions::cbMultipleCopiesClicked(bool val) {
    setUseMultipleCopiesToActionAndView(val);
}

void QG_ModifyOffsetOptions::cbUseCurrentAttributesClicked(bool val) {
    setUseCurrentAttributesToActionAndView(val);
}

void QG_ModifyOffsetOptions::cbUseCurrentLayerClicked(bool val) {
    setUseCurrentLayerToActionAndView(val);
}

void QG_ModifyOffsetOptions::onNumberOfCopiesValueChanged(int number) {
    setCopiesNumberToActionAndView(number);
}
