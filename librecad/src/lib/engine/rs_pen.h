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
    RS_Pen(const RS_Color& c,
           RS2::LineWidth w,
           RS2::LineType t)
    {
        setColor(c);
        setWidth(w);
        setLineType(t);
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
    RS_Pen(unsigned int f) : RS_Flags(f) {
    }
    //RS_Pen(const RS_Pen& pen) : RS_Flags(pen.getFlags()) {
    //    lineType = pen.lineType;
    //    width = pen.width;
    //    color = pen.color;
    //}

    RS2::LineType getLineType() const {
        return lineType;
    }
    void setLineType(RS2::LineType t) {
        lineType = t;
    }
    RS2::LineWidth getWidth() const {
        return width;
    }
    void setWidth(RS2::LineWidth w) {
        width = w;
    }
    double getScreenWidth() const {
        return screenWidth;
    }
    void setScreenWidth(double w) {
        screenWidth = w;
    }

    RS_Color getColor() const {
        return color;
    }

    void setColor(const RS_Color& c) {
        color = c;
    }

    inline void setColorFromPen(const RS_Pen& pen){
        color = pen.color;
    }

    inline void setWidthFromPen(const RS_Pen& pen){
        width = pen.width;
    }

    inline void setLineTypeFromPen(const RS_Pen& pen){
        lineType = pen.lineType;
    }

    inline bool isColorByLayer() const {
        return color.getFlag(RS2::FlagByLayer);
    }

    inline bool isColorByBlock() const {
        return color.getFlag(RS2::FlagByBlock);
    }

    inline bool isWidthByLayer() const {
        return width == RS2::WidthByLayer;
    }

    inline bool isWidthByBlock() const {
        return width == RS2::WidthByBlock;
    }

    inline bool isLineTypeByLayer() const {
        return lineType == RS2::LineByLayer;
    }

    inline bool isLineTypeByBlock() const {
        return lineType == RS2::LineByBlock;
    }

    bool hasByLayerAttributes() const {
        return color.getFlag(RS2::FlagByLayer) ||  width == RS2::WidthByLayer || lineType == RS2::LineByLayer;
    }

    bool isValid() const {
        return !getFlag(RS2::FlagInvalid);
    }

    float getAlpha() const {
        return alpha;
    }
    void setAlpha(float a) {
        alpha = a;
    }

    //RS_Pen& operator = (const RS_Pen& p) {
    //    lineType = p.lineType;
    //    width = p.width;
    //    color = p.color;
    //    setFlags(p.getFlags());

    //    return *this;
    //}

    bool operator == (const RS_Pen& p) const {
        return (lineType==p.lineType && width==p.width && color==p.color);
    }

    bool isSameAs(const RS_Pen &p, const double &patternOffset) const{
        return (lineType==p.lineType && width==p.width && color==p.color && alpha == p.alpha && m_dashOffset == patternOffset && !getFlag(RS2::FlagInvalid));
    }

    void updateBy(const RS_Pen & p){
        color = p.color;
        lineType = p.lineType;
        width = p.width;
        alpha = p.alpha;
        m_dashOffset = p.m_dashOffset;
        setFlags(p.getFlags());
        delFlag(RS2::FlagInvalid);
    }

    bool operator != (const RS_Pen& p) const {
        return !(*this==p);
    }

    // accessor/mutator for dash pattern offset
    void setDashOffset(double offset){
        m_dashOffset = offset;
    }

    double dashOffset() const
    {
        return m_dashOffset;
    }

    friend std::ostream& operator << (std::ostream& os, const RS_Pen& p);


private:
    RS2::LineType lineType = RS2::SolidLine;
    RS2::LineWidth width = RS2::Width00;
    double screenWidth = 0.;
    RS_Color color{};
    float alpha = 1.;
    double m_dashOffset = 0.;
};

#endif
