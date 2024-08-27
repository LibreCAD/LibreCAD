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
QG_ImageOptionsDialog::QG_ImageOptionsDialog(QWidget* parent)
    : LC_Dialog(parent, "ImageOptions"){
    setupUi(this);
    init();
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
    updateEnabled = false;
    useResolution = true;

    LC_GROUP_GUARD("Export");
    {
        if (LC_GET_BOOL("UseResolution", true)) {
            cbResolution->setCurrentIndex(cbResolution->findText(QString("%1").arg(LC_GET_STR("Resolution", "1"))));
        } else {
            leWidth->setText(LC_GET_STR("Width", "640"));
            leHeight->setText(LC_GET_STR("Height", "480"));
        }
        if (LC_GET_BOOL("BlackBackground")) {
            rbBlack->setChecked(true);
            rbWhite->setChecked(false);
        } else {
            rbBlack->setChecked(false);
            rbWhite->setChecked(true);
        }
        if (LC_GET_BOOL("BlackWhite", true)) {
            rbBlackWhite->setChecked(true);
            rbColoured->setChecked(false);
        } else {
            rbBlackWhite->setChecked(false);
            rbColoured->setChecked(true);
        }
        leLeftRight->setText(LC_GET_STR("BorderLeftRight", "5"));
        leTopBottom->setText(LC_GET_STR("BorderTopBottom", "5"));
        if (LC_GET_STR("BorderSameSize", "1") == "1") {
            cbSameBorders->setChecked(true);
            sameBordersChanged();
        }
    }

    updateEnabled = true;
}

void QG_ImageOptionsDialog::setGraphicSize(const RS_Vector& s) {
    graphicSize = s;
    if(!useResolution){
        sizeChanged();
    }
    else {
        resolutionChanged();
    }
}

void QG_ImageOptionsDialog::ok() {
    LC_GROUP_GUARD("Export");
    {
        LC_SET("UseResolution", useResolution);
        LC_SET("Resolution", cbResolution->currentText());
        LC_SET("Width", leWidth->text());
        LC_SET("Height", leHeight->text());
        LC_SET("BorderLeftRight", leLeftRight->text());
        LC_SET("BorderTopBottom", leTopBottom->text());
        LC_SET("BorderSameSize", cbSameBorders->isChecked());
        LC_SET("BlackBackground", rbBlack->isChecked());
        LC_SET("BlackWhite", rbBlackWhite->isChecked());
    }
    accept();
}

void QG_ImageOptionsDialog::sameBordersChanged() {
    if(cbSameBorders->isChecked()) {
        leTopBottom->setText(leLeftRight->text());
        leTopBottom->setDisabled(true);
    }
    else {
        leTopBottom->setEnabled(true);
    }
}

void QG_ImageOptionsDialog::borderChanged() {
    if(cbSameBorders->isChecked()) {
        leTopBottom->setText(leLeftRight->text());
    }
}

void QG_ImageOptionsDialog::sizeChanged() {
    if (updateEnabled) {
        updateEnabled = false;
        useResolution = false;
        cbResolution->setCurrentIndex(cbResolution->findText("auto"));
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
        useResolution = true;
        leWidth->setText(QString("%1").arg(w));
        leHeight->setText(QString("%1").arg(h));
        updateEnabled = true;
    }
}

QSize QG_ImageOptionsDialog::getSize() {
    return QSize(RS_Math::round(RS_Math::eval(leWidth->text())),
                    RS_Math::round(RS_Math::eval(leHeight->text())));
}

QSize QG_ImageOptionsDialog::getBorders() {
    return QSize(RS_Math::round(RS_Math::eval(leLeftRight->text())),
                   RS_Math::round(RS_Math::eval(leTopBottom->text())));
}

bool QG_ImageOptionsDialog::isBackgroundBlack() {
    return rbBlack->isChecked();
}

bool QG_ImageOptionsDialog::isBlackWhite() {
    return rbBlackWhite->isChecked();
}
