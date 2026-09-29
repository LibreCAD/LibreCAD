/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2020 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_COLOR_H
#define RS_COLOR_H

#include <QColor>
#include <QString>

#include "rs.h"
#include "rs_flags.h"

//! Color defined by layer not entity
//#define C_BY_LAYER     0x00000001
//! Color defined by block not entity
//#define C_BY_BLOCK     0x00000002

/**
 * Color class.
 *
 * @author Andrew Mustun
 */
class RS_Color : public QColor, public RS_Flags {
public:
    RS_Color() = default;

    RS_Color(const int r, const int g, const int b) : QColor(r, g, b) {
    }

    RS_Color(const int r, const int g, const int b, const int a) : QColor(r, g, b, a) {
    }

    RS_Color(const QColor& c) : QColor(c) {
    }

    // not explicit, or RS_Color(unsigned int) would take a Qt::GlobalColor as flags
    // ReSharper disable once CppNonExplicitConvertingConstructor
    RS_Color(const Qt::GlobalColor color) : QColor(color) {
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    RS_Color(const unsigned int f) : RS_Flags(f) {
    }

    explicit RS_Color(const QString& name) : QColor(name) {
    }

    /** @return A copy of this color without flags. */
    RS_Color stripFlags() const {
        return {red(), green(), blue(), alpha()};
    }

    /** @return true if the color is defined by layer. */
    bool isByLayer() const {
        return getFlag(RS2::FlagByLayer);
    }

    /** @return true if the color is defined by block. */
    bool isByBlock() const {
        return getFlag(RS2::FlagByBlock);
    }

    QColor toQColor() const {
        return {red(), green(), blue(), alpha()};
    }

    //These 3 methods are used for plugins
    int toIntColor() const;
    void fromIntColor(int co);
    int colorDistance(const RS_Color& c) const;

    /**
     * WCAG 2 relative luminance of \p color: its sRGB channels linearised and weighted
     * 0.2126 R + 0.7152 G + 0.0722 B, from 0 (black) to 1 (white). Alpha is ignored.
     */
    static double relativeLuminance(const QColor& color);
    /**
     * WCAG 2 contrast ratio of two colours, (L1 + 0.05) / (L2 + 0.05) with L1 the
     * relative luminance of the lighter one: from 1 (equal luminance) to 21 (black and
     * white), symmetric in its arguments. Alpha is ignored, so pass the opaque colours as
     * painted. WCAG 2.1 asks for at least 3 for graphics and large text, 4.5 for text.
     */
    static double contrastRatio(const QColor& a, const QColor& b);

    enum {
        Black = 0,
        /**
         * Minimum acceptable distance between two colors before visibility
         * enhancement is required. Determined empirically.
         */
        MinColorDistance = 20, //< in %
    };

    bool isEqualIgnoringFlags(const RS_Color& c) const {
        return red() == c.red() && green() == c.green() && blue() == c.blue() && alpha() == c.alpha();
    }

    void applyFlags(const RS_Color& source) {
        setFlags(source.getFlags());
    }

    bool operator ==(const RS_Color& c) const {
        return (red() == c.red() && green() == c.green() && blue() == c.blue() && alpha() == c.alpha() && getFlags() == c.getFlags());
    }

    // Optional named-color tag, e.g. DWG/DXF AcDbColor "BOOK$ENTRY".
    // Passive metadata: not part of equality, not used by rendering.
    const QString &colorName() const { return m_colorName; }
    void setColorName(const QString &n) { m_colorName = n; }
    bool hasColorName() const { return !m_colorName.isEmpty(); }

    friend std::ostream& operator << (std::ostream& os, const RS_Color& c);

  private:
    QString m_colorName;
};

#endif
