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
