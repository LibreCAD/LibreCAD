/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_NAMEDVIEWSBUTTON_H
#define LC_NAMEDVIEWSBUTTON_H

#include <QToolButton>
#include <QDebug>
#include <QList>
#include <QMenu>
#include <QAction>
#include "lc_namedviewslistwidget.h"

class LC_NamedViewsButton : public QToolButton
{
Q_OBJECT
public:
    LC_NamedViewsButton(LC_NamedViewsListWidget* widget);

protected:
    LC_NamedViewsListWidget* widget{nullptr};
    QMenu* menu;
    QList<QAction*> createdActions;
protected slots:
    void fillMenu();
    void menuTriggered(bool checked = false);
};

#endif // LC_NAMEDVIEWSBUTTON_H
