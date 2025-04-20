/*
 * **************************************************************************
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
 * *********************************************************************
 */
#include "qg_dlgimageoptions.h"
#include "rs_math.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_ImageOptionsDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_ImageOptionsDialog::QG_ImageOptionsDialog(QWidget* parent)
    : LC_Dialog(parent, "ImageOptions"){
    setupUi(this);
    init();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ImageOptionsDialog::languageChange()
{
    retranslateUi(this);
}

void QG_ImageOptionsDialog::init() {
    m_graphicSize = RS_Vector(0.0,0.0);
    m_updateEnabled = false;
    m_useResolution = true;

    LC_GROUP_GUARD("Export");
    {
        if (LC_GET_BOOL("UseResolution", true)) {
            cbResolution->setCurrentIndex(cbResolution->findText(QString("%1").arg(LC_GET_STR("Resolution", "1"))));
        } else {
            leWidth->setText(LC_GET_STR("Width", "640"));
            leHeight->setText(LC_GET_STR("Height", "480"));
        }
        if (LC_GET_BOOL("BlackBackground")) {
            rbBlack->setChecked(true);
            rbWhite->setChecked(false);
        } else {
            rbBlack->setChecked(false);
            rbWhite->setChecked(true);
        }
        if (LC_GET_BOOL("BlackWhite", true)) {
            rbBlackWhite->setChecked(true);
            rbColoured->setChecked(false);
        } else {
            rbBlackWhite->setChecked(false);
            rbColoured->setChecked(true);
        }
        leLeftRight->setText(LC_GET_STR("BorderLeftRight", "5"));
        leTopBottom->setText(LC_GET_STR("BorderTopBottom", "5"));
        if (LC_GET_STR("BorderSameSize", "1") == "1") {
            cbSameBorders->setChecked(true);
            sameBordersChanged();
        }
    }

    m_updateEnabled = true;
}

void QG_ImageOptionsDialog::setGraphicSize(const RS_Vector& s) {
    m_graphicSize = s;
    if(!m_useResolution){
        sizeChanged();
    }
    else {
        resolutionChanged();
    }
}

void QG_ImageOptionsDialog::ok() {
    LC_GROUP_GUARD("Export");
    {
        LC_SET("UseResolution", m_useResolution);
        LC_SET("Resolution", cbResolution->currentText());
        LC_SET("Width", leWidth->text());
        LC_SET("Height", leHeight->text());
        LC_SET("BorderLeftRight", leLeftRight->text());
        LC_SET("BorderTopBottom", leTopBottom->text());
        LC_SET("BorderSameSize", cbSameBorders->isChecked());
        LC_SET("BlackBackground", rbBlack->isChecked());
        LC_SET("BlackWhite", rbBlackWhite->isChecked());
    }
    accept();
}

void QG_ImageOptionsDialog::sameBordersChanged() {
    if(cbSameBorders->isChecked()) {
        leTopBottom->setText(leLeftRight->text());
        leTopBottom->setDisabled(true);
    }
    else {
        leTopBottom->setEnabled(true);
    }
}

void QG_ImageOptionsDialog::borderChanged() {
    if(cbSameBorders->isChecked()) {
        leTopBottom->setText(leLeftRight->text());
    }
}

void QG_ImageOptionsDialog::sizeChanged() {
    if (m_updateEnabled) {
        m_updateEnabled = false;
        m_useResolution = false;
        cbResolution->setCurrentIndex(cbResolution->findText("auto"));
        m_updateEnabled = true;
    }
}

void  QG_ImageOptionsDialog::resolutionChanged() {
    if (m_updateEnabled) {
        m_updateEnabled = false;
        bool ok = false;
        double res = RS_Math::eval(cbResolution->currentText(), &ok);
        if (!ok) {
            res = 1.0;
        }
        int w = RS_Math::round(res * m_graphicSize.x);
        int h = RS_Math::round(res * m_graphicSize.y);
        m_useResolution = true;
        leWidth->setText(QString("%1").arg(w));
        leHeight->setText(QString("%1").arg(h));
        m_updateEnabled = true;
    }
}

QSize QG_ImageOptionsDialog::getSize() {
    return QSize(RS_Math::round(RS_Math::eval(leWidth->text())),
                    RS_Math::round(RS_Math::eval(leHeight->text())));
}

QSize QG_ImageOptionsDialog::getBorders() {
    return QSize(RS_Math::round(RS_Math::eval(leLeftRight->text())),
                   RS_Math::round(RS_Math::eval(leTopBottom->text())));
}

bool QG_ImageOptionsDialog::isBackgroundBlack() {
    return rbBlack->isChecked();
}

bool QG_ImageOptionsDialog::isBlackWhite() {
    return rbBlackWhite->isChecked();
}
