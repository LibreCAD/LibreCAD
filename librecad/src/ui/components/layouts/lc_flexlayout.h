/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#ifndef LC_FLEXLAYOUT_H
#define LC_FLEXLAYOUT_H

#include <QLayout>
#include <QRect>
#include <QStyle>
#include <set>

/**
 * Minimalistic and straightforward implementation of Layout that position widgets based on given width, performing wrapping them on next line if
 * widgets do not fit into the width.
 */
class LC_FlexLayout :public QLayout
{
public:
    explicit LC_FlexLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit LC_FlexLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1, int widthOfFirstColumn = -1);
    ~LC_FlexLayout();

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;
    void fillFromLayout(QLayout *source);
    void setSoftBreakItems(const std::set<int> &itemPositions){softBreakItems = itemPositions;};
    void setFullWidthItems(const std::set<int> &itemPositions){fullWidthItems = itemPositions;};
private:
    int performLayout(const QRect &rect, bool geometryCheck) const;
    int defaultSpacing(QStyle::PixelMetric pm) const;

    /**
     *  layout items
     */
    QVector<QLayoutItem *> items;

    /**
     * indexes of widgets that should be moved to the next line if next widget does not fit width (used mostly for labels of controls to
     * avoid putting label and control on different lines
     */
    std::set<int> softBreakItems {};
    /**
     *  indexes of items that should occupy whole line if they does not fit into previous line
     */
    std::set<int> fullWidthItems {};
    /**
     * horizontal spacing
     */
    int hSpacing {-1};
    /**
     * vertical spacing
     */
    int vSpacing {-1};

    /**
     * fixed width of first column, if any
     */
    int firstColumnWidth {-1};
    int getSpaceX(const QWidget *wid) const;
    int getSpaceY(const QWidget *wid) const;
};

#endif // LC_FLEXLAYOUT_H
