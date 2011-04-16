/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include "qg_colorbox.h"

#include <qcolordialog.h>
#include <qpainter.h>
#include <qpixmap.h>

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_ColorBox::QG_ColorBox(QWidget* parent, const char* name)
        : QComboBox(false, parent, name) {

    showByLayer = false;
    showUnchanged = false;
	unchanged = false;
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing colors.
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
QG_ColorBox::QG_ColorBox(bool showByLayer, bool showUnchanged, 
	QWidget* parent, const char* name)
        : QComboBox(parent, name) {
	
	unchanged = false;
    init(showByLayer, showUnchanged);
}


/**
 * Destructor
 */
QG_ColorBox::~QG_ColorBox() {}


/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
void QG_ColorBox::init(bool showByLayer, bool showUnchanged) {
    this->showByLayer = showByLayer;
    this->showUnchanged = showUnchanged;

    if (showUnchanged) {
                insertItem(QPixmap(":/ui/color00.png"), tr("Unchanged"));
	}
    if (showByLayer) {
        insertItem(QPixmap(":/ui/color00.png"), tr("By Layer"));
        insertItem(QPixmap(":/ui/color00.png"), tr("By Block"));
    }

    insertItem(QPixmap(":/ui/color01.png"), tr("Red"));
    insertItem(QPixmap(":/ui/color02.png"), tr("Yellow"));
    insertItem(QPixmap(":/ui/color03.png"), tr("Green"));
    insertItem(QPixmap(":/ui/color04.png"), tr("Cyan"));
    insertItem(QPixmap(":/ui/color05.png"), tr("Blue"));
    insertItem(QPixmap(":/ui/color06.png"), tr("Magenta"));
    insertItem(QPixmap(":/ui/color07.png"), tr("Black / White"));
    insertItem(QPixmap(":/ui/color08.png"), tr("Gray"));
    insertItem(QPixmap(":/ui/color09.png"), tr("Light Gray"));
    insertItem(QPixmap(":/ui/colorxx.png"), tr("Others.."));

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotColorChanged(int)));

	if (showUnchanged) {
        setCurrentItem(0);
	}
    else if (showByLayer) {
        setCurrentItem(0);
    } else {
        setCurrentItem(6);
    }

    slotColorChanged(currentItem());
}

/**
 * Sets the color shown in the combobox to the given color.
 */
void QG_ColorBox::setColor(const RS_Color& color) {
    currentColor = color;
	
    if (color.isByLayer() && showByLayer) {
        setCurrentItem(0);
    } else if (color.isByBlock() && showByLayer) {
        setCurrentItem(1);
    } else if (color==QColor(Qt::red)) {
        setCurrentItem(0+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::yellow)) {
        setCurrentItem(1+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::green)) {
        setCurrentItem(2+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::cyan)) {
        setCurrentItem(3+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::blue)) {
        setCurrentItem(4+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::magenta)) {
        setCurrentItem(5+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(Qt::white) || color==QColor(Qt::black)) {
        setCurrentItem(6+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(127,127,127)) {
        setCurrentItem(7+(int)showByLayer*2 + (int)showUnchanged);
    } else if (color==QColor(191,191,191)) {
        setCurrentItem(8+(int)showByLayer*2 + (int)showUnchanged);
    } else {
        setCurrentItem(9+(int)showByLayer*2 + (int)showUnchanged);
    }

    if (currentItem()!=9+(int)showByLayer*2 + (int)showUnchanged) {
        slotColorChanged(currentItem());
    }
}



/**
 * Sets the color of the pixmap next to the "By Layer" item
 * to the given color.
 */
void QG_ColorBox::setLayerColor(const RS_Color& color) {
    if (showByLayer) {
        QPixmap pixmap;
        if (color==Qt::black || color==Qt::white) {
            pixmap = QPixmap(":/ui/color07.png");
        } else {
            pixmap = QPixmap(":/ui/color00.png");
            int w = pixmap.width();
            int h = pixmap.height();
            QPainter painter(&pixmap);
            painter.fillRect(1, 1, w-2, h-2, color);
            painter.end();
        }

        changeItem(pixmap, tr("By Layer"), 0);

        // needed for the first time a layer is added:
        if (currentItem()!=9) {
            slotColorChanged(currentItem());
        }
    }
}



/**
 * Called when the color has changed. This method 
 * sets the current color to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual color.
 */
void QG_ColorBox::slotColorChanged(int index) {
    currentColor.resetFlags();
	
    if (showUnchanged) {
		if (index==0) {
			unchanged = true;
		}
		else {
			unchanged = false;
		}
	}

    if (showByLayer) {
        switch (index-(int)showUnchanged) {
        case 0:
            currentColor = RS_Color(RS2::FlagByLayer);
            break;
        case 1:
            currentColor = RS_Color(RS2::FlagByBlock);
            break;
        default:
            break;
        }
    }

    switch (index-(int)showByLayer*2-(int)showUnchanged) {
    case 0:
        currentColor.setRgb(255, 0, 0);
        break;
    case 1:
        currentColor.setRgb(255, 255, 0);
        break;
    case 2:
        currentColor.setRgb(0, 255, 0);
        break;
    case 3:
        currentColor.setRgb(0, 255, 255);
        break;
    case 4:
        currentColor.setRgb(0, 0, 255);
        break;
    case 5:
        currentColor.setRgb(255, 0, 255);
        break;
    case 6:
        currentColor.setRgb(0, 0, 0);
        break;
    case 7:
        currentColor.setRgb(127, 127, 127);
        break;
    case 8:
        currentColor.setRgb(191, 191, 191);
        break;
    case 9:
        currentColor = QColorDialog::getColor(currentColor, this);
        break;
    default:
        break;
    }

    //printf("Current color is (%d): %s\n",
    //       index, currentColor.name().latin1());

    emit colorChanged(currentColor);
}


