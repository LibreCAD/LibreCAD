/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2026 LibreCAD (librecad.org)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
* USA
**
**********************************************************************/

#include "lc_colornumbers.h"

#include <cstdlib>

#include "drw_objects.h"
#include "rs_debug.h"

/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the dxflib documentation for details.
 *
 * @param num Color number.
 */
RS_Color LC_ColorNumbers::numberToColor(int num) {
  if (num == 0) {
    return RS_Color(RS2::FlagByBlock);
  } else if (num == 256) {
    return RS_Color(RS2::FlagByLayer);
  } else if (num <= 255 && num >= 0) {
    return RS_Color(DRW::dxfColors[num][0], DRW::dxfColors[num][1],
                    DRW::dxfColors[num][2]);
  } else {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_FilterDXF::numberToColor: Invalid color number given.");
    return RS_Color(RS2::FlagByLayer);
  }

  return RS_Color();
}

/**
 * Converts a color into a color number in the DXF palette.
 * The color that fits best is chosen.
 */
int LC_ColorNumbers::colorToNumber(const RS_Color &col, int *rgb) {
  // printf("Searching color for %s\n", col.name().toLatin1().data());
  *rgb = -1;
  // Special color BYBLOCK:
  if (col.getFlag(RS2::FlagByBlock)) {
    return 0;
  }
  // Special color BYLAYER
  else if (col.getFlag(RS2::FlagByLayer)) {
    return 256;
  }
  // Special color black is not in the table but white represents both
  // black and white
  else {
    int red = col.red();
    int green = col.green();
    int blue = col.blue();
    if (red == 0 && green == 0 && blue == 0) {
      return 7;
    }
    // All other colors
    else {
      int num = 0;
      int diff =
          255 * 3; // smallest difference to a color in the table found so far

      // Run through the whole table and compare
      for (int i = 1; i <= 255; i++) {
        int d = abs(red - DRW::dxfColors[i][0]) +
                abs(green - DRW::dxfColors[i][1]) +
                abs(blue - DRW::dxfColors[i][2]);

        if (d < diff) {
          /*
              printf("color %f,%f,%f is closer\n",
                     dxfColors[i][0],
                     dxfColors[i][1],
                     dxfColors[i][2]);
              */
          diff = d;
          num = i;
          if (d == 0) {
            break;
          }
        }
      }
      // printf("  Found: %d, diff: %d\n", num, diff);
      if (diff != 0) {
        *rgb = 0;
        *rgb = red << 16 | green << 8 | blue;
      }
      return num;
    }
  }
}
