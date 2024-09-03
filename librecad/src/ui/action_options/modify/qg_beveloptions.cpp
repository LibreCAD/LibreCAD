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
#include "qg_beveloptions.h"
#include "rs_actionmodifybevel.h"

#include "ui_qg_beveloptions.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_BevelOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_BevelOptions::QG_BevelOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionModifyBevel,"Modify", "Bevel"),
    ui(std::make_unique<Ui::Ui_BevelOptions>()){
    ui->setupUi(this);
    connect(ui->cbTrim, &QCheckBox::toggled, this, &QG_BevelOptions::onTrimToggled);
    connect(ui->leLength1, &QLineEdit::editingFinished, this, &QG_BevelOptions::onLength1EditingFinished);
    connect(ui->leLength2, &QLineEdit::editingFinished, this, &QG_BevelOptions::onLength2EditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_BevelOptions::~QG_BevelOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_BevelOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_BevelOptions::doSaveSettings(){
    save("Length1", ui->leLength1->text());
    save("Length2", ui->leLength2->text());
    save("Trim", (int) ui->cbTrim->isChecked());
}

void QG_BevelOptions::doSetAction(RS_ActionInterface *a, bool update){
        action = dynamic_cast<RS_ActionModifyBevel *>(a);

        QString len1;
        QString len2;
        bool trim;
        if (update){
            len1 = fromDouble(action->getLength1());
            len2 = fromDouble(action->getLength2());
            trim = action->isTrimOn();
        } else {
            len1 = load("Length1", "1.0");
            len2 = load("Length2", "1.0");
            trim = loadBool("Trim", "1");
        }
        setLength1ToActionAndView(len1);
        setLength2ToActionAndView(len2);
        setTrimToActionAndView(trim);
}

void QG_BevelOptions::onTrimToggled(bool checked){
    setTrimToActionAndView(checked);
}

void QG_BevelOptions::onLength1EditingFinished(){
    setLength1ToActionAndView(ui->leLength1->text());
}

void QG_BevelOptions::onLength2EditingFinished(){
    setLength2ToActionAndView(ui->leLength2->text());
}

void QG_BevelOptions::setLength1ToActionAndView(QString val){
    if (action != nullptr){
        double len;
        if (toDouble(val, len, 1.0, false)){ // fixme - check whether negative values are allowed
            action->setLength1(len);
            ui->leLength1->setText(fromDouble(len));
        }
    }
}

void QG_BevelOptions::setLength2ToActionAndView(QString val){
    if (action != nullptr){
        double len;
        if (toDouble(val, len, 1.0, false)){ // fixme - check whether negative values are allowed
            action->setLength2(len);
            ui->leLength2->setText(fromDouble(len));
        }
    }
}

void QG_BevelOptions::setTrimToActionAndView(bool val){
    action->setTrim(val);
    ui->cbTrim->setChecked(val);
}
