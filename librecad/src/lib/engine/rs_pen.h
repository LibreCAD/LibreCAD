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
     * Creates a default pen (black, solid, width 0).
     */
    RS_Pen() : RS_Flags() {
        setColor(RS_Color(0,0,0));
        setWidth(RS2::Width00);
        setLineType(RS2::SolidLine);
		setScreenWidth(0);
    }
    /**
     * Creates a pen with the given attributes.
     */
    RS_Pen(const RS_Color& c,
           RS2::LineWidth w,
           RS2::LineType t) : RS_Flags() {
        setColor(c);
        setWidth(w);
        setLineType(t);
		setScreenWidth(0);
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
        setColor(RS_Color(0,0,0));
        setWidth(RS2::Width00);
        setLineType(RS2::SolidLine);
		setScreenWidth(0);
    }
    //RS_Pen(const RS_Pen& pen) : RS_Flags(pen.getFlags()) {
    //    lineType = pen.lineType;
    //    width = pen.width;
    //    color = pen.color;
    //}
    virtual ~RS_Pen() {}

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
    const RS_Color& getColor() const {
        return color;
    }
    void setColor(const RS_Color& c) {
        color = c;
    }
    bool isValid() {
        return !getFlag(RS2::FlagInvalid);
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

    bool operator != (const RS_Pen& p) const {
        return !(*this==p);
    }

    friend std::ostream& operator << (std::ostream& os, const RS_Pen& p);


protected:
    RS2::LineType lineType;
    RS2::LineWidth width;
	double screenWidth;
    RS_Color color;
};

#endif
