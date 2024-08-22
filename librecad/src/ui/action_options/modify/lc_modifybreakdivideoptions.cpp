/****************************************************************************
**
* Options widget for ModifyBreakDivide action that breaks line, arc or circle
* to segments by points of intersection with other entities.

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
#include "lc_modifybreakdivideoptions.h"
#include "ui_lc_modifybreakdivideoptions.h"

LC_ModifyBreakDivideOptions::LC_ModifyBreakDivideOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionModifyBreakDivide, "Modify", "BreakDivide"),
    ui(new Ui::LC_ModifyBreakDivideOptions)
{
    ui->setupUi(this);
    connect(ui->cbRemoveSegments, SIGNAL(clicked(bool)), this, SLOT(onRemoveSegmentsClicked(bool)));
    connect(ui->cbRemoveSelected, SIGNAL(clicked(bool)), this, SLOT(onRemoveSelectedClicked(bool)));
}

LC_ModifyBreakDivideOptions::~LC_ModifyBreakDivideOptions(){
    delete ui;
}

void LC_ModifyBreakDivideOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionModifyBreakDivide *>(a);
    bool removeSegments;
    bool removeSelected;

    if (update){
        removeSelected = action->isRemoveSelected();
        removeSegments = action->isRemoveSegment();
    }
    else{
        removeSegments = loadBool("RemoveSegments", true);
        removeSelected = loadBool("RemoveSelected", true);
    }
    setRemoveSegmentsToActionAndView(removeSegments);
    setRemoveSelectedToActionAndView(removeSelected);
}

void LC_ModifyBreakDivideOptions::doSaveSettings(){
    save("RemoveSegments", ui->cbRemoveSegments->isChecked());
    save("RemoveSelected", ui->cbRemoveSelected->isChecked());
}

void LC_ModifyBreakDivideOptions::onRemoveSegmentsClicked(bool clicked){
    if (action != nullptr){
        setRemoveSegmentsToActionAndView(clicked);
    }
}

void LC_ModifyBreakDivideOptions::onRemoveSelectedClicked(bool clicked){
    if (action != nullptr){
        setRemoveSelectedToActionAndView(clicked);
    }
}

void LC_ModifyBreakDivideOptions::setRemoveSegmentsToActionAndView(bool val){
   action->setRemoveSegment(val);
   ui->cbRemoveSegments->setChecked(val);
   ui->cbRemoveSelected->setEnabled(val);
}

void LC_ModifyBreakDivideOptions::setRemoveSelectedToActionAndView(bool val){
     action->setRemoveSelected(val);
     ui->cbRemoveSelected->setChecked(val);
}

void LC_ModifyBreakDivideOptions::languageChange(){
    ui->retranslateUi(this);
}
