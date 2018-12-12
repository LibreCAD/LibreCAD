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
#include "qg_selectionwidget.h"

#include "rs_settings.h"

/*
 *  Constructs a QG_SelectionWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SelectionWidget::QG_SelectionWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(name);
    setupUi(this);

    lEntities->setText("0");
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SelectionWidget::~QG_SelectionWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SelectionWidget::languageChange()
{
    retranslateUi(this);
}

void QG_SelectionWidget::setNumber(int n) {
    QString str;
    str.setNum(n);
    lEntities->setText(str);
}
void QG_SelectionWidget::setTotalLength(double l) {
    QString str;
    str.setNum(l, 'g', 6);
    lTotalLength->setText(str);
}
