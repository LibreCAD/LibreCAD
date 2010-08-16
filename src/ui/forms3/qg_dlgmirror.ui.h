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

void QG_DlgMirror::init() {
    RS_SETTINGS->beginGroup("/Modify");
    numberMode = RS_SETTINGS->readNumEntry("/MirrorMode", 0);
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/MirrorUseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/MirrorUseCurrentAttributes", 0);
    RS_SETTINGS->endGroup();

    switch (numberMode) {
    case 0:
        rbMove->setChecked(true);
        break;
    case 1:
        rbCopy->setChecked(true);
        break;
    default:
        break;
    }
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgMirror::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/MirrorMode", numberMode);
    RS_SETTINGS->writeEntry("/MirrorUseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/MirrorUseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgMirror::setData(RS_MirrorData* d) {
    data = d;
}

void QG_DlgMirror::updateData() {
    if (rbMove->isChecked()) {
        data->copy = false;
    } else if (rbCopy->isChecked()) {
        data->copy = true;
    }
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

