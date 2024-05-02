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
#include "qg_linerelangleoptions.h"

#include "rs_actiondrawlinerelangle.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "ui_qg_linerelangleoptions.h"

namespace {
constexpr char* g_angleKey = "/LineRelAngleAngle";
constexpr char* g_lengthKey = "/LineRelAngleLength";

// format a number with specified digits after point
QString formatNumber(double value, int precision = 8)
{
    precision = std::max(precision, 0);
    precision = std::min(precision, 16);
    LC_ERR<<"value: "<<value;
    QString text = QString("%1").arg(value, 0, 'f', precision);
    LC_ERR<<"value: "<<text;
    text = RS_Dimension::stripZerosLinear(text, 12);
    LC_ERR<<"value: "<<text;
    return text;
}
}

/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineRelAngleOptions::QG_LineRelAngleOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(std::make_unique<Ui::Ui_LineRelAngleOptions>())
{
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineRelAngleOptions::~QG_LineRelAngleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineRelAngleOptions::languageChange()
{
    ui->retranslateUi(this);
}

void QG_LineRelAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a &&
            ( a->rtti()==RS2::ActionDrawLineRelAngle
              ||
              a->rtti()==RS2::ActionDrawLineOrthogonal )
            ) {
        action = static_cast<RS_ActionDrawLineRelAngle*>(a);
        if (action->hasFixedAngle()) {
            ui->lAngle->hide();
            ui->leAngle->hide();
        }else{
            ui->lAngle->show();
            ui->leAngle->show();
        }

        QString sa;
        QString sl;

        // settings from action:
        if (update) {
            sa = formatNumber(RS_Math::rad2deg(action->getAngle()));
            sl = formatNumber(action->getLength());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry(g_angleKey, "30.0");
                sa = RS_Dimension::stripZerosLinear(sa, 12);
            } else {
                sa = formatNumber(RS_Math::rad2deg(action->getAngle()));
            }
            sl = RS_SETTINGS->readEntry(g_lengthKey, "10.0");
            sl = RS_Dimension::stripZerosLinear(sl, 12);
            RS_SETTINGS->endGroup();
        }

        ui->leAngle->setText(sa);
        ui->leLength->setText(sl);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LineRelAngleOptions::setAction: wrong action type");
        action = nullptr;
    }
}

void QG_LineRelAngleOptions::saveSettings() {
    if (action) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!action->hasFixedAngle()) {
            QString angle = formatNumber(RS_Math::rad2deg(action->getAngle()));
            RS_SETTINGS->writeEntry(g_angleKey, angle);
            LC_ERR<<__LINE__<<": sa= "<<angle;
        }
        if (action->getLength() > RS_TOLERANCE) {
            QString length = formatNumber(action->getLength());
            RS_SETTINGS->writeEntry(g_lengthKey, length);
        }
        RS_SETTINGS->endGroup();
    }
}

void QG_LineRelAngleOptions::updateAngle(const QString& a) {
    if (action && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
        saveSettings();
    }
}

void QG_LineRelAngleOptions::updateLength(const QString& l) {
    if (action) {
        bool okay = false;
        double length = RS_Math::eval(l, &okay);
        if (okay && length > RS_TOLERANCE) {
            action->setLength(length);
            saveSettings();
        }
    }
}
