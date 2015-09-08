/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
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

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

#include "lc_dockwidget.h"

#include <QToolButton>

LC_DockWidget::LC_DockWidget(QWidget* parent): QDockWidget(parent)
{
    frame = new QFrame;
    this->setWidget(frame);
    grid = new QGridLayout;
    frame->setLayout(grid);
}

void LC_DockWidget::add_actions(QList<QAction*> list, int columns, int icon_size)
{
    --columns;
    int row = 0;
    int column = 0;
    QToolButton* toolbutton;

    foreach (auto item, list)
    {
        toolbutton = new QToolButton;
        toolbutton->setDefaultAction(item);
        toolbutton->setIconSize(QSize(icon_size, icon_size));
        grid->addWidget(toolbutton, row, column);

        if (column == columns)
        {
            column = 0;
            ++row;
        }
        else
        {
            ++column;
        }
    }
}
