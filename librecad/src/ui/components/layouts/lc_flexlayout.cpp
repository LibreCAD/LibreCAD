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

#include "lc_flexlayout.h"

#include <QWidget>

LC_FlexLayout::LC_FlexLayout(QWidget *parent, const int margin, const int hSpacing, const int vSpacing)
    :QLayout(parent), m_hSpacing(hSpacing), m_vSpacing(vSpacing){
    setContentsMargins(margin, margin, margin, margin);
}

LC_FlexLayout::LC_FlexLayout(const int margin, const int hSpacing, const int vSpacing, const int widthOfFirstColumn)
    :m_hSpacing(hSpacing), m_vSpacing(vSpacing), m_firstColumnWidth(widthOfFirstColumn){
    setContentsMargins(margin, margin, margin, margin);
}

LC_FlexLayout::~LC_FlexLayout(){
    const QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr) {
        delete item;
    }
}

/**
 * Fills the layout by another layout. Items are removed from source layout.
 * This method allows to convert layouts created in designer into flex layouts.
 * @param source
 */
void LC_FlexLayout::fillFromLayout(QLayout* source){
    QLayoutItem* pItem = source->itemAt(0);
    while (pItem != nullptr) {
        QWidget* widget = pItem->widget();
        if (widget != nullptr){
            this->addWidget(widget);
        }
        source->removeItem(pItem);
        pItem = source->itemAt(0);
    }
}
bool LC_FlexLayout::hasHeightForWidth() const{
    return true;
}

int LC_FlexLayout::heightForWidth(const int width) const{
    const int height = performLayout(QRect(0, 0, width, 0), true);
    return height;
}

void LC_FlexLayout::setGeometry(const QRect &rect){
    QLayout::setGeometry(rect);
    performLayout(rect, false);
}

QSize LC_FlexLayout::sizeHint() const{
    return minimumSize();
}

int LC_FlexLayout::performLayout(const QRect &rect, const bool geometryCheck) const{
    int left = 0, top = 0, right = 0, bottom = 0;
    getContentsMargins(&left, &top, &right, &bottom);
    const QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int columnX = effectiveRect.x();
    int rowY = effectiveRect.y();
    const int effectiveRight = effectiveRect.right();
    int lineHeight = 0;

    const int size = m_items.size();

     std::vector<int> linesHeight;

     int nextY = rowY;

    // do first pass to determine height for each line and overall height

    for (int i = 0; i < size; i++) {
        const QLayoutItem *item = std::as_const(m_items.at(i));
        const QWidget *wid = item->widget();
        const QLayoutItem *itemNext = nullptr;
        bool checkForNextBreak = false;
        const int nextIndex = i + 1;
        if (nextIndex < size){
            itemNext = m_items.at(nextIndex);
            if (m_softBreakItems.find(i) != m_softBreakItems.end()){
                checkForNextBreak = true;
            }
        }
        const int spaceX = getSpaceX(wid);

        QSize itemSizeHint = item->sizeHint();
        int itemHeight = itemSizeHint.height();
        int itemWidth = itemSizeHint.width();
        int nextX = columnX + itemWidth + spaceX;

        bool doBreak = false;
        if (checkForNextBreak){
            const int nextWidgetNextX = nextX + itemNext->sizeHint().width();
            doBreak = nextWidgetNextX > effectiveRight;
        }
        const int widgetEndX = nextX - spaceX;

        if ((widgetEndX > effectiveRight && lineHeight > 0) || doBreak){
            const int spaceY = getSpaceY(wid);
            columnX = effectiveRect.x();
            rowY = rowY + lineHeight + spaceY;
            nextY = rowY;
            if (m_fullWidthItems.find(i) != m_fullWidthItems.end()){
                linesHeight.emplace_back(lineHeight);
                nextY = rowY + itemHeight + spaceY;
                nextX = columnX;
            }
            else {
                if (m_firstColumnWidth > 0){
                    itemWidth = m_firstColumnWidth;
                }
                nextX = columnX + itemWidth + spaceX;
            }
            linesHeight.emplace_back(qMax(lineHeight, itemHeight));
            lineHeight = 0;
        }
        lineHeight = qMax(lineHeight, itemHeight);
        rowY = nextY;
        columnX = nextX;
    }
    linesHeight.emplace_back(lineHeight);

    const int result = rowY + lineHeight - rect.y() + bottom;

    // second pass with actual setting geometry, line height is calculated on previous step
    // here the actual placing of items is performed
    if (!geometryCheck){
        columnX = effectiveRect.x();
        rowY = effectiveRect.y();
        nextY = rowY;
        int lineIndex = 0;
        // second pass that takes into consideration lines height
        for (int i = 0; i < size; i++) {
            QLayoutItem *item = std::as_const(m_items.at(i));
            const QWidget *wid = item->widget();
            const QLayoutItem *itemNext = nullptr;
            bool checkForNextBreak = false;
            const int nextIndex = i + 1;
            if (nextIndex < size){
                itemNext = m_items.at(nextIndex);
                if (m_softBreakItems.find(i) != m_softBreakItems.end()){
                    checkForNextBreak = true;
                }
            }
            const int spaceX = getSpaceX(wid);

            QSize itemSizeHint = item->sizeHint();
            const int currentLineHeight = linesHeight.at(lineIndex);
            int itemWidth = itemSizeHint.width();
            int nextX = columnX + itemWidth + spaceX;

            bool doBreak = false;
            if (checkForNextBreak){
                const int nextWidgetNextX = nextX + itemNext->sizeHint().width();
                doBreak = nextWidgetNextX > effectiveRight;
            }
            const int widgetEndX = nextX - spaceX;

            if ((widgetEndX > effectiveRight) || doBreak){
                const int spaceY = getSpaceY(wid);
                columnX = effectiveRect.x();
                rowY = rowY + currentLineHeight + spaceY;
                nextY = rowY;
                if (m_fullWidthItems.find(i) != m_fullWidthItems.end()){
                    itemWidth = effectiveRect.width();
                    nextY = rowY + currentLineHeight + spaceY;
                    nextX = columnX;
                } else {
                    if (m_firstColumnWidth > 0){
                        itemWidth = m_firstColumnWidth;
                    }
                    nextX = columnX + itemWidth + spaceX;
                }
                lineIndex++;
            }

            itemSizeHint = QSize(itemWidth, currentLineHeight);
            item->setGeometry(QRect(QPoint(columnX, rowY), itemSizeHint));
            rowY = nextY;
            columnX = nextX;
        }
    }
    return result;
}

