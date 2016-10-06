/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
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

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

#include "lc_dockwidget.h"

#include <QToolButton>
#include <QGridLayout>
#include <QFrame>

LC_DockWidget::LC_DockWidget(QWidget* parent)
    : QDockWidget(parent)
    , frame(new QFrame(this))
    , grid(new QGridLayout)
{
	frame->setContentsMargins(0,0,0,0);
	setWidget(frame);

    grid->setSpacing(2);
	grid->setContentsMargins(1,1,1,1);
	frame->setLayout(grid);
}

void LC_DockWidget::add_actions(const QList<QAction*>& list, int columns, int icon_size)
{
	for (auto const& item: list)
    {
		QToolButton* toolbutton = new QToolButton(this);
		toolbutton->setDefaultAction(item);
		toolbutton->setIconSize(QSize(icon_size, icon_size));
		int const count = grid->count();
		grid->addWidget(toolbutton, count/columns, count%columns);
    }
}
