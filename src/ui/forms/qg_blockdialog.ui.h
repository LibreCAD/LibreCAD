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

void QG_BlockDialog::setBlockList(RS_BlockList* l) {
	RS_DEBUG->print("QG_BlockDialog::setBlockList");
	
    blockList = l;
    if (blockList!=NULL) {
        RS_Block* block = blockList->getActive();
        if (block!=NULL) {
            leName->setText(block->getName());
        } else {
            RS_DEBUG->print(RS_Debug::D_ERROR, 
				"QG_BlockDialog::setBlockList: No block active.");
        }
    }
}

RS_BlockData QG_BlockDialog::getBlockData() {
    /*if (blockList!=NULL) {
      RS_Block* block = blockList->getActive();
        if (block!=NULL) {
           return blockList->rename(block, leName->text().latin1());
        }
}

    return false;*/

    return RS_BlockData(leName->text(), RS_Vector(0.0,0.0), false);
}

void QG_BlockDialog::validate() {
    QString name = leName->text();

    if (!name.isEmpty()) {
        if (blockList!=NULL && blockList->find(name)==NULL) {
            accept();
        } else {
            QMessageBox::warning( this, tr("Renaming Block"),
                                  tr("Could not name block. A block named \"%1\" "
                                     "already exists.").arg(leName->text()),
                                  QMessageBox::Ok,
                                  Qt::NoButton);
        }
    }
    //else {
    //reject();
    //}
}

void QG_BlockDialog::cancel() {
    leName->setText("");
    reject();
}
