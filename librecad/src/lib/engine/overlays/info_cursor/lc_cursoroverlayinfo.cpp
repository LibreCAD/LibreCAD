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


#include "lc_cursoroverlayinfo.h"

#include <QColor>

#include "rs_painter.h"
#include "rs_settings.h"

struct LC_InfoCursorOptions::ZoneSetup{
    QColor color;
    int fontSize = 10;
    ZoneSetup(const QColor &color, int fontSize):
        color(color)
        , fontSize(fontSize)
    {}
};

struct LC_InfoCursorOptions::Impl {
    LC_InfoCursorOptions::ZoneSetup zones[4] = {{Qt::green, 10}, {Qt::cyan, 10}, {Qt::magenta, 10}, {Qt::gray, 10}};
};

LC_InfoCursorOptions::LC_InfoCursorOptions():
    m_pImpl{std::make_unique<Impl>()}
{}

LC_InfoCursorOptions::~LC_InfoCursorOptions() = default;

const LC_InfoCursorOptions::ZoneSetup& LC_InfoCursorOptions::zone(int index) const
{
    return m_pImpl->zones[index];
}

LC_InfoCursorOptions::ZoneSetup& LC_InfoCursorOptions::zone(int index)
{
    return m_pImpl->zones[index];
}

void LC_InfoCursorOptions::setFontSize(int size){
    fontSize = size;
    // todo - potentally, later we may use different font sizes for different zones?

    for(ZoneSetup& zone: m_pImpl->zones)
        zone.fontSize = size;
}

void LC_InfoCursorOverlayPrefs::loadSettings() {
    LC_GROUP("InfoOverlayCursor");
    {
        enabled = LC_GET_BOOL("Enabled", true);
        if (enabled) {
            showAbsolutePosition = LC_GET_BOOL("ShowAbsolute", true);
            showAbsolutePositionWCS = LC_GET_BOOL("ShowAbsoluteWCS", false);

            showRelativePositionDistAngle = LC_GET_BOOL("ShowRelativeDA", true);
            showRelativePositionDeltas = LC_GET_BOOL("ShowRelativeDD", true);
            showSnapType = LC_GET_BOOL("ShowSnapInfo", true);
            showCurrentActionName = LC_GET_BOOL("ShowActionName", true);
            showCommandPrompt = LC_GET_BOOL("ShowPrompt", true);
            showLabels = LC_GET_BOOL("ShowLabels", false);
            multiLine = !LC_GET_BOOL("SingleLine", true);

            showEntityInfoOnCatch = LC_GET_BOOL("ShowPropertiesCatched", true);
            showEntityInfoOnCreation = LC_GET_BOOL("ShowPropertiesCreating", true);
            showEntityInfoOnModification = LC_GET_BOOL("ShowPropertiesEdit", true);

            int infoCursorFontSize = LC_GET_INT("FontSize", 10);
            // todo - potentially, we may use different font sizes for different zones later
            options.setFontSize(infoCursorFontSize);
            options.fontName = LC_GET_STR("FontName", "Helvetica");
            options.offset = LC_GET_INT("OffsetFromCursor", 10);
        }
    }

    LC_GROUP("Colors");
    {
        if (enabled) {
            options.zone(0).color = QColor(LC_GET_STR("info_overlay_absolute", RS_Settings::overlayInfoCursorAbsolutePos));
            options.zone(1).color = QColor(LC_GET_STR("info_overlay_snap", RS_Settings::overlayInfoCursorSnap));
            options.zone(2).color = QColor(LC_GET_STR("info_overlay_relative", RS_Settings::overlayInfoCursorRelativePos));
            options.zone(3).color = QColor(LC_GET_STR("info_overlay_prompt", RS_Settings::overlayInfoCursorCommandPrompt));
        }
    }
    LC_GROUP_END();
}

LC_OverlayInfoCursor::LC_OverlayInfoCursor(const RS_Vector &coord, LC_InfoCursorOptions* cursorOverlaySettings):wcsPos(coord){
    options = cursorOverlaySettings;
}

void LC_OverlayInfoCursor::draw(RS_Painter *painter) {
    RS_Vector offset = RS_Vector(options->offset,options->offset);
    painter->save();

    double x,y;
    painter->toGui(wcsPos, x, y);

    QString zone1String = zonesData->getZone1();
    if (!zone1String.isEmpty()){
        RS_Color color = options->zone(0).color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone(0).fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone1String);

        double x0 = x - offset.x - size.width();
        double y0 = y + offset.y /*+ size.height()*/;

        QRect rect = QRect(QPoint(x0, y0), size);
        QRect boundingRect;
        painter->drawText(rect,  Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, zone1String, &boundingRect);
    }

    QString zone2String = zonesData->getZone2();
    if (!zone2String.isEmpty()){
        RS_Color color = options->zone(1).color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone(1).fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone2String);

        double x0 = x + offset.x;
        double y0 = y + offset.y /*+ size.height()*/;

        QRect rect = QRect(QPoint(x0, y0), size);
        QRect boundingRect;
        painter->drawText(rect,   Qt::AlignTop | Qt::AlignLeft | Qt::TextDontClip, zone2String, &boundingRect);
    }

    QString zone3String = zonesData->getZone3();
    if (!zone3String.isEmpty()){

        RS_Color color = options->zone(2).color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone(2).fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone3String);

        double x0 = x - offset.x - size.width();
        double  y0 = y - offset.y - size.height();

        QRect rect = QRect(QPoint(x0, y0), size);
        QRect boundingRect;
        painter->drawText(rect, Qt::AlignBottom | Qt::AlignRight | Qt::TextDontClip, zone3String, &boundingRect);
    }

    QString zone4String = zonesData->getZone4();
    if (!zone4String.isEmpty()){
        RS_Color color = options->zone(3).color;
        painter->setPen(color);

        QFont fontToUse(options->fontName, options->zone(3).fontSize);
        painter->setFont(fontToUse);

        const QSize &size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, zone4String);

        double x0 = x + offset.x;
        double y0 = y - offset.y - size.height();

        QRect rect = QRect(QPoint(x0, y0), size);
        QRect boundingRect;
        painter->drawText(rect, Qt::AlignBottom | Qt::AlignLeft | Qt::TextDontClip, zone4String, &boundingRect);
    }
    painter->restore();
}

void LC_OverlayInfoCursor::clear() {
    if (zonesData != nullptr){
        zonesData->clear();
    }
}

LC_InfoCursorData *LC_OverlayInfoCursor::getZonesData() const {
    return zonesData;
}

void LC_OverlayInfoCursor::setZonesData(LC_InfoCursorData *data) {
   zonesData = data;
}

LC_InfoCursorOptions *LC_OverlayInfoCursor::getOptions() const {
    return options;
}

void LC_OverlayInfoCursor::setOptions(LC_InfoCursorOptions *options) {
    LC_OverlayInfoCursor::options = options;
}
