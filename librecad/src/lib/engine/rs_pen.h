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


#ifndef RS_PEN_H
#define RS_PEN_H

#include "lc_linemath.h"
#include "rs.h"
#include "rs_color.h"
#include "rs_flags.h"


/**
 * A pen stores attributes for painting such as line width,
 * linetype, color, ...
 *
 * @author Andrew Mustun
 */
class RS_Pen : public RS_Flags {
public:
    /**
     * Creates a default pen (color, width, type) by layer
     */
    RS_Pen();

    /**
     * Creates a pen with the given attributes.
     */
    RS_Pen(const RS_Color& color, const RS2::LineWidth width, const RS2::LineType type) {
        setColor(color);
        setWidth(width);
        setLineType(type);
    }

    /**
     * Creates a default pen with the given flags. This is 
     * usually used to create invalid pens.
     *
     * e.g.:
     * <pre>
     *   RS_Pen p(RS2::FlagInvalid);
     * </pre>
     */
    explicit RS_Pen(const unsigned int f) : RS_Flags(f) {}
    //RS_Pen(const RS_Pen& pen) : RS_Flags(pen.getFlags()) {
    //    lineType = pen.lineType;
    //    width = pen.width;
    //    color = pen.color;
    //}

    RS2::LineType getLineType() const {
        return m_lineType;
    }
    void setLineType(const RS2::LineType t) {
        m_lineType = t;
    }
    RS2::LineWidth getWidth() const {
        return m_width;
    }
    void setWidth(const RS2::LineWidth w) {
        m_width = w;
    }
    double getScreenWidth() const {
        return m_screenWidth;
    }
    void setScreenWidth(const double w) {
        m_screenWidth = w;
    }

    RS_Color getColor() const {
        return m_color;
    }

    void setColor(const RS_Color& c) {
        m_color = c;
    }

    void setColorFromPen(const RS_Pen& pen){
        m_color = pen.m_color;
    }

    void setWidthFromPen(const RS_Pen& pen){
        m_width = pen.m_width;
    }

    void setLineTypeFromPen(const RS_Pen& pen){
        m_lineType = pen.m_lineType;
    }

    bool isColorByLayer() const {
        return m_color.getFlag(RS2::FlagByLayer);
    }

    bool isColorByBlock() const {
        return m_color.getFlag(RS2::FlagByBlock);
    }

    bool isWidthByLayer() const {
        return m_width == RS2::WidthByLayer;
    }

    bool isWidthByBlock() const {
        return m_width == RS2::WidthByBlock;
    }

    bool isLineTypeByLayer() const {
        return m_lineType == RS2::LineByLayer;
    }

    bool isLineTypeByBlock() const {
        return m_lineType == RS2::LineByBlock;
    }

    bool hasByLayerAttributes() const {
        return m_color.getFlag(RS2::FlagByLayer) ||  m_width == RS2::WidthByLayer || m_lineType == RS2::LineByLayer;
    }

    bool isValid() const {
        return !getFlag(RS2::FlagInvalid);
    }

    float getAlpha() const {
        return m_alpha;
    }
    void setAlpha(const float a) {
        m_alpha = a;
    }

    bool isFullyOpaque() const {
        return std::abs(m_alpha - 1.0) < RS_TOLERANCE;
    }

    //RS_Pen& operator = (const RS_Pen& p) {
    //    lineType = p.lineType;
    //    width = p.width;
    //    color = p.color;
    //    setFlags(p.getFlags());

    //    return *this;
    //}

    bool operator == (const RS_Pen& p) const {
        return m_lineType==p.m_lineType && m_width==p.m_width && m_color==p.m_color;
    }

    bool isSameAs(const RS_Pen& p, const double patternOffset) const {
        return m_lineType == p.m_lineType && m_width == p.m_width && m_color == p.m_color && LC_LineMath::isSameLength(m_alpha, p.m_alpha) &&
            LC_LineMath::isSameLength(m_dashOffset, patternOffset) && !getFlag(RS2::FlagInvalid);
    }

    void updateBy(const RS_Pen & p){
        m_color = p.m_color;
        m_lineType = p.m_lineType;
        m_width = p.m_width;
        m_alpha = p.m_alpha;
        m_dashOffset = p.m_dashOffset;
        setFlags(p.getFlags());
        delFlag(RS2::FlagInvalid);
    }

    bool operator != (const RS_Pen& p) const {
        return !(*this==p);
    }

    // accessor/mutator for dash pattern offset
    void setDashOffset(const double offset){
        m_dashOffset = offset;
    }

    double dashOffset() const
    {
        return m_dashOffset;
    }

    friend std::ostream& operator << (std::ostream& os, const RS_Pen& p);


private:
    RS2::LineType m_lineType = RS2::SolidLine;
    RS2::LineWidth m_width = RS2::Width00;
    double m_screenWidth = 0.;
    RS_Color m_color;
    float m_alpha = 1.;
    double m_dashOffset = 0.;
};

#endif
