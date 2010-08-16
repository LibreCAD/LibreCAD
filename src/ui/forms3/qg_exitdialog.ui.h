/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include <qmessagebox.h>
#include <q3accel.h>
#include <q3button.h>

//sets aditional accel, eg. Key_A for ALT+Key_A
static void makeLetterAccell(Q3Button *btn)
{
    if (btn->accel().count()<1)
        return;
    Q3Accel *a = new Q3Accel( btn );
    a->connectItem( a->insertItem( btn->accel() & ~(Qt::MODIFIER_MASK|Qt::UNICODE_ACCEL) ),
                    btn, SLOT(animateClick()) );
}

void QG_ExitDialog::init()
{
    //set dlg icon
	/* RVT_PORT
    QMessageBox mb("","",QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
    // RT_PORT l_icon->setPixmap( *mb.iconPixmap() );
    bLeave->setIconSet(QPixmap::fromMimeSource("fileclose.png"));
    makeLetterAccell( bLeave );
    bSave->setIconSet(QPixmap::fromMimeSource("filesave2.png"));
    makeLetterAccell( bSave );
    bSaveAs->setIconSet(QPixmap::fromMimeSource("filesaveas.png"));
    makeLetterAccell( bSaveAs );
    makeLetterAccell( bCancel ); */
}

void QG_ExitDialog::setText(const QString& text) {
/* RVT_PORT 
    lQuestion->setText(text);
    resize(lQuestion->sizeHint().width()+32,
           lQuestion->sizeHint().height() + layButtons->sizeHint().height()+32);
 */
}

void QG_ExitDialog::setTitle(const QString& text) {
    setCaption(text);
}

void QG_ExitDialog::setForce(bool force) {
     bCancel->setDisabled(force);
}

void QG_ExitDialog::slotSaveAs() {
    done(3);
}

void QG_ExitDialog::slotSave() {
    done(2);
}

 
