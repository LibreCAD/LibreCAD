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

#include "mainwindowx.h"
#include "rs_debug.h"

#include <QDockWidget>
#include <QToolBar>

namespace Sorting
{
    bool byWindowTitle(QWidget* left, QWidget* right)
    {
        return left->windowTitle() < right->windowTitle();
    }

    bool byGroupAndWindowTitle(QWidget* left, QWidget* right) {

        const QVariant &leftGroup = left->property("_group");
        const QVariant &rightGroup = right->property("_group");

        int iLeftGroup = -100;
        if (leftGroup.isValid()) {
            bool ok = false;
            iLeftGroup = leftGroup.toInt(&ok);
            if (!ok) {
                iLeftGroup = -100;
            }
        }

        int iRightGroup = -100;
        if (rightGroup.isValid()) {
            bool ok = false;
            iRightGroup = rightGroup.toInt(&ok);
            if (!ok) {
                iRightGroup = -100;
            }
        }

        bool result;

        if (iLeftGroup < iRightGroup) {
            result =  true;
        } else if (iLeftGroup == iRightGroup) {
            result = QString::compare(left->windowTitle(),right->windowTitle()) < 0;
        } else if (iLeftGroup > iRightGroup) {
            result = false;
        }
//        LC_ERR << iLeftGroup << " " << iRightGroup << " " << result;
        return result;
    }
}

MainWindowX::MainWindowX(QWidget* parent)
    : QMainWindow(parent) {}

void MainWindowX::sortWidgetsByTitle(QList<QDockWidget*>& list){
    std::sort(list.begin(), list.end(), Sorting::byWindowTitle);
}

void MainWindowX::sortWidgetsByTitle(QList<QToolBar*>& list){
    std::sort(list.begin(), list.end(), Sorting::byWindowTitle);
}

void MainWindowX::sortWidgetsByGroupAndTitle(QList<QToolBar *> &list) {
        std::sort(list.begin(), list.end(), Sorting::byGroupAndWindowTitle);
}

void MainWindowX::toggleLeftDockArea(bool state){
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::LeftDockWidgetArea && !dw->isFloating())
            dw->setVisible(state);
    }
}

void MainWindowX::toggleRightDockArea(bool state){
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::RightDockWidgetArea && !dw->isFloating())
            dw->setVisible(state);
    }
}

void MainWindowX::toggleTopDockArea(bool state){
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::TopDockWidgetArea && !dw->isFloating())
            dw->setVisible(state);
    }
}

void MainWindowX::toggleBottomDockArea(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::BottomDockWidgetArea && !dw->isFloating())
            dw->setVisible(state);
    }
}

void MainWindowX::toggleFloatingDockwidgets(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dw->isFloating())
            dw->setVisible(state);
    }
}
