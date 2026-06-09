/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_PENWIZARD_H
#define LC_PENWIZARD_H

#include "lc_graphicviewawarewidget.h"

class ColorWizard;

class LC_PenWizard : public LC_GraphicViewAwareWidget{
    Q_OBJECT
public:
    explicit LC_PenWizard(QWidget* parent = nullptr);
    void setGraphicView(RS_GraphicView* gview) override;
protected slots:
    void setColorForSelected(QColor color) const;
    void selectByColor(QColor color) const;
    void setActivePenColor(QColor color) const;
protected:
    QLayout* getTopLevelLayout() const override;
private:
    RS_GraphicView* m_graphicView = nullptr;
    ColorWizard* m_colorWizard = nullptr;
};

#endif
