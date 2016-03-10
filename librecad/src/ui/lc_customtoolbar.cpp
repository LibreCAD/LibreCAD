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

#include "lc_customtoolbar.h"

#include <QTextStream>
#include <QFile>
#include <QAction>

LC_CustomToolbar::LC_CustomToolbar(QWidget* parent)
    : QToolBar(parent)
{
    QAction* action = new QAction(tr("Add or Remove Action"), parent);
    action->setShortcut(QKeySequence("F2"));
    action->setIcon(QIcon(":/extui/char_pm.png"));
    connect(action, SIGNAL(triggered()), this, SLOT(slot_add_or_remove_action()));
    addAction(action);
}

LC_CustomToolbar::~LC_CustomToolbar()
{
    if (!file_path.isNull())
    {
        QFile file(file_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream txt_stream(&file);
        foreach (const QString& token, state_list)
        {
            txt_stream << token << "\n";
        }
    }
}

void LC_CustomToolbar::actions_from_file(const QString& path,
                                         QMap<QString, QAction*>& a_map)
{
    QFile file(path);    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    file_path = path;

    QTextStream txt_stream(&file);
    QString line;
    while (!txt_stream.atEnd())
    {
        line = txt_stream.readLine();

        if (line == "-")
        {
            addSeparator();
            state_list << line;
        }
        else if (a_map.contains(line))
        {
            addAction(a_map[line]);
            state_list << line;
        }
    }
}

void LC_CustomToolbar::slot_add_or_remove_action()
{
    if (most_recent_action )
    {
        QString token = most_recent_action->objectName();

        if (state_list.contains(token))
        {
            removeAction(most_recent_action);
            state_list.removeOne(token);
        }
        else
        {
            addAction(most_recent_action);
            state_list << token;
        }
    }
}

void LC_CustomToolbar::slot_most_recent_action(QAction* q_action)
{
    most_recent_action = q_action;
}


void LC_CustomToolbar::add_separator()
{
    addSeparator();
    state_list << "-";
}
