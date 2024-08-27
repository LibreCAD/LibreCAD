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
#include "qg_arcoptions.h"

#include "rs_actiondrawarc.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include "ui_qg_arcoptions.h"

/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ArcOptions::QG_ArcOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawArc, "Draw","Arc")
    , ui(std::make_unique<Ui::Ui_ArcOptions>()){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &QG_ArcOptions::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &QG_ArcOptions::onDirectionChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ArcOptions::~QG_ArcOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ArcOptions::languageChange(){
	ui->retranslateUi(this);
}

void QG_ArcOptions::doSaveSettings(){
    save("Reversed",  ui->rbNeg->isChecked());
}

void QG_ArcOptions::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionDrawArc *>(a);

    bool reversed;
    if (update){
        reversed = action->isReversed();
    } else {
        reversed = loadBool("Reversed", false);
        action->setReversed(reversed);
    }
    setReversedToActionAndView(reversed);
}

void QG_ArcOptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    action->setReversed(reversed);
}

/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/
void QG_ArcOptions::onDirectionChanged(bool /*pos*/){
    setReversedToActionAndView( ui->rbNeg->isChecked());
}
