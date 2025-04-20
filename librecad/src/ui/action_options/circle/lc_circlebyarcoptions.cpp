/****************************************************************************
**
* Options widget for "CircleByArc" action.

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
#include "lc_circlebyarcoptions.h"
#include "ui_lc_circlebyarcoptions.h"
#include "lc_actiondrawcirclebyarc.h"

LC_CircleByArcOptions::LC_CircleByArcOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionDrawCircleByArc, "Draw","CircleByArc"),
    ui(new Ui::LC_CircleByArcOptions),
    m_action(nullptr){
    ui->setupUi(this);
    connect(ui->cbReplace, &QCheckBox::clicked, this, &LC_CircleByArcOptions::onReplaceClicked);
    connect(ui->cbPen, &QComboBox::currentIndexChanged, this, &LC_CircleByArcOptions::onPenModeIndexChanged);
    connect(ui->cbLayer, &QComboBox::currentIndexChanged, this, &LC_CircleByArcOptions::onLayerModeIndexChanged);
    connect(ui->leRadiusShift, &QLineEdit::editingFinished, this, &LC_CircleByArcOptions::onRadiusShiftEditingFinished);
}

LC_CircleByArcOptions::~LC_CircleByArcOptions(){
    delete ui;
    m_action = nullptr;
}

void LC_CircleByArcOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionDrawCircleByArc *>(a);
    bool replace;
    int penMode;
    int layerMode;
    QString radiusShift;
    if (update){
        replace = m_action->isReplaceArcByCircle();
        penMode = m_action->getPenMode();
        layerMode = m_action->getLayerMode();
        radiusShift = fromDouble(m_action->getRadiusShift());
    }
    else{
        replace = loadBool("ReplaceArc", false);
        penMode = loadInt("PenMode", 0);
        layerMode = loadInt("LayerMode", 0);
        radiusShift = load("RadiusShift", "0.0");
    }
    setReplaceArcToActionAndView(replace);
    setPenModeToActionAndView(penMode);
    setLayerModeToActionAndeView(layerMode);
    setRadiusShiftToModelAndView(radiusShift);
}

void LC_CircleByArcOptions::doSaveSettings(){
    save("ReplaceArc", ui->cbReplace->isChecked());
    save("PenMode", ui->cbPen->currentIndex());
    save("LayerMode", ui->cbLayer->currentIndex());
    save("RadiusShift", ui->leRadiusShift->text());
}

void LC_CircleByArcOptions::onPenModeIndexChanged(int mode){
    if (m_action != nullptr){
        setPenModeToActionAndView(mode);
    }
}

void LC_CircleByArcOptions::onLayerModeIndexChanged(int mode){
    if (m_action != nullptr){
        setLayerModeToActionAndeView(mode);
    }
}

void LC_CircleByArcOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_CircleByArcOptions::onReplaceClicked(bool value){
    if (m_action != nullptr){
        setReplaceArcToActionAndView(value);
    }
}

void LC_CircleByArcOptions::setReplaceArcToActionAndView(bool value){
    m_action->setReplaceArcByCircle(value);
    ui->cbReplace->setChecked(value);

    ui->leRadiusShift->setEnabled(!value);
    ui->lblRadiusShift->setEnabled(!value);
}

void LC_CircleByArcOptions::setPenModeToActionAndView(int mode){
    m_action->setPenMode(mode);
    ui->cbPen->setCurrentIndex(mode);
}

void LC_CircleByArcOptions::setLayerModeToActionAndeView(int mode){
    m_action->setLayerMode(mode);
    ui->cbLayer->setCurrentIndex(mode);
}

void LC_CircleByArcOptions::onRadiusShiftEditingFinished(){
  if (m_action != nullptr){
      setRadiusShiftToModelAndView(ui->leRadiusShift->text());
  }
}

void LC_CircleByArcOptions::setRadiusShiftToModelAndView(QString val){
    double len;
    if (toDouble(val, len, 0.0, false)){
        m_action->setRadiusShift(len);
        ui->leRadiusShift->setText(fromDouble(len));
    }
}
