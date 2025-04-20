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
// oh - just have and options (hor/ver orientation)

#include "lc_caddockwidget.h"

#include <QFrame>
#include <QGridLayout>
#include <QToolButton>

#include "rs_settings.h"

LC_CADDockWidget::LC_CADDockWidget(QWidget* parent)
    : QDockWidget(parent)
    , m_frame(new QFrame(this))
    , m_gridLayout(new QGridLayout)
{
    m_frame->setContentsMargins(0, 0, 0, 0);
    setWidget(m_frame);

    // grid->setSpacing(2);
    m_gridLayout->setSpacing(0);
    // grid->setContentsMargins(1, 1, 1, 1);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_frame->setLayout(m_gridLayout);
}

void LC_CADDockWidget::add_actions(const QList<QAction *> &list, int columns, int icon_size, bool flatButton){
	for (auto const &item: list) {
		auto *toolbutton = new QToolButton(this);
		toolbutton->setDefaultAction(item);
		toolbutton->setAutoRaise(flatButton);
		toolbutton->setIconSize(QSize(icon_size, icon_size));
		int const count = m_gridLayout->count();
		if (columns == 0) {
			columns = 5;
		}
		m_gridLayout->addWidget(toolbutton, count / columns, count % columns);
	}
}

void LC_CADDockWidget::updateWidgetSettings(){
	LC_GROUP("Widgets"); {
		int leftToolbarColumnsCount = LC_GET_INT("LeftToolbarColumnsCount", 5);
		bool leftToolbarFlatIcons = LC_GET_BOOL("LeftToolbarFlatIcons", true);
		int leftToolbarIconSize = LC_GET_INT("LeftToolbarIconSize", 24);

		QSize size(leftToolbarIconSize, leftToolbarIconSize);

		QList<QToolButton *> widgets = m_frame->findChildren<QToolButton *>();

		auto* newGrid = new QGridLayout();
		newGrid->setSpacing(0);
		newGrid->setContentsMargins(0, 0, 0, 0);

		if (leftToolbarColumnsCount == 0) {
			leftToolbarColumnsCount = 5;
		}

		foreach(QToolButton *w, widgets) {
			w->setAutoRaise(leftToolbarFlatIcons);
			w->setIconSize(size);
			m_gridLayout->removeWidget(w);
			int const count = newGrid->count();
			newGrid->addWidget(w, count / leftToolbarColumnsCount, count % leftToolbarColumnsCount);
		}
		delete m_frame->layout();
		m_frame->setLayout(newGrid);
		m_gridLayout = newGrid;
	}
	LC_GROUP_END();
}
