/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "qg_linetypebox.h"

#include <qpixmap.h>

#include "rs_debug.h"

#include "ui/linetype00.xpm"
#include "ui/linetype01.xpm"
#include "ui/linetype02.xpm"
#include "ui/linetype03.xpm"
#include "ui/linetype04.xpm"
#include "ui/linetype05.xpm"
#include "ui/linetype06.xpm"
#include "ui/linetype07.xpm"
//#include "ui/linetype08.xpm"
//#include "ui/linetype09.xpm"
//#include "ui/linetype10.xpm"
//#include "ui/linetype11.xpm"
//#include "ui/linetype12.xpm"


/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_LineTypeBox::QG_LineTypeBox(QWidget* parent, const char* name)
        : QComboBox(parent, name) {

    showByLayer = false;
	showUnchanged = false;
	unchanged = false;
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing linetypes.
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
QG_LineTypeBox::QG_LineTypeBox(bool showByLayer, bool showUnchanged, 
		QWidget* parent, const char* name)
        : QComboBox(parent, name) {
	unchanged = false;
    init(showByLayer, showUnchanged);
}


/**
 * Destructor
 */
QG_LineTypeBox::~QG_LineTypeBox() {}


/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
void QG_LineTypeBox::init(bool showByLayer, bool showUnchanged) {
    this->showByLayer = showByLayer;
	this->showUnchanged = showUnchanged;

    if (showUnchanged) {
        insertItem(QPixmap(linetype00_xpm), tr("- Unchanged -"));
	}

    if (showByLayer) {
        insertItem(QPixmap(linetype00_xpm), tr("By Layer"));
        insertItem(QPixmap(linetype00_xpm), tr("By Block"));
    }
    insertItem(QPixmap(linetype00_xpm), tr("No Pen"));
    insertItem(QPixmap(linetype01_xpm), tr("Continuous"));
    insertItem(QPixmap(linetype02_xpm), tr("Dot"));
    insertItem(QPixmap(linetype02_xpm), tr("Dot (small)"));
    insertItem(QPixmap(linetype02_xpm), tr("Dot (large)"));
    insertItem(QPixmap(linetype03_xpm), tr("Dash"));
    insertItem(QPixmap(linetype03_xpm), tr("Dash (small)"));
    insertItem(QPixmap(linetype03_xpm), tr("Dash (large)"));
    insertItem(QPixmap(linetype04_xpm), tr("Dash Dot"));
    insertItem(QPixmap(linetype04_xpm), tr("Dash Dot (small)"));
    insertItem(QPixmap(linetype04_xpm), tr("Dash Dot (large)"));
    insertItem(QPixmap(linetype05_xpm), tr("Divide"));
    insertItem(QPixmap(linetype05_xpm), tr("Divide (small)"));
    insertItem(QPixmap(linetype05_xpm), tr("Divide (large)"));
    insertItem(QPixmap(linetype06_xpm), tr("Center"));
    insertItem(QPixmap(linetype06_xpm), tr("Center (small)"));
    insertItem(QPixmap(linetype06_xpm), tr("Center (large)"));
    insertItem(QPixmap(linetype07_xpm), tr("Border"));
    insertItem(QPixmap(linetype07_xpm), tr("Border (small)"));
    insertItem(QPixmap(linetype07_xpm), tr("Border (large)"));

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotLineTypeChanged(int)));

    setCurrentItem(0);
    slotLineTypeChanged(currentItem());
}

/**
 * Sets the currently selected linetype item to the given linetype.
 */
