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
#include "qg_libraryinsertoptions.h"

#include "rs_actioninterface.h"
#include "rs_actionlibraryinsert.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_libraryinsertoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_LibraryInsertOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LibraryInsertOptions::QG_LibraryInsertOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionLibraryInsert, "LibraryInsert", "LibraryInsert")
	, ui(new Ui::Ui_LibraryInsertOptions{}){
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_LibraryInsertOptions::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &QG_LibraryInsertOptions::onFactorEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LibraryInsertOptions::~QG_LibraryInsertOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LibraryInsertOptions::languageChange(){
	ui->retranslateUi(this);
}

void QG_LibraryInsertOptions::doSaveSettings() {
	save("Angle", ui->leAngle->text());
	save("Factor", ui->leFactor->text());
}

void QG_LibraryInsertOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionLibraryInsert*>(a);

    QString angle;
    QString factor;
    if (update) {
        angle = fromDouble(RS_Math::rad2deg(action->getAngle()));
        factor = fromDouble(action->getFactor());
    } else {
        angle = load("Angle", "0.0");
        factor = load("Factor", "1.0");
    }
    setAngleToActionAndView(angle);
    setFactorToActionAndView(factor);
}

void QG_LibraryInsertOptions::setAngleToActionAndView(QString val) {
    ui->leAngle->setText(val);
    action->setAngle(RS_Math::deg2rad(RS_Math::eval(val)));
}

void QG_LibraryInsertOptions::setFactorToActionAndView(QString val) {
    ui->leFactor->setText(val);
    action->setFactor(RS_Math::eval(val));
}

void QG_LibraryInsertOptions::onAngleEditingFinished() {
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_LibraryInsertOptions::onFactorEditingFinished() {
    setFactorToActionAndView(ui->leFactor->text());
}
