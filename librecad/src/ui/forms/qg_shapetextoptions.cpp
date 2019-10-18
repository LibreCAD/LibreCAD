/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Doug Geiger (noneyabiz@mail.wasent.cz)
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
#include "qg_shapetextoptions.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_actionmodifyshapetext.h"
#include "ui_qg_shapeTextoptions.h"

/*
 *  Constructs a QG_shapeTextOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ShapeTextOptions::QG_ShapeTextOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_ShapeTextOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ShapeTextOptions::~QG_ShapeTextOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ShapeTextOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_ShapeTextOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Modify");
	RS_SETTINGS->writeEntry("/ShapeTextOffset", ui->leOffset->text());
	RS_SETTINGS->writeEntry("/ShapeTextReversed", (int)ui->cbReversed->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_ShapeTextOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && ( a->rtti()==RS2::ActionModifyShapeText ) ) {
        action = static_cast<RS_ActionModifyShapeText*>(a);

        QString sr;
		bool reversed;
        if (update) {
            sr = QString("%1").arg(action->getOffset());
			reversed = action->getReversed();
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sr = RS_SETTINGS->readEntry("/ShapeTextOffset", "0.5");
			reversed = RS_SETTINGS->readNumEntry("/ShapeTextReversed", 0);
            RS_SETTINGS->endGroup();
            action->setOffset(sr.toDouble());
			action->setReversed(reversed);
        }
		ui->leOffset->setText(sr);
		ui->cbReversed->setChecked(reversed);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_ShapeTextOptions::setAction: wrong action type");
		action = nullptr;
    }

}



void QG_ShapeTextOptions::updateOffset(const QString& r) {
    if (action) {
        action->setOffset(RS_Math::eval(r));
    }
}

void QG_ShapeTextOptions::updateReversed(const bool reversed) {
	if (action) {
		action->setReversed(reversed);
	}
}
