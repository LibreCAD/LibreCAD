/*
 * **************************************************************************
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
 * *********************************************************************
 *
 */

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

// fixme - sand - add support of flex layout, with it potentially will be possible to support something ribbon-like
// oh - just have and options (hor/ver  orientation)

#include "lc_caddockwidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>

#include "rs_settings.h"

LC_CADDockWidget::LC_CADDockWidget(QWidget *parent, const bool allTools)
    : QDockWidget(parent), m_frame(new QFrame(this)),
      m_gridLayout(new QGridLayout), m_allTools{allTools} {
  m_frame->setContentsMargins(0, 0, 0, 0);

  if (allTools) {
      m_scrollArea = new QScrollArea(this);
      m_scrollArea->setWidgetResizable(true);
      m_scrollArea->setFrameStyle(QFrame::NoFrame); // Avoid double borders with the dock widget
      m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Clip horizontally instead of showing scrollbars
      m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
      m_scrollArea->setWidget(m_frame);
      setWidget(m_scrollArea);
  }
  else {
      setWidget(m_frame);
  }
  m_gridLayout->setSpacing(0);
  m_gridLayout->setContentsMargins(0, 0, 0, 0);
  m_frame->setLayout(m_gridLayout);
}

void LC_CADDockWidget::addSpacers(QGridLayout *layout, const int columns) {
  const auto verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum,
                                        QSizePolicy::Policy::Expanding);
  const int filledRows = layout->count() / columns;
  layout->addItem(verticalSpacer, filledRows + 1, 0, 1, 1);

  // auto hSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding,
  // QSizePolicy::Policy::Minimum); layout->addItem(hSpacer, 0, columns,
  // filledRows + 1, 1);
}

void LC_CADDockWidget::addActions(const QList<QAction *> &list, int columns, const int iconSize, const bool flatButton) {
  for (const auto&item : list) {
    auto *toolButton = new QToolButton(this);
    toolButton->setDefaultAction(item);
    toolButton->setAutoRaise(flatButton);
    toolButton->setIconSize(QSize(iconSize, iconSize));

    toolButton->setFixedSize(QSize(iconSize + 8, iconSize + 8));

    const int count = m_gridLayout->count();
    if (columns == 0) {
       columns = 5;
    }

    m_gridLayout->addWidget(toolButton, count / columns, count % columns);
  }

  addSpacers(m_gridLayout, columns);

  m_frame->setFrameShadow(QFrame::Raised);
  m_frame->setLineWidth(2);
}

void LC_CADDockWidget::doUpdateWidgetSettings(int leftToolbarColumnsCount, const int leftToolbarIconSize, const bool leftToolbarFlatIcons) {
  const QSize size(leftToolbarIconSize, leftToolbarIconSize);

  QList<QToolButton *> widgets = m_frame->findChildren<QToolButton *>();

  auto *newGridLayout = new QGridLayout();
  newGridLayout->setSpacing(0);
  newGridLayout->setContentsMargins(0, 0, 0, 0);

  if (leftToolbarColumnsCount == 0) {
    leftToolbarColumnsCount = 5;
  }

  foreach (QToolButton *w, widgets) {
    w->setAutoRaise(leftToolbarFlatIcons);
    w->setIconSize(size);

    w->setFixedSize(QSize(leftToolbarIconSize + 8, leftToolbarIconSize + 8));

    m_gridLayout->removeWidget(w);
    const int count = newGridLayout->count();
    newGridLayout->addWidget(w, count / leftToolbarColumnsCount,
                             count % leftToolbarColumnsCount);
  }
  delete m_frame->layout();

  addSpacers(newGridLayout, leftToolbarColumnsCount);
  m_frame->setLayout(newGridLayout);
  m_gridLayout = newGridLayout;

  m_columns = leftToolbarColumnsCount;
  m_iconSize = leftToolbarIconSize;

  updateMinimumWidth();
}

void LC_CADDockWidget::updateWidgetSettings() {
  LC_GROUP("Widgets");
  int columnsCount = 0, iconSize = 0;
  bool flatIcons = false;
  if (m_allTools) {
    columnsCount = LC_GET_INT("LeftToolbarAllColumnsCount", 5);
    flatIcons = LC_GET_BOOL("LeftToolbarAllFlatIcons", true);
    iconSize = LC_GET_INT("LeftToolbarAllIconSize", 24);
  } else {
    columnsCount = LC_GET_INT("LeftToolbarColumnsCount", 5);
    flatIcons = LC_GET_BOOL("LeftToolbarFlatIcons", true);
    iconSize = LC_GET_INT("LeftToolbarIconSize", 24);
  }
  doUpdateWidgetSettings(columnsCount, iconSize, flatIcons);
  LC_GROUP_END();
}

void LC_CADDockWidget::updateMinimumWidth() {
    if (!m_scrollArea) return;

    // Mathematically calculate the exact horizontal space required by the grid
    const int contentWidth = m_columns * (m_iconSize + 8);

    // Retrieve the OS vertical scrollbar width
    int sbWidth = m_scrollArea->verticalScrollBar()->sizeHint().width();
    if (sbWidth <= 0) {
        sbWidth = 16;
    }

    // Account for m_frame's Raised border shadows
    const int framePadding = 2 * m_frame->lineWidth();

    const int totalMinWidth = contentWidth + sbWidth + framePadding;

    m_scrollArea->setMinimumWidth(totalMinWidth);
    setMinimumWidth(totalMinWidth);


    updateGeometry();
}

QSize LC_CADDockWidget::minimumSizeHint() const {
    QSize baseHint = QDockWidget::minimumSizeHint();
    if (m_scrollArea != nullptr) {
        const int minW = m_scrollArea->minimumWidth();
        if (minW > 0) {
            baseHint.setWidth(minW);
        }
    }
    return baseHint;
}
