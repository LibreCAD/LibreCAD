/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (ravas@outlook.com)
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

    QAction* action;

    action = new QAction(tr("&Copy"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));
    addAction(action);

    action = new QAction(tr("select&All"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));
    addAction(action);

    setStyleSheet("selection-color: white; selection-background-color: green;");
}

void QG_CommandHistory::mouseReleaseEvent(QMouseEvent* event)
{
    QTextEdit::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        copy();
    }
}
