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

#include "qg_linetypebox.h"

#include "rs_debug.h"

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_LineTypeBox::QG_LineTypeBox(QWidget* parent)
        : QComboBox(parent) {

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
        : QComboBox(parent) {
    setObjectName(name);
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
        addItem(QIcon(":ui/linetype00.png"), tr("- Unchanged -"), RS2::LineTypeUnchanged);
	}

    if (showByLayer) {
        addItem(QIcon(":ui/linetype00.png"), tr("By Layer"), RS2::LineByLayer);
        addItem(QIcon(":ui/linetype00.png"), tr("By Block"), RS2::LineByBlock);
    }
    addItem(QIcon(":ui/linetype00.png"), tr("No Pen"),RS2::NoPen);
    addItem(QIcon(":ui/linetype01.png"), tr("Continuous"), RS2::SolidLine);
    addItem(QIcon(":ui/linetype02.png"), tr("Dot"),RS2::DotLine);
    addItem(QIcon(":ui/linetype02.png"), tr("Dot (tiny)"),RS2::DotLineTiny);
    addItem(QIcon(":ui/linetype02.png"), tr("Dot (small)"), RS2::DotLine2);
    addItem(QIcon(":ui/linetype02.png"), tr("Dot (large)"), RS2::DotLineX2);
    addItem(QIcon(":ui/linetype03.png"), tr("Dash"),RS2::DashLine);
    addItem(QIcon(":ui/linetype03.png"), tr("Dash (tiny)"),RS2::DashLineTiny);
    addItem(QIcon(":ui/linetype03.png"), tr("Dash (small)"),RS2::DashLine2);
    addItem(QIcon(":ui/linetype03.png"), tr("Dash (large)"),RS2::DashLineX2);
    addItem(QIcon(":ui/linetype04.png"), tr("Dash Dot"),RS2::DashDotLine);
    addItem(QIcon(":ui/linetype04.png"), tr("Dash Dot (tiny)"),RS2::DashDotLineTiny);
    addItem(QIcon(":ui/linetype04.png"), tr("Dash Dot (small)"),RS2::DashDotLine2);
    addItem(QIcon(":ui/linetype04.png"), tr("Dash Dot (large)"),RS2::DashDotLineX2);
    addItem(QIcon(":ui/linetype05.png"), tr("Divide"),RS2::DivideLine);
    addItem(QIcon(":ui/linetype05.png"), tr("Divide (tiny)"),RS2::DivideLineTiny);
    addItem(QIcon(":ui/linetype05.png"), tr("Divide (small)"),RS2::DivideLine2);
    addItem(QIcon(":ui/linetype05.png"), tr("Divide (large)"),RS2::DivideLineX2);
    addItem(QIcon(":ui/linetype06.png"), tr("Center"),RS2::CenterLine);
    addItem(QIcon(":ui/linetype06.png"), tr("Center (tiny)"),RS2::CenterLineTiny);
    addItem(QIcon(":ui/linetype06.png"), tr("Center (small)"),RS2::CenterLine2);
    addItem(QIcon(":ui/linetype06.png"), tr("Center (large)"),RS2::CenterLineX2);
    addItem(QIcon(":ui/linetype07.png"), tr("Border"),RS2::BorderLine);
    addItem(QIcon(":ui/linetype07.png"), tr("Border (tiny)"),RS2::BorderLineTiny);
    addItem(QIcon(":ui/linetype07.png"), tr("Border (small)"),RS2::BorderLine2);
    addItem(QIcon(":ui/linetype07.png"), tr("Border (large)"),RS2::BorderLineX2);

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotLineTypeChanged(int)));

    setCurrentIndex(0);
    slotLineTypeChanged(currentIndex());
}

/**
 * Sets the currently selected linetype item to the given linetype.
 */
void QG_LineTypeBox::setLineType(RS2::LineType t) {

    RS_DEBUG->print("QG_LineTypeBox::setLineType %d\n", (int)t);

    switch (t) {
    case RS2::LineByLayer:
        if (showByLayer) {
            setCurrentIndex(0 + (int)showUnchanged);
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "QG_LineTypeBox::setLineType: "
                            "Combobox doesn't support linetype BYLAYER");
        }
        break;
    case RS2::LineByBlock:
        if (showByLayer) {
            setCurrentIndex(1 + (int)showUnchanged);
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "QG_LineTypeBox::setLineType: "
                            "Combobox doesn't support linetype BYBLOCK");
        }
        break;
    default:{
        int index=findData(t);
        if(t>=0){
            setCurrentIndex(index);
        }else{
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "QG_LineTypeBox::setLineType: "
                            "Combobox doesn't support linetype %d",(int) t);
        }
    }

    }

    slotLineTypeChanged(currentIndex());
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
            pixmap = QPixmap(":ui/linetype00.png");
            break;
        default:
        case RS2::SolidLine:
            pixmap = QPixmap(":ui/linetype01.png");
            break;
        case RS2::DashLine:
            pixmap = QPixmap(":ui/linetype02.png");
            break;
        case RS2::DotLine:
            pixmap = QPixmap(":ui/linetype03.png");
            break;
        case RS2::DashDotLine:
            pixmap = QPixmap(":ui/linetype04.png");
            break;
        case RS2::DivideLine:
            pixmap = QPixmap(":ui/linetype05.png");
            break;
        }

        setItemIcon(0, QIcon(pixmap));
        setItemText(0, tr("By Layer"));

        // needed for the first time a layer is added:
        slotLineTypeChanged(currentIndex());
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
            currentLineType = (RS2::LineType) itemData(index).toInt();
        }
    } else {
        currentLineType = (RS2::LineType) itemData(index).toInt();
    }

//    RS_DEBUG->print(RS_Debug::D_ERROR, "Current linetype is (%d): %d\n", index, currentLineType);

    emit lineTypeChanged(currentLineType);
}



