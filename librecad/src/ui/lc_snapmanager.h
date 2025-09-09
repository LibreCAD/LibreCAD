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

#ifndef LC_SNAPMANAGER_H
#define LC_SNAPMANAGER_H

#include "lc_graphicviewaware.h"
#include "qg_snaptoolbar.h"
#include "rs_snapper.h"

class LC_SnapManager: public LC_GraphicViewAware{
public:
    explicit LC_SnapManager(QG_SnapToolBar* snapToolbar);
    void setSnaps(RS_SnapMode const& s);
    bool tryToProcessSnapActions(RS2::ActionType type);
    void toggleSnapFree();
    bool toggleTemporarySnapFree();
    void toggleSnapGrid();
    void toggleSnapEndpoint();
    void toggleSnapOnEntity();
    void toggleSnapCenter();
    void toggleSnapMiddle();
    void toggleSnapDist();
    void toggleSnapIntersection();
    RS_SnapMode getSnaps() const;
    void disableSnaps();
    void setSnapRestriction(RS2::SnapRestriction restriction);
    void restrictNothing();
    void restrictOrthogonal();
    void restrictHorizontal();
    void restrictVertical();
    RS2::SnapRestriction getSnapRestriction() const;
    void setGraphicView(RS_GraphicView* gview) override;
    void setRelativeZeroLock(bool on) const;
private:
    RS_GraphicView* m_view{nullptr};
    QG_SnapToolBar* m_snapToolbar{nullptr};
    RS_SnapMode m_savedSnapMode;
    bool m_inTempSnapFreeMode{false};
};

#endif // LC_SNAPMANAGER_H
