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
#include <QtWidgets>
#include "lc_flexlayout.h"

LC_FlexLayout::LC_FlexLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    :QLayout(parent), hSpacing(hSpacing), vSpacing(vSpacing){
    setContentsMargins(margin, margin, margin, margin);
}

LC_FlexLayout::LC_FlexLayout(int margin, int hSpacing, int vSpacing, int widthOfFirstColumn)
    :hSpacing(hSpacing), vSpacing(vSpacing), firstColumnWidth(widthOfFirstColumn){
    setContentsMargins(margin, margin, margin, margin);
}

LC_FlexLayout::~LC_FlexLayout(){
    QLayoutItem *item;
    while ((item = takeAt(0))) {
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

int LC_FlexLayout::heightForWidth(int width) const{
    int height = performLayout(QRect(0, 0, width, 0), true);
    return height;
}

void LC_FlexLayout::setGeometry(const QRect &rect){
    QLayout::setGeometry(rect);
    performLayout(rect, false);
}

QSize LC_FlexLayout::sizeHint() const{
    return minimumSize();
}

int LC_FlexLayout::performLayout(const QRect &rect, bool geometryCheck) const{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int columnX = effectiveRect.x();
    int rowY = effectiveRect.y();
    int effectiveRight = effectiveRect.right();
    int lineHeight = 0;

    int size = items.size();

     std::vector<int> linesHeight;

     int nextY = rowY;

    // do first pass to determine height for each line and overall height

    for (int i = 0; i < size; i++) {
        QLayoutItem *item = std::as_const(items.at(i));
        const QWidget *wid = item->widget();
        QLayoutItem *itemNext = nullptr;
        bool checkForNextBreak = false;
        int nextIndex = i + 1;
        if (nextIndex < size){
            if (softBreakItems.find(i) != softBreakItems.end()){
                itemNext = items.at(nextIndex);
                checkForNextBreak = true;
            }
        }
        int spaceX = getSpaceX(wid);
        int spaceY = getSpaceY(wid);
        QSize itemSizeHint = item->sizeHint();
        int itemHeight = itemSizeHint.height();
        int itemWidth = itemSizeHint.width();
        int nextX = columnX + itemWidth + spaceX;

        bool doBreak = false;
        if (checkForNextBreak){
            int nextWidgetNextX = nextX + itemNext->sizeHint().width();
            doBreak = nextWidgetNextX > effectiveRight;
        }
        int widgetEndX = nextX - spaceX;

        if ((widgetEndX > effectiveRight && lineHeight > 0) || doBreak){
            columnX = effectiveRect.x();
            rowY = rowY + lineHeight + spaceY;
            nextY = rowY;
            if (fullWidthItems.find(i) != fullWidthItems.end()){
                linesHeight.emplace_back(lineHeight);
                nextY = rowY + itemHeight + spaceY;
                nextX = columnX;
            }
            else {
                if (firstColumnWidth > 0){
                    itemWidth = firstColumnWidth;
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

    int result = rowY + lineHeight - rect.y() + bottom;

    // second pass with actual setting geometry, line height is calculated on previous step
    // here the actual placing of items is performed
    if (!geometryCheck){
        columnX = effectiveRect.x();
        rowY = effectiveRect.y();
        nextY = rowY;
        int lineIndex = 0;
        int currentLineHeight;
        // second pass that takes into consideration lines height
        for (int i = 0; i < size; i++) {
            QLayoutItem *item = std::as_const(items.at(i));
            const QWidget *wid = item->widget();
            QLayoutItem *itemNext = nullptr;
            bool checkForNextBreak = false;
            int nextIndex = i + 1;
            if (nextIndex < size){
                if (softBreakItems.find(i) != softBreakItems.end()){
                    itemNext = items.at(nextIndex);
                    checkForNextBreak = true;
                }
            }
            int spaceX = getSpaceX(wid);
            int spaceY = getSpaceY(wid);
            QSize itemSizeHint = item->sizeHint();
            currentLineHeight = linesHeight.at(lineIndex);
            int itemWidth = itemSizeHint.width();
            int nextX = columnX + itemWidth + spaceX;

            bool doBreak = false;
            if (checkForNextBreak){
                int nextWidgetNextX = nextX + itemNext->sizeHint().width();
                doBreak = nextWidgetNextX > effectiveRight;
            }
            int widgetEndX = nextX - spaceX;

            if ((widgetEndX > effectiveRight) || doBreak){
                columnX = effectiveRect.x();
                rowY = rowY + currentLineHeight + spaceY;
                nextY = rowY;
                if (fullWidthItems.find(i) != fullWidthItems.end()){
                    itemWidth = effectiveRect.width();
                    nextY = rowY + currentLineHeight + spaceY;
                    nextX = columnX;
                } else {
                    if (firstColumnWidth > 0){
                        itemWidth = firstColumnWidth;
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
    if (spacingX == -1)
        spacingX = wid->style()->layoutSpacing(
            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
    return spacingX;
}

int LC_FlexLayout::getSpaceX(const QWidget *wid) const{
    int spacingY = horizontalSpacing();
    if (spacingY == -1)
        spacingY = wid->style()->layoutSpacing(
            QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    return spacingY;
}

int LC_FlexLayout::defaultSpacing(QStyle::PixelMetric pm) const{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        auto *pw = dynamic_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    } else {
        return dynamic_cast<QLayout *>(parent)->spacing();
    }
}
int LC_FlexLayout::horizontalSpacing() const{
    if (hSpacing >= 0) {
        return hSpacing;
    } else {
        return defaultSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int LC_FlexLayout::verticalSpacing() const{
    if (vSpacing >= 0) {
        return vSpacing;
    } else {
        return defaultSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}
QLayoutItem*LC_FlexLayout::takeAt(int index){
    QLayoutItem* result = nullptr;
    if (index >= 0 && index < items.size()){
        result = items.takeAt(index);
    }
    return result;
}

Qt::Orientations LC_FlexLayout::expandingDirections() const{
    return {};
}


QSize LC_FlexLayout::minimumSize() const{
    QSize size;
    for (const QLayoutItem *item : std::as_const(items))
        size = size.expandedTo(item->minimumSize());

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

void LC_FlexLayout::addItem(QLayoutItem *item){
    items.append(item);
}


int LC_FlexLayout::count() const{
    return items.size();
}

QLayoutItem *LC_FlexLayout::itemAt(int index) const{
    return items.value(index);
}


