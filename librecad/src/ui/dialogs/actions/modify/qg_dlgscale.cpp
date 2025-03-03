/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

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
*  Destroys the object and frees any allocated resources
*/
QG_DlgScale::~QG_DlgScale() {
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
*  Sets the strings of the subwidgets using the current
*  language.
*/
void QG_DlgScale::languageChange() {
    retranslateUi(this);
}

void QG_DlgScale::onFactorXChanged(const QString &arg1) {
    if (cbIsotropic->isChecked()) {
        leFactorY->setText(arg1);
    }
}

void QG_DlgScale::onIsotropicToggled(bool checked) {
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

    leFactorY->setReadOnly(isotropic);

    connect(cbIsotropic, &QCheckBox::toggled, this, &QG_DlgScale::onIsotropicToggled);
    connect(leFactorX, &QLineEdit::textEdited, this, &QG_DlgScale::onFactorXChanged);
    connect(cbMultipleCopies, &QCheckBox::clicked, this, &QG_DlgScale::onMultipleCopiesClicked);
    connect(bFindFactor, &QPushButton::clicked, this, &QG_DlgScale::onFactorByPoints);
}

void QG_DlgScale::onFactorByPoints() {
    if (data != nullptr) {
        data->toFindFactor = true;
        data->isotropicScaling = cbIsotropic->isChecked();
        accept();
    }
}
 void QG_DlgScale::onMultipleCopiesClicked(){
    sbNumber->setEnabled(cbMultipleCopies->isChecked());
}

void QG_DlgScale::setData(RS_ScaleData *d) {
    data = d;
    init();
}

void QG_DlgScale::updateData() {
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

    bool okX = false;
    double sx = RS_Math::eval(leFactorX->text(), &okX);
    bool okY = false;
    double sy = RS_Math::eval(leFactorY->text(), &okY);
    if (okX && okY) {
        data->factor = RS_Vector(sx, sy);
    } else {
        data->factor = RS_Vector(1,1,0);
    }
}
