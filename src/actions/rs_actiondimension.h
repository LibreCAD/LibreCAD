/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#ifndef RS_ACTIONDIMENSION_H
#define RS_ACTIONDIMENSION_H

#include "rs_previewactioninterface.h"
#include "rs_dimension.h"

/**
 * Base class for dimension actions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimension : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDimension(const char* name,
                       RS_EntityContainer& container,
                       RS_GraphicView& graphicView);
    ~RS_ActionDimension();

    virtual void reset();

    virtual void init(int status=0);

    virtual void hideOptions();
    virtual void showOptions();

    virtual void updateMouseCursor();
    virtual void updateToolBar();

    RS_String getText() {
		if (!data.text.isEmpty()) {
			return data.text;
		}
	
        RS_String l = label;

        if (l.isEmpty() &&
            (diameter==true || !tol1.isEmpty() || !tol2.isEmpty())) {
            l = "<>";
        }

        if (diameter==true) {
            l = RS_Char(0x2205) + l;
        }

        if (!tol1.isEmpty() || !tol2.isEmpty()) {
            l += RS_String("\\S%1\\%2;").arg(tol1).arg(tol2);
        }

        return l;
    }
	
    void setText(const RS_String& t) {
        data.text = t;
	}

	RS_String getLabel() {
		return label;
	}
    void setLabel(const RS_String& t) {
        //data.text = t;
        label = t;
    }
	RS_String getTol1() {
		return tol1;
	}
    void setTol1(const RS_String& t) {
        tol1 = t;
    }
	RS_String getTol2() {
		return tol2;
	}
    void setTol2(const RS_String& t) {
        tol2 = t;
    }
	bool getDiameter() {
		return diameter;
	}
    void setDiameter(bool d) {
        diameter = d;
    }

    static bool isDimensionAction(RS2::ActionType type) {
        return (type==RS2::ActionDimAligned ||
                type==RS2::ActionDimLinear ||
                type==RS2::ActionDimAngular ||
                type==RS2::ActionDimDiametric ||
                type==RS2::ActionDimRadial);
    }

protected:
    /**
     * Generic dimension data.
     */
    RS_DimensionData data;

    RS_String label;
    RS_String tol1;
    RS_String tol2;
    bool diameter;


    /**
     * Commands.
     */
    /*
      RS_String cmdText;
      RS_String cmdText2;
    */
};

#endif
