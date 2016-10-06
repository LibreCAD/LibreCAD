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

// -- https://github.com/LibreCAD/LibreCAD --

#include "qg_commandhistory.h"
#include <QAction>
#include <QMouseEvent>

// -- commandline history (output) widget --

QG_CommandHistory::QG_CommandHistory(QWidget* parent) :
    QTextEdit(parent)
{
	setContextMenuPolicy(Qt::ActionsContextMenu);

	m_pCopy = new QAction(tr("&Copy"), this);
	connect(m_pCopy, SIGNAL(triggered()), this, SLOT(copy()));
	addAction(m_pCopy);
	m_pCopy->setVisible(false);
	//only show "copy" menu item when there's available selection to copy
	connect(this, SIGNAL(copyAvailable(bool)), m_pCopy, SLOT(setVisible(bool)));

	m_pSelectAll = new QAction(tr("Select &All"), this);
	connect(m_pSelectAll, SIGNAL(triggered()), this, SLOT(selectAll()));
	addAction(m_pSelectAll);
	connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));

    QAction* clear = new QAction(tr("Clear"), this);
    connect(clear, SIGNAL(triggered(bool)), this, SLOT(clear()));
    addAction(clear);

    setStyleSheet("selection-color: white; selection-background-color: green;");
}

void QG_CommandHistory::mouseReleaseEvent(QMouseEvent* event)
{
    QTextEdit::mouseReleaseEvent(event);
	if (event->button() == Qt::LeftButton && m_pCopy->isVisible())
    {
        copy();
    }
}

void QG_CommandHistory::slotTextChanged()
{
	//only show the selectAll item when there is text
	m_pSelectAll->setVisible(! toPlainText().isEmpty());
}

