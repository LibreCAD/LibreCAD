/****************************************************************************
**
* Utility base class for widgets that represents options for actions

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 sand1024

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
**********************************************************************/
#ifndef LC_SNAPOPTIONSHOLDERMANAGER_H
#define LC_SNAPOPTIONSHOLDERMANAGER_H

#include <QWidget>

#include "lc_propertysheetwidget.h"

class LC_ActionOptionsWidget;
class LC_OptionsWidgetsHolder;
class QToolBar;
class LC_SnapOptionsWidgetsHolder;

class LC_ActionOptionsManager: public QObject{
    Q_OBJECT
public:
    LC_ActionOptionsManager(QWidget *parent, QToolBar *optionsToolbar, LC_SnapOptionsWidgetsHolder *snapOptionsHolder);
    ~LC_ActionOptionsManager() = default;
    void setOptionWidget(QToolBar *ow);
    void addOptionsWidget(LC_ActionOptionsWidget *options) const;
    void removeOptionsWidget(LC_ActionOptionsWidget *options) const;
    void hideSnapOptions();
    void requestSnapMiddleOptions(int *middlePoints, bool on);
    void requestSnapDistOptions(double *dist, bool on);
    void update();
    void clearActionIcon() const;
    LC_OptionsWidgetsHolder * getActionOptionWidgetHolder() const{
        return m_actionOptionWidgetHolder;
    }

    void setPropertySheetWidget(LC_PropertySheetWidget* widget) {m_propertySheetWidget = widget;}

protected:
    //! Pointer to the widget which can host individual tool options
    QToolBar* m_actionOptionsToolbar = nullptr;
    LC_PropertySheetWidget* m_propertySheetWidget {nullptr};
    LC_OptionsWidgetsHolder* m_actionOptionWidgetHolder = nullptr;
    LC_SnapOptionsWidgetsHolder * m_snapOptionsWidgetHolderSnapToolbar = nullptr;
    LC_SnapOptionsWidgetsHolder * m_snapOptionsWidgetHolderOptionsToolbar = nullptr;
    LC_SnapOptionsWidgetsHolder * m_lastUsedSnapOptionsWidgetHolder = nullptr;
    LC_SnapOptionsWidgetsHolder *getSnapOptionsHolder();

    bool m_previousSnapDistanceMode = {false};
    bool m_previousSnapMiddleMode = {false};
};

#endif