int LC_FlexLayout::getSpaceY(const QWidget *wid) const{
    int spacingX = verticalSpacing();
    if (spacingX == -1) {
        spacingX = wid->style()->layoutSpacing(
            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
    }
    return spacingX;
}

int LC_FlexLayout::getSpaceX(const QWidget *wid) const{
    int spacingY = horizontalSpacing();
    if (spacingY == -1) {
        spacingY = wid->style()->layoutSpacing(
            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    }
    return spacingY;
}

int LC_FlexLayout::defaultSpacing(const QStyle::PixelMetric pm) const{
    QObject *parent = this->parent();
    if (parent == nullptr) {
        return -1;
    }
    if (parent->isWidgetType()) {
        const auto *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout *>(parent)->spacing();
}
int LC_FlexLayout::horizontalSpacing() const{
    if (m_hSpacing >= 0) {
        return m_hSpacing;
    }
    return defaultSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int LC_FlexLayout::verticalSpacing() const{
    if (m_vSpacing >= 0) {
        return m_vSpacing;
    }
    return defaultSpacing(QStyle::PM_LayoutVerticalSpacing);
}
QLayoutItem*LC_FlexLayout::takeAt(const int index){
    QLayoutItem* result = nullptr;
    if (index >= 0 && index < m_items.size()){
        result = m_items.takeAt(index);
    }
    return result;
}

Qt::Orientations LC_FlexLayout::expandingDirections() const{
    return {};
}

QSize LC_FlexLayout::minimumSize() const{
    QSize size;
    for (const QLayoutItem *item : std::as_const(m_items)) {
        size = size.expandedTo(item->minimumSize());
    }

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

void LC_FlexLayout::addItem(QLayoutItem *item){
    m_items.append(item);
}

int LC_FlexLayout::count() const{
    return m_items.size();
}

QLayoutItem *LC_FlexLayout::itemAt(const int index) const{
    return m_items.value(index);
}
