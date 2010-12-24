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

#include "qg_widthbox.h"

#include <qpixmap.h>

#include "rs_debug.h"

#include "ui/width00.xpm"
#include "ui/width01.xpm"
#include "ui/width02.xpm"
#include "ui/width03.xpm"
#include "ui/width04.xpm"
#include "ui/width05.xpm"
#include "ui/width06.xpm"
#include "ui/width07.xpm"
#include "ui/width08.xpm"
#include "ui/width09.xpm"
#include "ui/width10.xpm"
#include "ui/width11.xpm"
#include "ui/width12.xpm"


/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_WidthBox::QG_WidthBox(QWidget* parent, const char* name)
        : QComboBox(parent, name) {

    showByLayer = false;
    showUnchanged = false;
    unchanged = false;
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing widths.
 *
 * @param showByLayer true: Show attributes ByLayer, ByBlock
 */
QG_WidthBox::QG_WidthBox(bool showByLayer, bool showUnchanged,
                         QWidget* parent, const char* name)
        : QComboBox(parent, name) {
    init(showByLayer, showUnchanged);
}


/**
 * Destructor
 */
QG_WidthBox::~QG_WidthBox() {}


/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attributes ByLayer, ByBlock
 */
void QG_WidthBox::init(bool showByLayer, bool showUnchanged) {
    this->showByLayer = showByLayer;
	this->showUnchanged = showUnchanged;

    if (showUnchanged) {
        insertItem(QPixmap(width00_xpm), tr("- Unchanged -"));
    }
    if (showByLayer) {
        insertItem(QPixmap(width00_xpm), tr("By Layer"));
        insertItem(QPixmap(width00_xpm), tr("By Block"));
    }
    insertItem(QPixmap(width01_xpm), tr("Default"));
    insertItem(QPixmap(width01_xpm), tr("0.00mm"));
    insertItem(QPixmap(width01_xpm), tr("0.05mm"));
    insertItem(QPixmap(width01_xpm), tr("0.09mm"));
    insertItem(QPixmap(width01_xpm), tr("0.13mm (ISO)"));
    insertItem(QPixmap(width01_xpm), tr("0.15mm"));
    insertItem(QPixmap(width01_xpm), tr("0.18mm (ISO)"));
    insertItem(QPixmap(width01_xpm), tr("0.20mm"));
    insertItem(QPixmap(width01_xpm), tr("0.25mm (ISO)"));
    insertItem(QPixmap(width01_xpm), tr("0.30mm"));
    insertItem(QPixmap(width03_xpm), tr("0.35mm (ISO)"));
    insertItem(QPixmap(width03_xpm), tr("0.40mm"));
    insertItem(QPixmap(width04_xpm), tr("0.50mm (ISO)"));
    insertItem(QPixmap(width05_xpm), tr("0.53mm"));
    insertItem(QPixmap(width05_xpm), tr("0.60mm"));
    insertItem(QPixmap(width06_xpm), tr("0.70mm (ISO)"));
    insertItem(QPixmap(width07_xpm), tr("0.80mm"));
    insertItem(QPixmap(width08_xpm), tr("0.90mm"));
    insertItem(QPixmap(width09_xpm), tr("1.00mm (ISO)"));
    insertItem(QPixmap(width10_xpm), tr("1.06mm"));
    insertItem(QPixmap(width10_xpm), tr("1.20mm"));
    insertItem(QPixmap(width12_xpm), tr("1.40mm (ISO)"));
    insertItem(QPixmap(width12_xpm), tr("1.58mm"));
    insertItem(QPixmap(width12_xpm), tr("2.00mm (ISO)"));
    insertItem(QPixmap(width12_xpm), tr("2.11mm"));

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotWidthChanged(int)));

    setCurrentItem(0);
    slotWidthChanged(currentItem());
}

/**
 * Sets the currently selected width item to the given width.
 */
void QG_WidthBox::setWidth(RS2::LineWidth w) {

    RS_DEBUG->print("QG_WidthBox::setWidth %d\n", (int)w);

    int offset = (int)showByLayer*2 + (int)showUnchanged;

    switch (w) {
    case RS2::WidthByLayer:
        if (showByLayer) {
            setCurrentItem(0 + (int)showUnchanged);
        } else {
        	RS_DEBUG->print(RS_Debug::D_WARNING,
            	"QG_WidthBox::setWidth: Unsupported width.");
        }
        break;
    case RS2::WidthByBlock:
        if (showByLayer) {
            setCurrentItem(1 + (int)showUnchanged);
        } else {
        	RS_DEBUG->print(RS_Debug::D_WARNING,
            	"QG_WidthBox::setWidth: Unsupported width.");
        }
        break;
    case RS2::WidthDefault:
        setCurrentItem(0 + offset);
        break;
    case RS2::Width00:
        setCurrentItem(1 + offset);
        break;
    case RS2::Width01:
        setCurrentItem(2 + offset);
        break;
    case RS2::Width02:
        setCurrentItem(3 + offset);
        break;
    case RS2::Width03:
        setCurrentItem(4 + offset);
        break;
    case RS2::Width04:
        setCurrentItem(5 + offset);
        break;
    case RS2::Width05:
        setCurrentItem(6 + offset);
        break;
    case RS2::Width06:
        setCurrentItem(7 + offset);
        break;
    case RS2::Width07:
        setCurrentItem(8 + offset);
        break;
    case RS2::Width08:
        setCurrentItem(9 + offset);
        break;
    case RS2::Width09:
        setCurrentItem(10 + offset);
        break;
    case RS2::Width10:
        setCurrentItem(11 + offset);
        break;
    case RS2::Width11:
        setCurrentItem(12 + offset);
        break;
    case RS2::Width12:
        setCurrentItem(13 + offset);
        break;
    case RS2::Width13:
        setCurrentItem(14 + offset);
        break;
    case RS2::Width14:
        setCurrentItem(15 + offset);
        break;
    case RS2::Width15:
        setCurrentItem(16 + offset);
        break;
    case RS2::Width16:
        setCurrentItem(17 + offset);
        break;
    case RS2::Width17:
        setCurrentItem(18 + offset);
        break;
    case RS2::Width18:
        setCurrentItem(19 + offset);
        break;
    case RS2::Width19:
        setCurrentItem(20 + offset);
        break;
    case RS2::Width20:
        setCurrentItem(21 + offset);
        break;
    case RS2::Width21:
        setCurrentItem(22 + offset);
        break;
    case RS2::Width22:
        setCurrentItem(23 + offset);
        break;
    case RS2::Width23:
        setCurrentItem(24 + offset);
        break;
    default:
        break;
    }

    slotWidthChanged(currentItem());
}



