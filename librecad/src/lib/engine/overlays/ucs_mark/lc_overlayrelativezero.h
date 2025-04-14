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

#ifndef LC_OVERLAYRELATIVEZERO_H
#define LC_OVERLAYRELATIVEZERO_H

#include "lc_overlayentity.h"
#include "rs_color.h"
#include "rs_vector.h"

struct LC_OverlayRelZeroOptions{
    bool hideRelativeZero = false;
    int m_relativeZeroRadius  = 2;
    RS_Color m_colorRelativeZero = Qt::red;

    void loadSettings();
};

class LC_OverlayRelativeZero:public LC_OverlayDrawable{
  public:
    LC_OverlayRelativeZero(const RS_Vector &wcsPosition, LC_OverlayRelZeroOptions *options);
    LC_OverlayRelativeZero(LC_OverlayRelZeroOptions *options);
    void draw(RS_Painter *painter) override;
    void setPos(const RS_Vector &pos);
protected:
    RS_Vector wcsPosition;
    LC_OverlayRelZeroOptions* options;
};

#endif // LC_OVERLAYRELATIVEZERO_H
