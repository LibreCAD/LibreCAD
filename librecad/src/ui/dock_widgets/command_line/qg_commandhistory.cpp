/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/
#include "qg_commandhistory.h"

#include <QMouseEvent>
// -- commandline history (output) widget --
QG_CommandHistory::QG_CommandHistory(QWidget* parent) :
    QTextEdit(parent){
    setContextMenuPolicy(Qt::ActionsContextMenu);

    m_pCopy = new QAction(tr("&Copy"), this);
    connect(m_pCopy, &QAction::triggered, this, &QG_CommandHistory::copy);
    addAction(m_pCopy);
    m_pCopy->setVisible(false);
    //only show "copy" menu item when there's available selection to copy
    connect(this, &QG_CommandHistory::copyAvailable, m_pCopy, &QAction::setVisible);

    m_pSelectAll = new QAction(tr("Select &All"), this);
    connect(m_pSelectAll, &QAction::triggered, this, &QG_CommandHistory::selectAll);
    addAction(m_pSelectAll);
    connect(this, &QG_CommandHistory::textChanged, this, &QG_CommandHistory::slotTextChanged);

    QAction* clear = new QAction(tr("Clear"), this);
    connect(clear, &QAction::triggered, this, &QG_CommandHistory::clear);
    addAction(clear);

#ifndef DONT_FORCE_WIDGETS_CSS
    setStyleSheet("selection-color: white; selection-background-color: green;");
#endif
}

void QG_CommandHistory::mouseReleaseEvent(QMouseEvent* event){
    QTextEdit::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && m_pCopy->isVisible())    {
        copy();
    }
}

void QG_CommandHistory::slotTextChanged(){
//only show the selectAll item when there is text
    m_pSelectAll->setVisible(! toPlainText().isEmpty());
}
