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
#include "qg_roundoptions.h"

#include "rs_actionmodifyround.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_roundoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_RoundOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_RoundOptions::QG_RoundOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyRound, "Modify", "Round")
	, ui(new Ui::Ui_RoundOptions{}){
	ui->setupUi(this);
 connect(ui->leRadius, &QLineEdit::editingFinished, this, &QG_RoundOptions::onRadiusEditingFinished);
 connect(ui->cbTrim, &QCheckBox::toggled, this, &QG_RoundOptions::onTrimToggled);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_RoundOptions::~QG_RoundOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_RoundOptions::languageChange(){
	ui->retranslateUi(this);
}

void QG_RoundOptions::doSaveSettings(){
    save("Radius", ui->leRadius->text());
    save("Trim", ui->cbTrim->isChecked());
}

void QG_RoundOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyRound *>(a);

    QString radius;
    bool trim;
    if (update){
        radius = fromDouble(action->getRadius());
        trim = action->isTrimOn();
    } else {
        radius = load("Radius", "1.0");
        trim = loadBool("Trim", true);
    }
    setTrimToActionAndView(trim);
    setRadiusToActionAndView(radius);
}

void QG_RoundOptions::onRadiusEditingFinished(){
    setRadiusToActionAndView(ui->leRadius->text());
}

void QG_RoundOptions::onTrimToggled(bool checked){
    setTrimToActionAndView(checked);
}

void QG_RoundOptions::setTrimToActionAndView(bool checked){
    ui->cbTrim->setChecked(checked);
    action->setTrim(checked);
}

void QG_RoundOptions::setRadiusToActionAndView(const QString &strValue){
    double radius;
    if (toDouble(strValue, radius, 1.0, false)){
        action->setRadius(radius);
        ui->leRadius->setText(fromDouble(radius));
    }
}
