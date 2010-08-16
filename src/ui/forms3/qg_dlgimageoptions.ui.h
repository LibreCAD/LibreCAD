/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

void QG_ImageOptionsDialog::init() {
    graphicSize = RS_Vector(0.0,0.0);
    updateEnabled = true;
    
    RS_SETTINGS->beginGroup("/ExportImage");
    leWidth->setText(RS_SETTINGS->readEntry("/Width", "640"));
    leHeight->setText(RS_SETTINGS->readEntry("/Height", "480"));
    if (RS_SETTINGS->readEntry("/BlackBackground", "0")=="1") {
        rbBlack->setChecked(true);
    }
    /*if (RS_SETTINGS->readEntry("/Blackwhite", "0")=="1") {
        rbBlackwhite->setChecked(true);
    }*/
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
    //RS_SETTINGS->writeEntry("/Blackwhite", (int)rbBlackwhite->isChecked());
    RS_SETTINGS->endGroup();
    
    accept();
}

void QG_ImageOptionsDialog::sizeChanged() {
    if (updateEnabled) {
    updateEnabled = false;
    cbResolution->setCurrentText("auto");
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

/*bool QG_ImageOptionsDialog::isBlackwhite() {
    return rbBlackwhite->isChecked();
}*/
