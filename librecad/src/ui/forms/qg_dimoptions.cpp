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
#include "qg_dimoptions.h"

#include "rs_settings.h"
#include "rs_debug.h"
#include "ui_qg_dimoptions.h"
#include "rs_actiondimension.h"

/*
 *  Constructs a QG_DimOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimOptions::QG_DimOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(std::make_unique<Ui::Ui_DimOptions>())
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimOptions::~QG_DimOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_DimOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/DimLabel", ui->leLabel->text());
	RS_SETTINGS->writeEntry("/DimTol1", ui->leTol1->text());
	RS_SETTINGS->writeEntry("/DimTol2", ui->leTol2->text());
    if (action != nullptr && action->rtti() == RS2::ActionDimRadial)
        RS_SETTINGS->writeEntry("/DimRadial", ui->bDiameter->isChecked());
    if (action != nullptr)
        RS_SETTINGS->writeEntry("/DimDiameter", ui->bDiameter->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DimOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && RS_ActionDimension::isDimensionAction(a->rtti())) {
		action = static_cast<RS_ActionDimension*>(a);

        QString st;
        QString stol1;
        QString stol2;
        bool diam = false;
        bool radial = false;
        if (update) {
            st = action->getLabel();
            stol1 = action->getTol1();
            stol2 = action->getTol2();
            diam = action->getDiameter();
            ui->bDiameter->setChecked(action->getDiameter());
        } else {
            //st = "";
            RS_SETTINGS->beginGroup("/Draw");
            st = RS_SETTINGS->readEntry("/DimLabel", "");
            stol1 = RS_SETTINGS->readEntry("/DimTol1", "");
            stol2 = RS_SETTINGS->readEntry("/DimTol2", "");
            diam = (bool)RS_SETTINGS->readNumEntry("/DimDiameter", 0);
            radial = (bool)RS_SETTINGS->readNumEntry("/DimRadial", 0);
            RS_SETTINGS->endGroup();
        }
        if (action != nullptr && action->rtti() == RS2::ActionDimRadial) {
            ui->bDiameter->setIcon({});
            ui->bDiameter->setText(tr("R", "Radial dimension prefix"));
            ui->bDiameter->setChecked(radial);
            action->setDiameter(radial);
        } else {
            ui->bDiameter->setChecked(diam);
        }
        ui->leLabel->setText(st);
		ui->leTol1->setText(stol1);
		ui->leTol2->setText(stol2);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_DimensionOptions::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_DimOptions::updateLabel() {
    if (action) {
        action->setText("");
		action->setLabel(ui->leLabel->text());
		action->setDiameter(ui->bDiameter->isChecked());
		action->setTol1(ui->leTol1->text());
		action->setTol2(ui->leTol2->text());

        action->setText(action->getText());
        saveSettings();
  }
}

void QG_DimOptions::insertSign(const QString& c) {
	ui->leLabel->insert(c);
}


