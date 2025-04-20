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

#include "lc_actionoptionsmanager.h"

#include <QToolBar>

#include "lc_actionoptionswidget.h"
#include "lc_optionswidgetsholder.h"
#include "rs_settings.h"

LC_ActionOptionsManager::LC_ActionOptionsManager([[maybe_unused]]QWidget* parent, QToolBar* optionsToolbar,
                                                 LC_SnapOptionsWidgetsHolder* snapOptionsHolder){
    m_snapOptionsWidgetHolderSnapToolbar  = snapOptionsHolder;
    setOptionWidget(optionsToolbar);
}

void LC_ActionOptionsManager::setOptionWidget(QToolBar* ow) {
    m_actionOptionsToolbar = ow;
    m_actionOptionWidgetHolder = new LC_OptionsWidgetsHolder(ow);
    m_actionOptionsToolbar->addWidget(m_actionOptionWidgetHolder);
    m_snapOptionsWidgetHolderOptionsToolbar = m_actionOptionWidgetHolder->getSnapOptionsHolder();
}

void LC_ActionOptionsManager::addOptionsWidget(LC_ActionOptionsWidget * options){
    m_actionOptionWidgetHolder->addOptionsWidget(options);
    m_actionOptionsToolbar->update();
}

void LC_ActionOptionsManager::removeOptionsWidget(LC_ActionOptionsWidget * options){
    m_actionOptionWidgetHolder->removeOptionsWidget(options);
}

void LC_ActionOptionsManager::hideSnapOptions(){
    getSnapOptionsHolder()->hideSnapOptions();
}

/**
 * Shows a widget for 'snap to equidistant middle points ' options.
 */
void LC_ActionOptionsManager::requestSnapMiddleOptions(int* middlePoints, bool on) {
    getSnapOptionsHolder()->showSnapMiddleOptions(middlePoints, on);
}

/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void LC_ActionOptionsManager::requestSnapDistOptions(double* dist, bool on) {
    getSnapOptionsHolder()->showSnapDistOptions(dist, on);
}

LC_SnapOptionsWidgetsHolder* LC_ActionOptionsManager::getSnapOptionsHolder(){
    LC_SnapOptionsWidgetsHolder* result = nullptr;
    bool useSnapToolbar = LC_GET_ONE_BOOL("Appearance", "showSnapOptionsInSnapToolbar", false);
    if (useSnapToolbar){
        result = m_snapOptionsWidgetHolderSnapToolbar;
        m_snapOptionsWidgetHolderOptionsToolbar->setVisible(false);
    }
    else{
        result = m_snapOptionsWidgetHolderOptionsToolbar;
        m_snapOptionsWidgetHolderOptionsToolbar->setVisible(true);
    }
    if (m_lastUsedSnapOptionsWidgetHolder != nullptr && m_lastUsedSnapOptionsWidgetHolder != result){
        result->updateBy(m_lastUsedSnapOptionsWidgetHolder);
    }
    m_lastUsedSnapOptionsWidgetHolder = result;

    return result;
}

void LC_ActionOptionsManager::update(){
    getSnapOptionsHolder();
}

void LC_ActionOptionsManager::clearActionIcon(){
    m_actionOptionWidgetHolder->clearActionIcon();
}
