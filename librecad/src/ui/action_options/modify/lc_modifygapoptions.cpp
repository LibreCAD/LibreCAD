/****************************************************************************
**
* Options widget for action that creates a gap in selected line

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
#include "lc_modifygapoptions.h"
#include "lc_actionmodifylinegap.h"
#include "ui_lc_modifygapoptions.h"

LC_ModifyGapOptions::LC_ModifyGapOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionModifyLineGap, "Modify", "LineGap"),
    ui(new Ui::LC_ModifyGapOptions) {
    ui->setupUi(this);

    connect(ui->cbFree, &QCheckBox::clicked, this, &LC_ModifyGapOptions::onFreeGapClicked);
    connect(ui->leSize, &QLineEdit::editingFinished, this, &LC_ModifyGapOptions::onSizeEditingFinished);
    connect(ui->leDistance, &QLineEdit::editingFinished, this, &LC_ModifyGapOptions::onDistanceEditingFinished);
    connect(ui->cbLineSnap, &QComboBox::currentIndexChanged, this, &LC_ModifyGapOptions::onLineSnapModeIndexChanged);
    connect(ui->cbGapSnap, &QComboBox::currentIndexChanged, this, &LC_ModifyGapOptions::onGapSnapModeIndexChanged);
}

LC_ModifyGapOptions::~LC_ModifyGapOptions(){
    delete ui;
}

void LC_ModifyGapOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionModifyLineGap *>(a);
    QString gapSize;
    bool gapFree;
    int lineSnap;
    int gapSnap;
    QString snapDistance;

    if (update){
        gapSize = fromDouble(m_action->getGapSize());
        gapFree = m_action->isFreeGapSize();
        lineSnap = m_action->getLineSnapMode();
        snapDistance = fromDouble(m_action->getSnapDistance());
        gapSnap = m_action->getGapSnapMode();
    }
    else{
        gapSize = load("GapSize", "1.0");
        gapFree = loadBool("GapFree", false);
        lineSnap = loadInt("LineSnap", 1);
        snapDistance = load("SnapDistance", "0.0");
        gapSnap = loadInt("GapSnap", 0);
    }
    setGapSizeToActionAndView(gapSize);
    setGapIsFreeToActionAndView(gapFree);
    setLineSnapToActionAndView(lineSnap);
    setSnapDistanceToActionAndView(snapDistance);
    setGapSnapToActionAndView(gapSnap);

}

void LC_ModifyGapOptions::doSaveSettings(){
    save("GapSize", ui->leSize->text());
    save("GapFree", ui->cbFree->isChecked());
    save("LineSnap", ui->cbLineSnap->currentIndex());
    save("SnapDistance", ui->leDistance->text());
    save("GapSnap", ui->cbGapSnap->currentIndex());
}

void LC_ModifyGapOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_ModifyGapOptions::onFreeGapClicked(bool val){
    if (m_action != nullptr){
        setGapIsFreeToActionAndView(val);
    }
}

void LC_ModifyGapOptions::onSizeEditingFinished(){
  if (m_action != nullptr){
      setGapSizeToActionAndView(ui->leSize->text());
  }
}

void LC_ModifyGapOptions::onDistanceEditingFinished(){
    if (m_action != nullptr){
        setSnapDistanceToActionAndView(ui->leDistance->text());
    }
}

void LC_ModifyGapOptions::onLineSnapModeIndexChanged(int index){
  if (m_action != nullptr){
      setLineSnapToActionAndView(index);
  }
}

void LC_ModifyGapOptions::onGapSnapModeIndexChanged(int index){
    if (m_action != nullptr){
        setGapSnapToActionAndView(index);
    }
}

void LC_ModifyGapOptions::setGapSizeToActionAndView(const QString &val){
    double len;
    if (toDouble(val, len, 1.0, false)){
        m_action->setGapSize(len);
        ui->leSize->setText(fromDouble(len));
    }
}

void LC_ModifyGapOptions::setGapIsFreeToActionAndView(bool val){
    m_action->setFreeGapSize(val);
    ui->cbFree->setChecked(val);

    ui->leSize->setEnabled(!val);
    ui->cbGapSnap->setEnabled(!val);
}

void LC_ModifyGapOptions::setLineSnapToActionAndView(int val){
    m_action->setLineSnapMode(val);
    ui->cbLineSnap->setCurrentIndex(val);
    ui->leDistance->setEnabled(val > 0);
}

void LC_ModifyGapOptions::setGapSnapToActionAndView(int val){
    m_action->setGapSnapMode(val);
    ui->cbGapSnap->setCurrentIndex(val);
}

void LC_ModifyGapOptions::setSnapDistanceToActionAndView(const QString &val){
    double len;
    if (toDouble(val, len, 0.0, false)){
        m_action->setSnapDistance(len);
        ui->leDistance->setText(fromDouble(len));
    }
}
