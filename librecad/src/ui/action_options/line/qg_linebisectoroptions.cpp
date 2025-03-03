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
#include "qg_linebisectoroptions.h"

#include "rs_actiondrawlinebisector.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "ui_qg_linebisectoroptions.h"

/*
 *  Constructs a QG_LineBisectorOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineBisectorOptions::QG_LineBisectorOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawLineBisector, "Draw", "LineBisector")
	, ui(new Ui::Ui_LineBisectorOptions{}){
    ui->setupUi(this);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &QG_LineBisectorOptions::onLengthEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &QG_LineBisectorOptions::onNumberValueChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineBisectorOptions::~QG_LineBisectorOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineBisectorOptions::languageChange(){
	ui->retranslateUi(this);
}

void QG_LineBisectorOptions::doSaveSettings(){
    save("Length", ui->leLength->text());
    save("Number", ui->sbNumber->text());
}

void QG_LineBisectorOptions::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionDrawLineBisector *>(a);

    QString length;
    int number;
    if (update){
        length = fromDouble(action->getLength());
        number = action->getNumber();
    } else {
        length = load("Length", "1.0");
        number = loadInt("Number", 1);
    }
    setLengthToActionAndView(length);
    setNumberToActionAndView(number);
}

void QG_LineBisectorOptions::onNumberValueChanged(int number){
    setNumberToActionAndView(number);
}

void QG_LineBisectorOptions::onLengthEditingFinished(){
    setLengthToActionAndView(ui->leLength->text());
}

void QG_LineBisectorOptions::setLengthToActionAndView(QString val){
    double len;
    if (toDouble(val, len, 1.0, false)){
        action->setLength(len);
        ui->leLength->setText(fromDouble(len));
    }
}

void QG_LineBisectorOptions::setNumberToActionAndView(int number){
    action->setNumber(number);
    ui->sbNumber->setValue(number);
}
