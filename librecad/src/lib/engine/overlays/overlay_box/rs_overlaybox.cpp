/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
#include "rs_overlaybox.h"

#include <math.h>

#include "rs_painter.h"
#include "rs_settings.h"

void LC_OverlayBoxOptions::loadSettings() {
    LC_GROUP("Colors");
    {
        int overlayTransparency = LC_GET_INT("overlay_box_transparency",90);
        colorBoxLine = RS_Color(LC_GET_STR("overlay_box_line", RS_Settings::OVERLAY_BOX_LINE));
        auto tmp = RS_Color(LC_GET_STR("overlay_box_fill", RS_Settings::OVERLAY_BOX_FILL));
        RS_Color fillColor(tmp.red(), tmp.green(), tmp.blue(), overlayTransparency);
        colorBoxFill = fillColor;
        colorLineInverted = RS_Color(LC_GET_STR("overlay_box_line_inv", RS_Settings::OVERLAY_BOX_LINE_INVERTED));
        tmp = RS_Color(LC_GET_STR("overlay_box_fill_inv", RS_Settings::OVERLAY_BOX_FILL_INVERTED));
        RS_Color fillColorInverted(tmp.red(), tmp.green(), tmp.blue(), overlayTransparency);
        colorBoxFillInverted = fillColorInverted;
    }// colors group
    LC_GROUP_END();
    LC_GROUP("Appearance"); {
        lineType = static_cast<RS2::LineType>(LC_GET_INT("selection_overlay_line_type", RS2::SolidLine));
        invertedLineType = static_cast<RS2::LineType>(LC_GET_INT("selection_overlay_inverted_line_type", RS2::DashLine));
    }
    LC_GROUP_END();
}

RS_OverlayBox::RS_OverlayBox(const RS_Vector &corner1, const RS_Vector &corner2, LC_OverlayBoxOptions *options)
   :m_corner1(corner1), m_corner2(corner2), m_options(options) {}

void RS_OverlayBox::draw(RS_Painter* painter) {
    double v1x = NAN;
    double v1y;
    double v2x;
    double v2y;

    painter->toGui(m_corner1, v1x, v1y);
    painter->toGui(m_corner2, v2x, v2y);

    QRectF selectRect(v1x,v1y,v2x - v1x,v2y - v1y);

    if (v1x > v2x) {
        RS_Pen p(m_options->colorLineInverted, RS2::Width00, m_options->invertedLineType);
        painter->setPen(p);
        const RS_Color &fillColor = m_options->colorBoxFillInverted;
        painter->fillRect(selectRect, fillColor);
    }
    else {
        RS_Pen p(m_options->colorBoxLine, RS2::Width00, m_options->lineType);
        painter->setPen(p);
        const RS_Color &fillColor = m_options->colorBoxFill;
        painter->fillRect(selectRect, fillColor);
    }

    painter->drawRectUI(v1x, v1y, v2x, v2y);
}
