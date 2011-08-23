/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
#include "qg_mousewidget.h"

#include <qvariant.h>
#include "rs_settings.h"

/*
 *  Constructs a QG_MouseWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MouseWidget::QG_MouseWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_MouseWidget::~QG_MouseWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MouseWidget::languageChange()
{
    retranslateUi(this);
}

void QG_MouseWidget::init() {
    lLeftButton->setText("");
    lRightButton->setText("");
    
    int fsize;
#ifdef __APPLE__
    fsize = 9;
#else
    fsize = 7;
#endif
    
    RS_SETTINGS->beginGroup("/Appearance");
    fsize = RS_SETTINGS->readNumEntry("/StatusBarFontSize", fsize);
    RS_SETTINGS->endGroup();
    
    lLeftButton->setFont(QFont("Helvetica", fsize));
    lRightButton->setFont(QFont("Helvetica", fsize));
}

void QG_MouseWidget::setHelp(const QString& left, const QString& right) {
    lLeftButton->setText(left);
    lRightButton->setText(right);
}
