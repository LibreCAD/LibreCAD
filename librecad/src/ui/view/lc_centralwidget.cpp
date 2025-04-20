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

#include "lc_centralwidget.h"

#include <QMdiArea>
#include <QVBoxLayout>

LC_CentralWidget::LC_CentralWidget(QWidget* parent)
    : QFrame(parent)
    , m_mdiArea(new QMdiArea(this)){
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_mdiArea);

    m_mdiArea->setObjectName("mdi_area");
    m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setFocusPolicy(Qt::ClickFocus);
    m_mdiArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    m_mdiArea->setTabsMovable(true);
    m_mdiArea->setTabsClosable(true);

    setLayout(layout);
}

QMdiArea* LC_CentralWidget::getMdiArea() const{
    return m_mdiArea;
}
