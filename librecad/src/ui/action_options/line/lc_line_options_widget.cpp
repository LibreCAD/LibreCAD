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
#include "lc_line_options_widget.h"

#include "lc_action_draw_line.h"
#include "ui_lc_line_options_widget.h"

/*
 *  Constructs a QG_LineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineOptionsWidget::LC_LineOptionsWidget():ui(new Ui::LC_LineOptionsWidget{}) {
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineOptionsWidget::~LC_LineOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}


void LC_LineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = dynamic_cast<LC_ActionDrawLine*>(a);
    enableButtons();
}

void LC_LineOptionsWidget::close() const {
    if (m_action != nullptr) {
        m_action->close();
    }
}

void LC_LineOptionsWidget::undo() {
    if (m_action != nullptr) {
        m_action->undo();
        m_action->updateOptions();
    }
}

void LC_LineOptionsWidget::redo() {
    if (m_action != nullptr) {
        m_action->redo();
        m_action->updateOptions();
    }
}

void LC_LineOptionsWidget::enableButtons() const {
    if (m_action != nullptr) {
        ui->bClose->setEnabled(m_action->mayClose());
        ui->bUndo->setEnabled(m_action->mayUndo());
        ui->bRedo->setEnabled(m_action->mayRedo());
    }
}
