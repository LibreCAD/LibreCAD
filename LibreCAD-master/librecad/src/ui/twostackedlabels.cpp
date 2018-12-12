/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
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

#include "twostackedlabels.h"

#include <QLabel>
#include <QVBoxLayout>

TwoStackedLabels::TwoStackedLabels(QWidget* parent)
    : QFrame(parent)
    , top_label(new QLabel(this))
    , bottom_label(new QLabel(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(top_label);
    layout->addWidget(bottom_label);
    layout->setContentsMargins(4,0,4,0);
    layout->setSpacing(0);
    setLayout(layout);
}

void TwoStackedLabels::setTopLabel(const QString& status)
{
    top_label->setText(status);
}

void TwoStackedLabels::setBottomLabel(const QString& status)
{
    bottom_label->setText(status);
}
