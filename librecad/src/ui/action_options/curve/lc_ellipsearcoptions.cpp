/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#include "lc_ellipsearcoptions.h"
#include "ui_lc_ellipsearcoptions.h"


/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_EllipseArcOptions::LC_EllipseArcOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawEllipseArcAxis, "Draw","EllipseArc")
    , ui(std::make_unique<Ui::LC_EllipseArcOptions>()){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_EllipseArcOptions::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &LC_EllipseArcOptions::onDirectionChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_EllipseArcOptions::~LC_EllipseArcOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_EllipseArcOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_EllipseArcOptions::doSaveSettings(){
    save("Reversed",  ui->rbNeg->isChecked());
}

void LC_EllipseArcOptions::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionDrawEllipseAxis *>(a);

    bool reversed;
    if (update){
        reversed = action->isReversed();
    } else {
        reversed = loadBool("Reversed", false);
        action->setReversed(reversed);
    }
    setReversedToActionAndView(reversed);
}

void LC_EllipseArcOptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    action->setReversed(reversed);
}

/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/
void LC_EllipseArcOptions::onDirectionChanged(bool /*pos*/){
    setReversedToActionAndView( ui->rbNeg->isChecked());
}