/**
 * Sets the pixmap showing the width of the "By Layer" item.
 */
void QG_WidthBox::setLayerWidth(RS2::LineWidth w) {
    if (showByLayer) {
        QPixmap pixmap;
        switch(w) {
        default:
        case RS2::Width00:
            pixmap = QPixmap(width00_xpm);
            break;
        case RS2::Width01:
        case RS2::Width02:
            pixmap = QPixmap(width01_xpm);
            break;
        case RS2::Width03:
        case RS2::Width04:
            pixmap = QPixmap(width02_xpm);
            break;
        case RS2::Width05:
        case RS2::Width06:
            pixmap = QPixmap(width03_xpm);
            break;
        case RS2::Width07:
        case RS2::Width08:
            pixmap = QPixmap(width04_xpm);
            break;
        case RS2::Width09:
        case RS2::Width10:
            pixmap = QPixmap(width05_xpm);
            break;
        case RS2::Width11:
        case RS2::Width12:
            pixmap = QPixmap(width06_xpm);
            break;
        case RS2::Width13:
        case RS2::Width14:
            pixmap = QPixmap(width07_xpm);
            break;
        case RS2::Width15:
        case RS2::Width16:
            pixmap = QPixmap(width08_xpm);
            break;
        case RS2::Width17:
        case RS2::Width18:
            pixmap = QPixmap(width09_xpm);
            break;
        case RS2::Width19:
        case RS2::Width20:
            pixmap = QPixmap(width10_xpm);
            break;
        case RS2::Width21:
        case RS2::Width22:
            pixmap = QPixmap(width11_xpm);
            break;
        case RS2::Width23:
            //case RS2::Width24:
            pixmap = QPixmap(width12_xpm);
            break;
        }

        changeItem(pixmap, tr("By Layer"), 0);

        // needed for the first time a layer is added:
        slotWidthChanged(currentItem());
    }
}

/**
 * Called when the width has changed. This method 
 * sets the current width to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual width.
 */
void QG_WidthBox::slotWidthChanged(int index) {

    RS_DEBUG->print("QG_WidthBox::slotWidthChanged %d\n", index);

    bool done = false;

    if (showUnchanged && index==0) {
        unchanged = true;
        done = true;
    } else {
        unchanged = false;
    }

    if (!done && showByLayer) {
        if (index==0 + (int)showUnchanged) {
            currentWidth = RS2::WidthByLayer;
            done = true;
        } else if (index==1 + (int)showUnchanged) {
            currentWidth = RS2::WidthByBlock;
            done = true;
        }
    }

    if (!done) {
        switch (index-((int)showByLayer*2)-((int)showUnchanged)) {
        case 0:
            currentWidth = RS2::WidthDefault;
            break;
        case 1:
            currentWidth = RS2::Width00;
            break;
        case 2:
            currentWidth = RS2::Width01;
            break;
        case 3:
            currentWidth = RS2::Width02;
            break;
        case 4:
            currentWidth = RS2::Width03;
            break;
        case 5:
            currentWidth = RS2::Width04;
            break;
        case 6:
            currentWidth = RS2::Width05;
            break;
        case 7:
            currentWidth = RS2::Width06;
            break;
        case 8:
            currentWidth = RS2::Width07;
            break;
        case 9:
            currentWidth = RS2::Width08;
            break;
        case 10:
            currentWidth = RS2::Width09;
            break;
        case 11:
            currentWidth = RS2::Width10;
            break;
        case 12:
            currentWidth = RS2::Width11;
            break;
        case 13:
            currentWidth = RS2::Width12;
            break;
        case 14:
            currentWidth = RS2::Width13;
            break;
        case 15:
            currentWidth = RS2::Width14;
            break;
        case 16:
            currentWidth = RS2::Width15;
            break;
        case 17:
            currentWidth = RS2::Width16;
            break;
        case 18:
            currentWidth = RS2::Width17;
            break;
        case 19:
            currentWidth = RS2::Width18;
            break;
        case 20:
            currentWidth = RS2::Width19;
            break;
        case 21:
            currentWidth = RS2::Width20;
            break;
        case 22:
            currentWidth = RS2::Width21;
            break;
        case 23:
            currentWidth = RS2::Width22;
            break;
        case 24:
            currentWidth = RS2::Width23;
            break;
        default:
            break;
        }
    }
    //currentWidth = (RS2::LineWidth)(index-1);
    //currentWidth = (RS2::LineWidth)(currentText().left(4).toDouble()*100);
    //}

    RS_DEBUG->print("Current width is (%d): %d\n",
                    index, ((int)currentWidth));

    emit widthChanged(currentWidth);
}

// EOF
