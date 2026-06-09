/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef MAINWINDOWX_H
#define MAINWINDOWX_H

#include <QMainWindow>

/**
 * an eXtension of QMainWindow;
 * It is intended to be generic,
 * for use with other projects.
 */
class QToolBar;

class MainWindowX : public QMainWindow{
    Q_OBJECT
public:
    explicit MainWindowX(QWidget* parent = nullptr);
    void sortWidgetsByTitle(QList<QDockWidget*>& list);
    void sortWidgetsByTitle(QList<QToolBar*>& list);
    void sortWidgetsByGroupAndTitle(QList<QToolBar*>& list);
protected:
    void toggleDockArea(Qt::DockWidgetArea dockArea, bool state) const;
    void toggleToolBarArea(Qt::ToolBarArea tbArea, bool state) const;
public slots:
    void toggleRightDockArea(bool state);
    void toggleLeftDockArea(bool state);
    void toggleTopDockArea(bool state);
    void toggleBottomDockArea(bool state);
    void toggleFloatingDockwidgets(bool state);

    void toggleLeftToolbarArea(bool state);
    void toggleRightToolbarArea(bool state);
    void toggleTopToolbarArea(bool state);
    void toggleBottomToolbarArea(bool state);
};

#endif
