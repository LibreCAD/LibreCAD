/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2012 LibreCAD.org

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
#include "lc_circle_tangental_2_entities_radius_options_widget.h"

#include "lc_action_draw_circle_tangental_2entities_radius.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_circle_tangental_2_entities_radius_options_widget.h"

/*
 *  Constructs a QG_CircleTan2Options as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_CircleTangental2EntitiesRadiusOptionsWidget::LC_CircleTangental2EntitiesRadiusOptionsWidget()
    :m_action{nullptr}, ui(new Ui::LC_CircleTangental2EntitiesRadiusOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_CircleTangental2EntitiesRadiusOptionsWidget::onRadiusEditingFinished);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_CircleTangental2EntitiesRadiusOptionsWidget::~LC_CircleTangental2EntitiesRadiusOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_CircleTangental2EntitiesRadiusOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_CircleTangental2EntitiesRadiusOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawCircleTangental2EntitiesRadius*>(a);
    const QString radius = fromDouble(m_action->getRadius());
    LC_GuardedSignalsBlocker({ui->leRadius});
    ui->leRadius->setText(radius);
}

void LC_CircleTangental2EntitiesRadiusOptionsWidget::onRadiusEditingFinished() {
    const auto val = ui->leRadius->text();
    double radius;
    if (toDouble(val, radius, 1.0, true)) {
        m_action->setRadius(radius);
    }
    m_action->updateOptions();
}
