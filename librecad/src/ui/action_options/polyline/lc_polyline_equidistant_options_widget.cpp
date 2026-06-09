/****************************************************************************
**
  * Create option widget used to draw equidistant polylines

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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
**********************************************************************/
#include "lc_polyline_equidistant_options_widget.h"

#include "lc_action_polyline_equidistant.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_polyline_equidistant_options_widget.h"

/*
  * Create option widget used to draw equidistant polylines
  *
  *@Author Dongxu Li
 */
LC_PolylineEquidistantOptionsWidget::LC_PolylineEquidistantOptionsWidget(): ui{new Ui::LC_PolylineEquidistantOptionsWidget{}} {
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &LC_PolylineEquidistantOptionsWidget::onDistEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_PolylineEquidistantOptionsWidget::onNumberValueChanged);
    pickDistanceSetup("spacing", ui->tbPickSpacing, ui->leDist);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_PolylineEquidistantOptionsWidget::~LC_PolylineEquidistantOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_PolylineEquidistantOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_PolylineEquidistantOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionPolylineEquidistant*>(a);

    const QString distance = fromDouble(m_action->getDistance());
    const int number = m_action->getCopiesNumber();

    LC_GuardedSignalsBlocker({ui->leDist, ui->sbNumber});

    ui->leDist->setText(distance);
    ui->sbNumber->setValue(number);
}

void LC_PolylineEquidistantOptionsWidget::onNumberValueChanged(const int number) {
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}

void LC_PolylineEquidistantOptionsWidget::onDistEditingFinished() {
    const auto strVal = ui->leDist->text();
    double val;
    if (toDouble(strVal, val, 10.0, false)) {
        m_action->setDistance(val);
    }
    m_action->updateOptions();
}
