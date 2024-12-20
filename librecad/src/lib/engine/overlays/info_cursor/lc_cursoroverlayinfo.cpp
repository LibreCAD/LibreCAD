/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include <QFont>
#include "lc_cursoroverlayinfo.h"
#include "rs_painter.h"
#include "rs_graphicview.h"

LC_InfoCursor::LC_InfoCursor(RS_EntityContainer *parent, const RS_Vector &coord, LC_InfoCursorOptions* cursorOverlaySettings):RS_Point(parent, RS_PointData(coord)){
    options = cursorOverlaySettings;
}

void LC_InfoCursor::draw(RS_Painter *painter, RS_GraphicView *view, [[maybe_unused]]double &patternOffset) {
    RS_Vector offset = RS_Vector(options->offset,options->offset);
    QString zone1String = zonesData->getZone1();
    painter->save();

    if (!zone1String.isEmpty()){
        RS_Color color = options->zone1Settings.color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone1Settings.fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone1String);
        double x = view->toGuiX(data.pos.x);
        double y = view->toGuiY(data.pos.y);

        y = y + offset.y /*+ size.height()*/;
        x = x - offset.x - size.width();

        QRect rect = QRect(QPoint(x, y), size);
        QRect boundingRect;
        painter->drawText(rect,  Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, zone1String, &boundingRect);
    }

    QString zone2String = zonesData->getZone2();
    if (!zone2String.isEmpty()){
        RS_Color color = options->zone2Settings.color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone2Settings.fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone2String);

        double x = view->toGuiX(data.pos.x);
        double y = view->toGuiY(data.pos.y);

        y = y + offset.y /*+ size.height()*/;
        x = x + offset.x;

        QRect rect = QRect(QPoint(x, y), size);
        QRect boundingRect;
        painter->drawText(rect,   Qt::AlignTop | Qt::AlignLeft | Qt::TextDontClip, zone2String, &boundingRect);
    }

    QString zone3String = zonesData->getZone3();
    if (!zone3String.isEmpty()){

        RS_Color color = options->zone3Settings.color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone3Settings.fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone3String);

        double x = view->toGuiX(data.pos.x);
        double y = view->toGuiY(data.pos.y);

        y = y - offset.y - size.height();
        x = x - offset.x - size.width();

        QRect rect = QRect(QPoint(x, y), size);
        QRect boundingRect;
        painter->drawText(rect, Qt::AlignBottom | Qt::AlignRight | Qt::TextDontClip, zone3String, &boundingRect);
    }

    QString zone4String = zonesData->getZone4();
    if (!zone4String.isEmpty()){
        RS_Color color = options->zone4Settings.color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone4Settings.fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone4String);

        double x = view->toGuiX(data.pos.x);
        double y = view->toGuiY(data.pos.y);

        y = y - offset.y - size.height();
        x = x + offset.x;

        QRect rect = QRect(QPoint(x, y), size);
        QRect boundingRect;
        painter->drawText(rect, Qt::AlignBottom | Qt::AlignLeft | Qt::TextDontClip, zone4String, &boundingRect);
    }
    painter->restore();
}

void LC_InfoCursor::clear() {
    if (zonesData != nullptr){
        zonesData->clear();
    }
}

LC_InfoCursorData *LC_InfoCursor::getZonesData() const {
    return zonesData;
}

void LC_InfoCursor::setZonesData(LC_InfoCursorData *data) {
   zonesData = data;
}

LC_InfoCursorOptions *LC_InfoCursor::getOptions() const {
    return options;
}

void LC_InfoCursor::setOptions(LC_InfoCursorOptions *options) {
    LC_InfoCursor::options = options;
}
