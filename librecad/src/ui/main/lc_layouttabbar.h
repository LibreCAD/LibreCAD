/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

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

#ifndef LC_LAYOUTTABBAR_H
#define LC_LAYOUTTABBAR_H

#include <QWidget>

#include "drw_base.h"

class QTabBar;
class QTimer;
class RS_Graphic;

/**
 * Paper-space layout tab bar (PR 10a — DWG OBJECTS surface).
 *
 * Surfaces the LayoutRecord set populated by RS_FilterDXFRW::addLayout
 * during DWG import.  Each paper-space layout becomes a tab; clicking a
 * tab calls RS_Graphic::setActiveLayoutHandle(handle).  No rendering
 * swap is performed — the tab bar is purely informational + the handle
 * setter so the plot dialog (PR 11) and downstream consumers can react
 * to the active-layout change.  Entity-block-record-scoped rendering is
 * tracked as PR 12 follow-up.
 *
 * The widget polls RS_Graphic::layouts() via a low-frequency QTimer to
 * pick up additions/removals — the underlying storage is the live view
 * over LC_DwgAdvancedMetadata::layouts(), so no observer interface is
 * required for now.  Active-tab tracking uses
 * RS_Graphic::activeLayoutHandle() so external mutations (DWG re-import,
 * filter override, etc.) reflect in the UI on the next poll tick.
 */
class LC_LayoutTabBar : public QWidget {
    Q_OBJECT
public:
    explicit LC_LayoutTabBar(QWidget* parent = nullptr);
    ~LC_LayoutTabBar() override;

    /** Bind to the active document.  Pass nullptr to detach.  Safe to
     *  re-bind (e.g. when QC_MDIWindow swaps documents). */
    void setGraphic(RS_Graphic* graphic);

    /** @return current bound graphic, or nullptr. */
    RS_Graphic* graphic() const { return m_graphic; }

signals:
    /** Emitted when the user clicks a tab.  Provided for QC_MDIWindow
     *  to hook custom behavior alongside the handle setter. */
    void tabClicked(duint32 handle);

private slots:
    void onTabBarClicked(int index);
    void refreshFromGraphic();

private:
    /** Repopulate the internal QTabBar from RS_Graphic::layouts() and
     *  highlight the active layout via activeLayoutHandle().  Sorts
     *  paper-space tabs by tabOrder (matching the AutoCAD convention).
     *  No-op when m_graphic == nullptr. */
    void rebuildTabs();

    RS_Graphic* m_graphic = nullptr;
    QTabBar* m_tabBar = nullptr;
    QTimer* m_pollTimer = nullptr;

    /** Tab-index → layout handle.  Empty for the modelspace tab (handle 0). */
    std::vector<duint32> m_handlesByIndex;

    /** Cached state used to suppress redundant rebuilds. */
    std::size_t m_lastLayoutCount = 0;
    duint32 m_lastActiveHandle = 0;
};

#endif  // LC_LAYOUTTABBAR_H
