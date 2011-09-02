/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include "qg_colorbox.h"

#include <qcolordialog.h>
#include <qpainter.h>
#include <qpixmap.h>

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_ColorBox::QG_ColorBox(QWidget* parent, const char* name)
        : QComboBox(parent) {

    setObjectName(name);
    setEditable ( false );
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
        : QComboBox(parent) {
	
    setObjectName(name);
    setEditable ( false );
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
                addItem(QIcon(":/ui/color00.png"), tr("Unchanged"));
	}
    if (showByLayer) {
        addItem(QIcon(":/ui/color00.png"), tr("By Layer"));
        addItem(QIcon(":/ui/color00.png"), tr("By Block"));
    }

    addItem(QIcon(":/ui/color01.png"), tr("Red"));
    addItem(QIcon(":/ui/color02.png"), tr("Yellow"));
    addItem(QIcon(":/ui/color03.png"), tr("Green"));
    addItem(QIcon(":/ui/color04.png"), tr("Cyan"));
    addItem(QIcon(":/ui/color05.png"), tr("Blue"));
    addItem(QIcon(":/ui/color06.png"), tr("Magenta"));
    QString blackWhite(tr("Black / White"));
    addItem(QIcon(":/ui/color07.png"), blackWhite);
    addItem(QIcon(":/ui/color08.png"), tr("Gray"));
    addItem(QIcon(":/ui/color09.png"), tr("Light Gray"));
    addItem(QIcon(":/ui/colorxx.png"), tr("Others.."));

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotColorChanged(int)));

	if (showUnchanged) {
        setCurrentIndex(0);
	}
    else if (showByLayer) {
        setCurrentIndex(0);
    } else {
        setCurrentIndex(findText(blackWhite));
    }

    slotColorChanged(currentIndex());
}

/**
 * Sets the color shown in the combobox to the given color.
 */
void QG_ColorBox::setColor(const RS_Color& color) {
    currentColor = color;
	
    RS_Color c0(color.stripFlags());
int cIndex((int)showByLayer*2 + (int)showUnchanged);

    if (color.isByLayer() && showByLayer) {
        cIndex=0;
    } else if (color.isByBlock() && showByLayer) {
        cIndex=1;
    } else if (c0==RS_Color(QColor(Qt::red))) {
        cIndex += 0;
    } else if (c0==RS_Color(QColor(Qt::yellow))) {
        cIndex += 1;
    } else if (c0==RS_Color(QColor(Qt::green))) {
        cIndex += 2;
    } else if (c0==RS_Color(QColor(Qt::cyan))) {
        cIndex += 3;
    } else if (c0==RS_Color(QColor(Qt::blue))) {
        cIndex += 4;
    } else if (c0==RS_Color(QColor(Qt::magenta))) {
        cIndex += 5;
    } else if (c0==RS_Color(QColor(Qt::white)) || c0==RS_Color(QColor(Qt::black))) {
        cIndex += 6;
    } else if (c0==RS_Color(QColor(127,127,127))) {
        cIndex += 7;
    } else if (c0==RS_Color(QColor(191,191,191))) {
        cIndex += 8;
    } else {
        cIndex += 9;
    }
    setCurrentIndex(cIndex);

    if (currentIndex()!=9+(int)showByLayer*2 + (int)showUnchanged) {
        slotColorChanged(currentIndex());
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

        setItemIcon(0, QIcon(pixmap));
        setItemText(0, tr("By Layer"));

        // needed for the first time a layer is added:
        if (currentIndex()!=9) {
            slotColorChanged(currentIndex());
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


