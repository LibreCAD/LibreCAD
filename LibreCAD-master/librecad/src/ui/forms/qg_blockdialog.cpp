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
#include <QMessageBox>
#include "qg_blockdialog.h"
#include "rs_blocklist.h"
#include "rs_block.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_BlockDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_BlockDialog::QG_BlockDialog(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_BlockDialog::languageChange()
{
    retranslateUi(this);
}

void QG_BlockDialog::setBlockList(RS_BlockList* l) {
        RS_DEBUG->print("QG_BlockDialog::setBlockList");

    blockList = l;
	if (blockList) {
        RS_Block* block = blockList->getActive();
		if (block) {
            leName->setText(block->getName());
		}
    }
}

RS_BlockData QG_BlockDialog::getBlockData() {
    return RS_BlockData(leName->text(), RS_Vector(0.0,0.0), false);
}

void QG_BlockDialog::validate() {
    QString name = leName->text();

    if (!name.isEmpty()) {
		if (blockList && !blockList->find(name)) {
            accept();
        } else {
            QMessageBox::warning( this, tr("Renaming Block"),
                                  tr("Could not name block. A block named \"%1\" "
                                     "already exists.").arg(leName->text()),
                                  QMessageBox::Ok,
                                  Qt::NoButton);
        }
	}
}

void QG_BlockDialog::cancel() {
    leName->setText("");
    reject();
}
