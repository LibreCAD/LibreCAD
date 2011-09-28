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
#include "qg_snapmiddleoptions.h"

#include <qvariant.h>

/*
 *  Constructs a QG_SnapMiddleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SnapMiddleOptions::QG_SnapMiddleOptions(int& i, QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
    middlePoints=&i;
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SnapMiddleOptions::~QG_SnapMiddleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SnapMiddleOptions::languageChange()
{
    retranslateUi(this);
}

void QG_SnapMiddleOptions::destroy() {
    RS_SETTINGS->beginGroup("/Snap");
    RS_SETTINGS->writeEntry("/MiddlePoints", *middlePoints);
    RS_SETTINGS->endGroup();
}

void QG_SnapMiddleOptions::setMiddlePoints( int& i, bool initial) {
    middlePoints = &i;
    if(initial) {
    RS_SETTINGS->beginGroup("/Snap");
    *middlePoints=RS_SETTINGS->readNumEntry("/MiddlePoints", 1);
    if( !( *middlePoints>=1 && *middlePoints<=99)) {
        *middlePoints=1;
        RS_SETTINGS->writeEntry("/MiddlePoints", 1);
    }
    sbMiddlePoints->setValue(*middlePoints);
    RS_SETTINGS->endGroup();
    } else {
        *middlePoints=sbMiddlePoints->value();
    }

}

void QG_SnapMiddleOptions::updateMiddlePoints() {
    if (middlePoints != NULL) {
        *middlePoints = sbMiddlePoints->value();
    }
}

void QG_SnapMiddleOptions::on_sbMiddlePoints_valueChanged(int i)
{
    if (middlePoints != NULL) {
        *middlePoints = i;/*
    RS_SETTINGS->beginGroup("/Snap");
    RS_SETTINGS->writeEntry("/MiddlePoints", *middlePoints);
    RS_SETTINGS->endGroup();*/
    }
}
//EOF
