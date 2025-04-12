/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#ifndef LC_ANGLESBASISWIDGET_H
#define LC_ANGLESBASISWIDGET_H

#include <QWidget>
#include "lc_graphicviewaware.h"

namespace Ui{
    class LC_AnglesBasisWidget;
}

class RS_Graphic;

class LC_AnglesBasisWidget : public QWidget, public LC_GraphicViewAware{
    Q_OBJECT
public:
    explicit LC_AnglesBasisWidget(QWidget *parent, const char* name);
    ~LC_AnglesBasisWidget();
    void update(QString angle, bool counterclockwise);
    void update(RS_Graphic* graphic);
    void setGraphicView(RS_GraphicView* gview) override;
signals:
    void clicked();
public slots:
    void onIconsRefreshed();
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    QIcon m_iconClockwise;
    QIcon m_iconCounterClockwise;
    bool m_counterclockwise  = false;
private:
    Ui::LC_AnglesBasisWidget *ui;
    int m_iconSize = 24;
};

#endif // LC_ANGLESBASISWIDGET_H
