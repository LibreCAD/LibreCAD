/****************************************************************************
**
* Options widget for "DrawCross" action.

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
#include "lc_crossoptions.h"
#include "lc_actiondrawcross.h"
#include "ui_lc_crossoptions.h"

LC_CrossOptions::LC_CrossOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionDrawCross, "Draw", "Cross"),
    ui(new Ui::LC_CrossOptions),
    m_action(nullptr) {
    ui->setupUi(this);

    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_CrossOptions::onXEditingFinished);
    connect(ui->leY, &QLineEdit::editingFinished, this, &LC_CrossOptions::onYEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_CrossOptions::onAngleEditingFinished);
    connect(ui->cbMode, &QComboBox::currentIndexChanged, this, &LC_CrossOptions::onModeIndexChanged);
}

LC_CrossOptions::~LC_CrossOptions() {
    delete ui;
    m_action = nullptr;
}

void LC_CrossOptions::doSetAction(RS_ActionInterface* a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawCross*>(a);

    QString x;
    QString y;
    QString angle;
    int mode;
    if (update) {
        x = fromDouble(m_action->getLenX());
        y = fromDouble(m_action->getLenY());
        angle = fromDouble(m_action->getCrossAngle());
        mode = m_action->getCrossMode();
    }
    else {
        x = load("X", "1.0");
        y = load("Y", "1.0");
        angle = load("Angle", "0.0");
        mode = loadInt("Mode", 1);
    }
    setXToActionAndView(x);
    setYToActionAndView(y);
    setAngleToActionAndView(angle);
    setModeToActionAndView(mode);
}

void LC_CrossOptions::doSaveSettings() {
    save("X", ui->leX->text());
    save("Y", ui->leY->text());
    save("Angle", ui->leAngle->text());
    save("Mode", ui->cbMode->currentIndex());
}

void LC_CrossOptions::onXEditingFinished() {
    const QString& expr = ui->leX->text();
    setXToActionAndView(expr);
}

void LC_CrossOptions::onYEditingFinished() {
    const QString& expr = ui->leY->text();
    setYToActionAndView(expr);
}

void LC_CrossOptions::onAngleEditingFinished() {
    const QString& expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_CrossOptions::onModeIndexChanged(int index) {
    setModeToActionAndView(index);
}

void LC_CrossOptions::setXToActionAndView(const QString& strValue) {
    double x;
    if (toDouble(strValue, x, 1.0, true)) {
        m_action->setXLength(x);
        ui->leX->setText(fromDouble(x));
    }
}

void LC_CrossOptions::setYToActionAndView(const QString& strValue) {
    double y;
    if (toDouble(strValue, y, 1.0, true)) {
        m_action->setYLength(y);
        ui->leY->setText(fromDouble(y));
    }
}

void LC_CrossOptions::setAngleToActionAndView(const QString& expr) {
    double angle;
    if (toDoubleAngleDegrees(expr, angle, 0.0, false)) {
        m_action->setCrossAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_CrossOptions::setModeToActionAndView(int mode) {
    m_action->setCrossMode(mode);
    ui->cbMode->setCurrentIndex(mode);
}

void LC_CrossOptions::languageChange() {
    ui->retranslateUi(this);
}
