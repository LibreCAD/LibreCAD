/*******************************************************************************
 *
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD (librecad.org)
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 *
 ******************************************************************************/

#include "lc_layouttabbar.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QString>
#include <QTabBar>
#include <QTimer>

#include "rs_graphic.h"

namespace {
/// Polling interval (ms) used to pick up layout additions/removals from
/// RS_FilterDXFRW::addLayout via the live view RS_Graphic::layouts().  A
/// formal observer interface is deferred to PR 10b — the poll cadence is
/// low enough to be invisible to the user and only does work when the
/// layout count or active handle actually changed.
constexpr int kLayoutPollIntervalMs = 500;
}  // namespace

LC_LayoutTabBar::LC_LayoutTabBar(QWidget* parent)
    : QWidget(parent),
      m_tabBar(new QTabBar(this)),
      m_pollTimer(new QTimer(this)) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabBar);
    setLayout(layout);

    m_tabBar->setShape(QTabBar::RoundedSouth);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(true);
    m_tabBar->setMovable(false);
    m_tabBar->setObjectName("layoutTabBar");

    connect(m_tabBar, &QTabBar::tabBarClicked,
            this, &LC_LayoutTabBar::onTabBarClicked);

    m_pollTimer->setInterval(kLayoutPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout,
            this, &LC_LayoutTabBar::refreshFromGraphic);
}

LC_LayoutTabBar::~LC_LayoutTabBar() = default;

void LC_LayoutTabBar::setGraphic(RS_Graphic* graphic) {
    if (m_graphic == graphic) {
        return;
    }
    m_graphic = graphic;
    m_lastLayoutCount = 0;
    m_lastActiveHandle = 0;
    rebuildTabs();
    if (m_graphic != nullptr) {
        m_pollTimer->start();
    } else {
        m_pollTimer->stop();
    }
}

void LC_LayoutTabBar::refreshFromGraphic() {
    if (m_graphic == nullptr) {
        return;
    }
    const std::size_t count = m_graphic->layouts().size();
    const std::uint32_t active = m_graphic->activeLayoutHandle();
    if (count == m_lastLayoutCount && active == m_lastActiveHandle) {
        return;
    }
    rebuildTabs();
}

void LC_LayoutTabBar::rebuildTabs() {
    // Detach the click handler while we mutate the tab bar so the
    // re-population doesn't trigger a spurious tabClicked signal.
    QSignalBlocker blocker(m_tabBar);
    while (m_tabBar->count() > 0) {
        m_tabBar->removeTab(0);
    }
    m_handlesByIndex.clear();

    if (m_graphic == nullptr) {
        setVisible(false);
        return;
    }
    const auto& layouts = m_graphic->layouts();
    m_lastLayoutCount = layouts.size();
    m_lastActiveHandle = m_graphic->activeLayoutHandle();

    if (layouts.empty()) {
        // No paper-space layouts loaded (DXF or fresh document).  Hide
        // the tab bar entirely so it doesn't take vertical space below
        // the drawing area.
        setVisible(false);
        return;
    }
    setVisible(true);

    // AutoCAD shows the Model tab first.  LibreCAD's modelspace isn't
    // represented as a LayoutRecord (it lives on the BLOCK_RECORD side),
    // so synthesize a fixed first tab with handle == 0 — clicking it
    // calls setActiveLayoutHandle(0) which falls back to legacy margins
    // via activeLayoutMargins().
    m_tabBar->addTab(QStringLiteral("Model"));
    m_handlesByIndex.push_back(0);

    // Sort paper-space layouts by tabOrder (AutoCAD convention).  The
    // backing storage isn't pre-sorted by the reader — addLayout pushes
    // in import order — so the UI takes responsibility for the visible
    // ordering here.  Stable sort keeps the import order as a tiebreaker.
    std::vector<const LC_Layout*> sorted;
    sorted.reserve(layouts.size());
    for (const auto& layout : layouts) {
        sorted.push_back(&layout);
    }
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const LC_Layout* a, const LC_Layout* b) {
                         return a->tabOrder < b->tabOrder;
                     });

    for (const LC_Layout* layout : sorted) {
        const QString name = layout->name.empty()
            ? QStringLiteral("(unnamed)")
            : QString::fromStdString(layout->name);
        m_tabBar->addTab(name);
        m_handlesByIndex.push_back(layout->handle);
    }

    // Highlight the active layout — find its position in the rebuilt list.
    const std::uint32_t active = m_graphic->activeLayoutHandle();
    int activeIndex = 0;
    for (std::size_t i = 0; i < m_handlesByIndex.size(); ++i) {
        if (m_handlesByIndex[i] == active) {
            activeIndex = static_cast<int>(i);
            break;
        }
    }
    m_tabBar->setCurrentIndex(activeIndex);
}

void LC_LayoutTabBar::onTabBarClicked(int index) {
    if (m_graphic == nullptr) {
        return;
    }
    if (index < 0 || static_cast<std::size_t>(index) >= m_handlesByIndex.size()) {
        return;
    }
    const std::uint32_t handle = m_handlesByIndex[static_cast<std::size_t>(index)];
    m_graphic->setActiveLayoutHandle(handle);
    m_lastActiveHandle = handle;
    emit tabClicked(handle);
}