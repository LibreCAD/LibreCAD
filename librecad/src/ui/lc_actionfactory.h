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

#ifndef LC_ACTIONFACTORY_H
#define LC_ACTIONFACTORY_H

#include <QObject>
#include <QMap>

class QActionGroup;
class QAction;
class LC_ActionGroupManager;
class QC_ApplicationWindow;
class QG_ActionHandler;

class LC_ActionFactory : public QObject
{
    Q_OBJECT

public:
    LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler);
    void fillActionContainer(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm);
    void commonActions(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm);
    bool using_theme;

private:
    QC_ApplicationWindow* main_window;
    QG_ActionHandler* action_handler;
};

#endif

