/* ********************************************************************************
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
 * ********************************************************************************/

#include "lc_dlgucsproperties.h"

#include <QMessageBox>

#include "lc_ucs.h"
#include "lc_ucslist.h"
#include "rs_math.h"
#include "rs_units.h"
#include "ui_lc_dlgucsproperties.h"

LC_DlgUCSProperties::LC_DlgUCSProperties(QWidget *parent)
    : LC_Dialog(parent, "UCSEdit")
    , ui(new Ui::LC_DlgUCSProperties){
    ui->setupUi(this);
}

LC_DlgUCSProperties::~LC_DlgUCSProperties(){
    delete ui;
}

void LC_DlgUCSProperties::languageChange() {
    ui->retranslateUi(this);
}

void LC_DlgUCSProperties::updateUCS() {
    QString name = ui->leName->text().trimmed();
    if (name.isEmpty()){
        QMessageBox::warning(this, tr("UCS Edit"),
                             tr("Please specify name of UCS"),
                             QMessageBox::Close,
                             QMessageBox::Close);
    }
    else if (!LC_UCS::isValidName(name)){
        QMessageBox::warning(this, tr("UCS Edit"),
                             tr("Name contains not allowed characters"),
                             QMessageBox::Close,
                             QMessageBox::Close);
    }
    else{
        LC_UCS* existingUCS = m_ucsList->find(name);
        if (existingUCS == nullptr){
            m_ucs->setName(name);
            m_ucsList->setModified(true);
        }
        else{
            if (existingUCS == m_ucs){
                // actually, do nothing... no name change
            }
            else{
                if (m_applyDuplicateSilently){

                }
            }
        }
    }
}

void LC_DlgUCSProperties::setUCS(LC_UCSList *ulist, bool applyDuplicates, [[maybe_unused]]LC_UCS* u, RS2::Unit unit, RS2::LinearFormat linearFormat, int linearPrec, RS2::AngleFormat angleFormat, int anglePrec) {

    double angleValue = RS_Math::correctAnglePlusMinusPi(m_ucs->getXAxis().angle());
    QString originX = RS_Units::formatLinear(m_ucs->getOrigin().x, unit, linearFormat, linearPrec);
    QString originY = RS_Units::formatLinear(m_ucs->getOrigin().y, unit, linearFormat, linearPrec);
    QString angle = RS_Units::formatAngle(angleValue, angleFormat, anglePrec);

    QString origin;
    origin.append(originX).append(" , "). append(originY);

    ui->lblOrigin->setText(origin);
    ui->lblAxis->setText(angle);

    QString xEnd = RS_Units::formatLinear(m_ucs->getXAxis().x, unit, linearFormat, linearPrec);
    QString yxEnd = RS_Units::formatLinear(m_ucs->getXAxis().y, unit, linearFormat, linearPrec);
    QString xAxis;
    xAxis.append(originX).append(" , "). append(originY);

    ui->lblAxisEndtpoint->setText(xAxis);

    QString gridType;
    switch (m_ucs->getOrthoType()) {
        case LC_UCS::FRONT:
        case LC_UCS::BACK:
            gridType = tr("Ortho");
            break;
        case LC_UCS::LEFT: {
            gridType = tr("Left");
            break;
        }
        case LC_UCS::RIGHT: {
            gridType = tr("Right");
            break;
        }
        case LC_UCS::TOP:
        case LC_UCS::BOTTOM: {
            gridType = tr("Top");
            break;
        }
        default:
            gridType = tr("Ortho");
    }

    ui->lblGridType->setText(gridType);

    m_ucsList = ulist;
    m_applyDuplicateSilently = applyDuplicates;

}
