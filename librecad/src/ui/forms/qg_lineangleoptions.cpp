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
#include "qg_lineangleoptions.h"

#include "rs_actiondrawlineangle.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "ui_qg_lineangleoptions.h"

namespace {
// format a number with specified digits after point
QString formatNumber(double value, int precision = 8)
{
    precision = std::max(precision, 0);
    precision = std::min(precision, 16);
    QString text = QString("%1").arg(value, 0, 'f', precision);
    RS_Dimension::stripZerosLinear(text, 12);
    return text;
}
}

/*
 *  Constructs a QG_LineAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineAngleOptions::QG_LineAngleOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(std::make_unique<Ui::Ui_LineAngleOptions>())
{
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineAngleOptions::~QG_LineAngleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineAngleOptions::languageChange()
{
    ui->retranslateUi(this);
}

void QG_LineAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && (
                a->rtti()==RS2::ActionDrawLineAngle
                ||a->rtti()==RS2::ActionDrawLineHorizontal
                ||a->rtti()==RS2::ActionDrawLineVertical
                )
            ){
        action = static_cast<RS_ActionDrawLineAngle*>(a);
        m_bFixedAngle=action->hasFixedAngle();
        ui->leLength->show();
        ui->lLength->show();
        ui->leAngle->setVisible(!action->hasFixedAngle());
        ui->lAngle->setVisible(!action->hasFixedAngle());

        QString sa;
        QString sl;

        // settings from action:
        if (update && action->getLength() > RS_TOLERANCE) {
            if (!action->hasFixedAngle())
                sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sl = QString("%1").arg(action->getLength());
            m_snapPoint = action->getSnapPoint();
        } else {
            // settings from config file:
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry("/LineAngleAngle", "30.0");
            } else {
                sa = formatNumber(RS_Math::rad2deg(action->getAngle()));
            }
            sl = RS_SETTINGS->readEntry("/LineAngleLength", "10.0");
            m_snapPoint = RS_SETTINGS->readNumEntry("/LineAngleSnapPoint", 0);
            RS_SETTINGS->endGroup();
            action->setSnapPoint(m_snapPoint);
        }

        ui->leAngle->setText(sa);
        ui->leLength->setText(sl);
        ui->cbSnapPoint->setCurrentIndex(m_snapPoint);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LineAngleOptions::setAction: wrong action type");
        action = nullptr;
    }
}

/** fixme, action could be deleted already, moved the saving into the action
  class
  need to implement in shared_ptr*/
void QG_LineAngleOptions::saveSettings() {
    if (action != nullptr) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!m_bFixedAngle) {
            QString angle = formatNumber(RS_Math::rad2deg(action->getAngle()));
            RS_SETTINGS->writeEntry("/LineAngleAngle", angle);
        }
        if (action->getLength() > RS_TOLERANCE) {
            QString length = formatNumber(action->getLength());
            RS_SETTINGS->writeEntry("/LineAngleLength", length);
        }
        RS_SETTINGS->writeEntry("/LineAngleSnapPoint", m_snapPoint);
        RS_SETTINGS->endGroup();
    }
}

void QG_LineAngleOptions::updateAngle(const QString& a) {
    if (action != nullptr && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
        saveSettings();
    }
}

void QG_LineAngleOptions::updateLength(const QString& l) {
    if (action != nullptr) {
        bool ok = false;
        double length=RS_Math::eval(l,&ok);
        if(ok)
            action->setLength(length);
        saveSettings();
    }
}

void QG_LineAngleOptions::updateSnapPoint(int sp) {
    if (action != nullptr) {
        action->setSnapPoint(sp);
        saveSettings();
    }
}
