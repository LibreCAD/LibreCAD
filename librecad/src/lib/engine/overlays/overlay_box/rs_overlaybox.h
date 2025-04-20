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

#ifndef RS_OVERLAYBOX_H
#define RS_OVERLAYBOX_H

#include "lc_overlayentity.h"
#include "rs_color.h"
#include "rs_vector.h"

/**
 * Holds the data that defines a line.
 */

struct LC_OverlayBoxOptions{
    LC_OverlayBoxOptions() = default;
    ~LC_OverlayBoxOptions() = default;
    RS_Color m_colorBoxLine;
    RS_Color m_colorBoxFill;
    RS_Color m_colorLineInverted;
    RS_Color m_colorBoxFillInverted;
    void loadSettings();
};

/**
 * Class for a line entity.
 *
 * @author R. van Twisk
 */
class RS_OverlayBox : public LC_OverlayDrawable {
public:
    RS_OverlayBox(const RS_Vector &corner1, const RS_Vector &corner2, LC_OverlayBoxOptions *options);
    void draw(RS_Painter* painter) override;
protected:
    RS_Vector corner1;
    RS_Vector corner2;
    LC_OverlayBoxOptions* options;
};
#endif