void QG_LineTypeBox::setLineType(RS2::LineType t) {

    RS_DEBUG->print("QG_LineTypeBox::setLineType %d\n", (int)t);

	int offset = (int)showByLayer*2 + (int)showUnchanged;

    switch (t) {
    case RS2::LineByLayer:
        if (showByLayer) {
            setCurrentItem(0 + (int)showUnchanged);
        } else {
        	RS_DEBUG->print(RS_Debug::D_WARNING,
            	"QG_LineTypeBox::setLineType: "
				"Combobox doesn't support linetype BYLAYER");
        }
        break;
    case RS2::LineByBlock:
        if (showByLayer) {
            setCurrentItem(1 + (int)showUnchanged);
        } else {
        	RS_DEBUG->print(RS_Debug::D_WARNING,
            	"QG_LineTypeBox::setLineType: "
				"Combobox doesn't support linetype BYBLOCK");
        }
        break;
		
	case RS2::SolidLine:
		setCurrentItem(1 + offset);
		break;

	case RS2::DotLine:
		setCurrentItem(2 + offset);
		break;
	case RS2::DotLine2:
		setCurrentItem(3 + offset);
		break;
	case RS2::DotLineX2:
		setCurrentItem(4 + offset);
		break;

	case RS2::DashLine:
		setCurrentItem(5 + offset);
		break;
	case RS2::DashLine2:
		setCurrentItem(6 + offset);
		break;
	case RS2::DashLineX2:
		setCurrentItem(7 + offset);
		break;
	
	case RS2::DashDotLine:
		setCurrentItem(8 + offset);
		break;
	case RS2::DashDotLine2:
		setCurrentItem(9 + offset);
		break;
	case RS2::DashDotLineX2:
		setCurrentItem(10 + offset);
		break;
	
	case RS2::DivideLine:
		setCurrentItem(11 + offset);
		break;
	case RS2::DivideLine2:
		setCurrentItem(12 + offset);
		break;
	case RS2::DivideLineX2:
		setCurrentItem(13 + offset);
		break;
	
	case RS2::CenterLine:
		setCurrentItem(14 + offset);
		break;
	case RS2::CenterLine2:
		setCurrentItem(15 + offset);
		break;
	case RS2::CenterLineX2:
		setCurrentItem(16 + offset);
		break;
	
	case RS2::BorderLine:
		setCurrentItem(17 + offset);
		break;
	case RS2::BorderLine2:
		setCurrentItem(18 + offset);
		break;
	case RS2::BorderLineX2:
		setCurrentItem(19 + offset);
		break;
	
    default:
        break;
    }

    slotLineTypeChanged(currentItem());
}

/**
 * Sets the pixmap showing the linetype of the "By Layer" item.
 *
 * @todo needs an update, but not used currently
 */
void QG_LineTypeBox::setLayerLineType(RS2::LineType t) {
    if (showByLayer) {
        QPixmap pixmap;
        switch(t) {
        case RS2::NoPen:
            pixmap = QPixmap(linetype00_xpm);
            break;
        default:
        case RS2::SolidLine:
            pixmap = QPixmap(linetype01_xpm);
            break;
        case RS2::DashLine:
            pixmap = QPixmap(linetype02_xpm);
            break;
        case RS2::DotLine:
            pixmap = QPixmap(linetype03_xpm);
            break;
        case RS2::DashDotLine:
            pixmap = QPixmap(linetype04_xpm);
            break;
        case RS2::DivideLine:
            pixmap = QPixmap(linetype05_xpm);
            break;
        }

        changeItem(pixmap, tr("By Layer"), 0);

        // needed for the first time a layer is added:
        slotLineTypeChanged(currentItem());
    }
}

/**
 * Called when the linetype has changed. This method 
 * sets the current linetype to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual linetype.
 */
void QG_LineTypeBox::slotLineTypeChanged(int index) {

    RS_DEBUG->print("QG_LineTypeBox::slotLineTypeChanged %d\n", index);
	
	unchanged = false;

    if (showByLayer) {
        switch (index) {
        case 0:
			if (showUnchanged) {
				unchanged = true;
			}
			else {
            	currentLineType = RS2::LineByLayer;
			}
            break;

        case 1:
			if (showUnchanged) {
				currentLineType = RS2::LineByLayer;
			}
			else {
            	currentLineType = RS2::LineByBlock;
			}
            break;
			
        default:
            currentLineType = (RS2::LineType)(index-2-(int)showUnchanged);
            break;
        }
    } else {
        currentLineType = 
			(RS2::LineType)(index-(int)showByLayer*2-(int)showUnchanged);
    }

    RS_DEBUG->print("Current linetype is (%d): %d\n",
                    index, currentLineType);

    emit lineTypeChanged(currentLineType);
}


