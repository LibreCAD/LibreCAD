/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include <QMouseEvent>
#include "lc_anglesbasiswidget.h"
#include "ui_lc_anglesbasiswidget.h"
#include "rs_graphic.h"

LC_AnglesBasisWidget::LC_AnglesBasisWidget(QWidget *parent,const char* name)
    : QWidget(parent)
    , ui(new Ui::LC_AnglesBasisWidget){
    setObjectName(name);
    ui->setupUi(this);
    iconClockwise = QIcon(":/icons/dirneg_plus.lci");
    iconCounterClockwise = QIcon(":/icons/dirpos_plus.lci");
}

LC_AnglesBasisWidget::~LC_AnglesBasisWidget(){
    delete ui;
}

void LC_AnglesBasisWidget::mouseReleaseEvent(QMouseEvent *e) {
    if(e->button()==Qt::LeftButton){
        emit clicked();
    }
    QWidget::mouseReleaseEvent(e);
}

void LC_AnglesBasisWidget::update(RS_Graphic *graphic) {
    if (graphic != nullptr) {
        double baseAngle = graphic->getAnglesBase();
        QString angleStr = graphic->formatAngle(baseAngle);
        update(angleStr, graphic->areAnglesCounterClockWise());
    }
    else{
        update("", false);
    }
}

void LC_AnglesBasisWidget::update(QString angle, bool counterclockwise) {
    QIcon icon;
    QString tooltip;
    if (counterclockwise){
        icon = iconCounterClockwise;
        tooltip = tr("Positive angles direction is counterclockwise.");
    }
    else{
        icon = iconClockwise;
        tooltip = tr("Positive angles direction is clockwise.");
    }
    this->counterclockwise = counterclockwise;
    ui->lblBaseAngle->setText(angle);
    ui->lblPositiveDirection->setPixmap(icon.pixmap(iconSize));
    ui->lblPositiveDirection->setToolTip(tooltip);
}

void LC_AnglesBasisWidget::onIconsRefreshed(){
    ui->lblPositiveDirection->setPixmap(counterclockwise ? iconCounterClockwise.pixmap(iconSize) : iconClockwise.pixmap(iconSize));
}
