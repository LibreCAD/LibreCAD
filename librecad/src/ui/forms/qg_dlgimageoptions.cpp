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
#include "qg_dlgimageoptions.h"

#include "rs_math.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_ImageOptionsDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_ImageOptionsDialog::QG_ImageOptionsDialog(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ImageOptionsDialog::~QG_ImageOptionsDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ImageOptionsDialog::languageChange()
{
    retranslateUi(this);
}

void QG_ImageOptionsDialog::init() {
    graphicSize = RS_Vector(0.0,0.0);
    updateEnabled = true;

    RS_SETTINGS->beginGroup("/ExportImage");
    leWidth->setText(RS_SETTINGS->readEntry("/Width", "640"));
    leHeight->setText(RS_SETTINGS->readEntry("/Height", "480"));
    if (RS_SETTINGS->readEntry("/BlackBackground", "0")=="1") {
        rbBlack->setChecked(true);
    }
    if (RS_SETTINGS->readEntry("/Blackwhite", "0")=="1") {
        rbBlackWhite->setChecked(true);
    }
    RS_SETTINGS->endGroup();
}

void QG_ImageOptionsDialog::setGraphicSize(const RS_Vector& s) {
    graphicSize = s;
}

void QG_ImageOptionsDialog::ok() {
    RS_SETTINGS->beginGroup("/ExportImage");
    RS_SETTINGS->writeEntry("/Width", leWidth->text());
    RS_SETTINGS->writeEntry("/Height", leHeight->text());
    RS_SETTINGS->writeEntry("/BlackBackground", (int)rbBlack->isChecked());
    RS_SETTINGS->writeEntry("/Blackwhite", (int)rbBlackWhite->isChecked());
    RS_SETTINGS->endGroup();

    accept();
}

void QG_ImageOptionsDialog::sizeChanged() {
    if (updateEnabled) {
    updateEnabled = false;
    cbResolution->setItemText(cbResolution->currentIndex(), "auto");
    updateEnabled = true;
    }
}

void  QG_ImageOptionsDialog::resolutionChanged() {
    if (updateEnabled) {
    updateEnabled = false;
    bool ok = false;
    double res = RS_Math::eval(cbResolution->currentText(), &ok);
    if (!ok) {
        res = 1.0;
    }
    int w = RS_Math::round(res * graphicSize.x);
    int h = RS_Math::round(res * graphicSize.y);
    leWidth->setText(QString("%1").arg(w));
    leHeight->setText(QString("%1").arg(h));
    updateEnabled = true;
    }
}

QSize QG_ImageOptionsDialog::getSize() {
    return QSize(RS_Math::round(RS_Math::eval(leWidth->text())),
                 RS_Math::round(RS_Math::eval(leHeight->text())));
}

bool QG_ImageOptionsDialog::isBackgroundBlack() {
    return rbBlack->isChecked();
}

bool QG_ImageOptionsDialog::isBlackWhite() {
    return rbBlackWhite->isChecked();
}
