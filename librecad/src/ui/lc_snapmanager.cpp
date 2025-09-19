/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_snapmanager.h"

#include "qg_snaptoolbar.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_snapper.h"


LC_SnapManager::LC_SnapManager(QG_SnapToolBar* snapToolbar):m_snapToolbar{snapToolbar} {
}

void LC_SnapManager::setSnaps(RS_SnapMode const& s)  { // some flag set except free{
    if (m_snapToolbar) {
        m_snapToolbar->setSnaps(s);
    }
    else {
        RS_DEBUG->print("QG_ActionHandler::slotSetSnaps(): snapToolBar is nullptr");
    }
    if (m_view != nullptr) {
        m_view->setDefaultSnapMode(s);
    }

    if (m_inTempSnapFreeMode) {
        if (RS_SnapMode::toInt(s) != 0) { // some flag set except free
            m_inTempSnapFreeMode = false;
        }
    }


    RS_DEBUG->print("QG_ActionHandler::slotSetSnaps(): ok");
}

bool LC_SnapManager::tryToProcessSnapActions(RS2::ActionType type)  {
    switch (type) {
        case RS2::ActionSnapCenter:
            toggleSnapCenter();
            return true;
        case RS2::ActionSnapDist:
            toggleSnapDist();
            return true;
        case RS2::ActionSnapEndpoint:
            toggleSnapEndpoint();
            return true;
        case RS2::ActionSnapGrid:
            toggleSnapGrid();
            return true;
        case RS2::ActionSnapIntersection:
            toggleSnapIntersection();
            return true;
        case RS2::ActionSnapMiddle:
            toggleSnapMiddle();
            return true;
        case RS2::ActionSnapOnEntity:
            toggleSnapOnEntity();
            return true;
        case RS2::ActionRestrictNothing:
            restrictNothing();
            return true;
        case RS2::ActionRestrictOrthogonal:
            restrictOrthogonal();
            return true;
        case RS2::ActionRestrictHorizontal:
            restrictHorizontal();
            return true;
        case RS2::ActionRestrictVertical:
            restrictVertical();
            return true;
        default:
            return false;
    }
}

void LC_SnapManager::toggleSnapFree()  {
    auto s = getSnaps();
    s.snapFree = !s.snapFree;
    setSnaps(s);
}

bool LC_SnapManager::toggleTemporarySnapFree()  {
    if (m_inTempSnapFreeMode) {
        m_inTempSnapFreeMode = false;
        setSnaps(m_savedSnapMode);
    }
    else {
        m_savedSnapMode = getSnaps();
        m_inTempSnapFreeMode =  true;
        setSnaps(RS_SnapMode());
    }
    return m_inTempSnapFreeMode;
}

void LC_SnapManager::toggleSnapGrid()  {
    auto s = getSnaps();
    s.snapGrid = !s.snapGrid;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapEndpoint()  {
    auto s = getSnaps();
    s.snapEndpoint = !s.snapEndpoint;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapOnEntity()  {
    auto s = getSnaps();
    s.snapOnEntity = !s.snapOnEntity;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapCenter()  {
    auto s = getSnaps();
    s.snapCenter = !s.snapCenter;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapMiddle()  {
    RS_SnapMode s = getSnaps();
    s.snapMiddle = !s.snapMiddle;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapDist()  {
    RS_SnapMode s = getSnaps();
    s.snapDistance = !s.snapDistance;
    setSnaps(s);
}

void LC_SnapManager::toggleSnapIntersection()  {
    RS_SnapMode s = getSnaps();
    s.snapIntersection = !s.snapIntersection;
    setSnaps(s);
}

RS_SnapMode LC_SnapManager::getSnaps() const {
    if (m_snapToolbar != nullptr) {
        return m_snapToolbar->getSnaps();
    }
    //return a free snap mode
    return RS_SnapMode();
}

void LC_SnapManager::disableSnaps()  {
    setSnaps(RS_SnapMode());
}

void LC_SnapManager::setSnapRestriction(RS2::SnapRestriction restriction)  {
    RS_SnapMode s = getSnaps();
    s.restriction = restriction;
    setSnaps(s);
}

void LC_SnapManager::restrictNothing()  {
    setSnapRestriction(RS2::RestrictNothing);
}

void LC_SnapManager::restrictOrthogonal()  {
    setSnapRestriction(RS2::RestrictOrthogonal);
}

void LC_SnapManager::restrictHorizontal()  {
    setSnapRestriction(RS2::RestrictHorizontal);
}

void LC_SnapManager::restrictVertical()  {
    setSnapRestriction(RS2::RestrictVertical);
}

// find snap restriction from menu
RS2::SnapRestriction LC_SnapManager::getSnapRestriction() const {
    return getSnaps().restriction;
}

void LC_SnapManager::setGraphicView(RS_GraphicView* gview) {
    m_view = gview;
}

void LC_SnapManager::setRelativeZeroLock(bool on) const {
    if (m_snapToolbar) {
        m_snapToolbar->setLockedRelativeZero(on);
    }
}
