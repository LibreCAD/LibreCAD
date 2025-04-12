/****************************************************************************
**
* Options widget for "LineJoin" action.

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
#include "lc_linejoinoptions.h"
#include "lc_actionmodifylinejoin.h"
#include "ui_lc_linejoinoptions.h"

LC_LineJoinOptions::LC_LineJoinOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionModifyLineJoin, "Modify", "LineJoin"),
    ui(new Ui::LC_LineJoinOptions),
    m_action(nullptr){
    ui->setupUi(this);

    connect(ui->cbLine1EdgeMode, &QComboBox::currentIndexChanged,this, &LC_LineJoinOptions::onEdgeModelLine1IndexChanged);
    connect(ui->cbLine2EdgeMode, &QComboBox::currentIndexChanged,this, &LC_LineJoinOptions::onEdgeModelLine2IndexChanged);
    connect(ui->cbAttributesSource, &QComboBox::currentIndexChanged, this, &LC_LineJoinOptions::onAttributesSourceIndexChanged);

    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_LineJoinOptions::onUsePolylineClicked);
    connect(ui->cbRemoveOriginals, &QCheckBox::clicked, this, &LC_LineJoinOptions::onRemoveOriginalsClicked);
}

LC_LineJoinOptions::~LC_LineJoinOptions(){    
    delete ui;
    m_action = nullptr; 
}

void LC_LineJoinOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineJoinOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionModifyLineJoin *>(a);

    int line1EdgeMode;
    int line2EdgeMode;
    bool usePolyline;
    int  attributesSource;
    bool removeOriginals;

    if (update){
        line1EdgeMode = m_action->getLine1EdgeMode();
        line2EdgeMode = m_action->getLine2EdgeMode();
        usePolyline = m_action->isCreatePolyline();
        removeOriginals = m_action->isRemoveOriginalLines();
        attributesSource = m_action->getAttributesSource();
    }
    else{        
        usePolyline = loadBool("Polyline", false);
        removeOriginals = loadBool("RemoveOriginals", false);
        attributesSource= loadInt("AttributesSource", 0);
        line1EdgeMode = loadInt("Line1EdgeMode", 0);
        line2EdgeMode = loadInt("Line2EdgeMode", 0);
    }

    setUsePolylineToActionAndView(usePolyline);
    setRemoveOriginalsToActionAndView(removeOriginals);
    setAttributesSourceToActionAndView(attributesSource);
    setEdgeModeLine1ToActionAndView(line1EdgeMode);
    setEdgeModeLine2ToActionAndView(line2EdgeMode);
}

void LC_LineJoinOptions::doSaveSettings(){    
    save("Polyline", ui->cbPolyline->isChecked());
    save("RemoveOriginals", ui->cbRemoveOriginals->isChecked());
    save("AttributesSource", ui->cbAttributesSource->currentIndex());
    save("Line1EdgeMode", ui->cbLine1EdgeMode->currentIndex());
    save("Line2EdgeMode", ui->cbLine2EdgeMode->currentIndex());
}

void LC_LineJoinOptions::onUsePolylineClicked(bool value){
    if (m_action != nullptr){
        setUsePolylineToActionAndView(value);
    }
}

void LC_LineJoinOptions::onRemoveOriginalsClicked(bool value){
    if (m_action != nullptr){
        setRemoveOriginalsToActionAndView(value);
    }
}

#define NO_CHANGE_INDEX 2
#define EXTEND_TRIM_INDEX 0

void LC_LineJoinOptions::onEdgeModelLine1IndexChanged(int index){
    if (m_action != nullptr){
        setEdgeModeLine1ToActionAndView(index);
        if (index == NO_CHANGE_INDEX){
            ui->cbPolyline->setEnabled(false);
        }
        else{
            ui->cbPolyline->setEnabled(ui->cbLine2EdgeMode->currentIndex() != NO_CHANGE_INDEX);
        }
    }
    bool allowRemoval = index == EXTEND_TRIM_INDEX || ui->cbLine2EdgeMode->currentIndex() == EXTEND_TRIM_INDEX;
    ui->cbRemoveOriginals->setEnabled(allowRemoval);
}

void LC_LineJoinOptions::onEdgeModelLine2IndexChanged(int index){
    if (m_action != nullptr){
        setEdgeModeLine2ToActionAndView(index);
        if (index == NO_CHANGE_INDEX){
            ui->cbPolyline->setEnabled(false);
        }
        else{
            ui->cbPolyline->setEnabled(ui->cbLine1EdgeMode->currentIndex() != NO_CHANGE_INDEX);
        }
    }
    bool allowRemoval = index == EXTEND_TRIM_INDEX || ui->cbLine1EdgeMode->currentIndex() == EXTEND_TRIM_INDEX;
    ui->cbRemoveOriginals->setEnabled(allowRemoval);
}

void LC_LineJoinOptions::onAttributesSourceIndexChanged(int index){
    if (m_action != nullptr){
        setAttributesSourceToActionAndView(index);
    }
}

void LC_LineJoinOptions::setEdgeModeLine1ToActionAndView(int index){
    m_action->setLine1EdgeMode(index);
    ui->cbLine1EdgeMode->setCurrentIndex(index);
}

void LC_LineJoinOptions::setAttributesSourceToActionAndView(int index){
    m_action->setAttributesSource(index);
    ui->cbAttributesSource->setCurrentIndex(index);
}

void LC_LineJoinOptions::setEdgeModeLine2ToActionAndView(int index){
    m_action->setLine2EdgeMode(index);
    ui->cbLine2EdgeMode->setCurrentIndex(index);
}

void LC_LineJoinOptions::setUsePolylineToActionAndView(bool value){
    m_action->setCreatePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_LineJoinOptions::setRemoveOriginalsToActionAndView(bool value){
   m_action->setRemoveOriginalLines(value);
   ui->cbRemoveOriginals->setChecked(value);
}
