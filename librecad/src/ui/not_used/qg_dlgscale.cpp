/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "qg_dlgscale.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_DlgScale as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */

QG_DlgScale::QG_DlgScale(QWidget *parent, bool modal, Qt::WindowFlags fl)
    :QDialog(parent, fl) {
    setModal(modal);
    setupUi(this);
}

/*
*  Sets the strings of the subwidgets using the current
*  language.
*/
void QG_DlgScale::languageChange() {
    retranslateUi(this);
}

void QG_DlgScale::onFactorXChanged(const QString &arg1) const {
    if (cbIsotropic->isChecked()) {
        leFactorY->setText(arg1);
    }
}

void QG_DlgScale::onIsotropicToggled(bool checked) const {
    leFactorY->setDisabled(checked);
    leFactorY->setReadOnly(checked);
    if (checked) {
        leFactorY->setText(leFactorX->text());
    }
}

void QG_DlgScale::init() {
    rbCopy->setChecked(data->keepOriginals);
    rbDeleteOrigin->setChecked(!data->keepOriginals);
    cbMultipleCopies -> setChecked(data->multipleCopies);

    sbNumber->setValue(data->number);
    cbCurrentAttributes->setChecked(data->useCurrentAttributes);
    cbCurrentLayer->setChecked(data->useCurrentLayer);    
    sbNumber->setEnabled(data->multipleCopies);

    bool isotropic = data->isotropicScaling;
    cbIsotropic->setChecked(isotropic);

    QString scaleX = QString("%1").arg(data->factor.x);
    leFactorX->setText(scaleX);
    if (isotropic){
        leFactorY->setText(scaleX);
    }
    else {
        leFactorY->setText(QString("%1").arg(data->factor.y));
    }

    bool findFactor = data->toFindFactor;
    bFindFactor->setChecked(findFactor);

    leFactorY->setDisabled(findFactor || isotropic);
    leFactorY->setReadOnly(findFactor || isotropic);

    connect(cbIsotropic, &QCheckBox::toggled, this, &QG_DlgScale::onIsotropicToggled);
    connect(leFactorX, &QLineEdit::textEdited, this, &QG_DlgScale::onFactorXChanged);
    connect(cbMultipleCopies, &QCheckBox::clicked, this, &QG_DlgScale::onMultipleCopiesClicked);
    connect(bFindFactor, &QPushButton::toggled, this, &QG_DlgScale::onFactorByPoints);
}

void QG_DlgScale::onFactorByPoints(bool checked) {
    if (data != nullptr) {
        data->toFindFactor = checked;
        data->isotropicScaling = cbIsotropic->isChecked();
        leFactorX->setDisabled(checked);
        leFactorY->setDisabled(checked || cbIsotropic->isChecked());
        accept();
    }
}
 void QG_DlgScale::onMultipleCopiesClicked() const {
    sbNumber->setEnabled(cbMultipleCopies->isChecked());
}

void QG_DlgScale::setData(RS_ScaleData *d) {
    data = d;
    init();
}

void QG_DlgScale::updateData() const {
    if (rbDeleteOrigin->isChecked()) {
        data->keepOriginals = false;
    } else if (rbCopy->isChecked()) {
        data->keepOriginals = true;
    }

    data->number = sbNumber->value();
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
    data->multipleCopies = cbMultipleCopies->isChecked();

    data->isotropicScaling = cbIsotropic->isChecked();
    data->toFindFactor = bFindFactor->isChecked();

    bool okX = false;
    double sx = RS_Math::eval(leFactorX->text(), &okX);
    bool okY = false;
    const double sy = RS_Math::eval(leFactorY->text(), &okY);
    if (okX && okY) {
        data->factor = RS_Vector(sx, sy);
    } else {
        data->factor = RS_Vector(1., 1.);
    }

    if (data->isotropicScaling) {
        if (okX) {
            data->factor = RS_Vector{sx, sx};
            leFactorY->setText(leFactorX->text());
        }
        leFactorY->setDisabled(true);
    }
}
