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
//static colors starts here
    //addColor(QIcon(":/ui/color01.png"), red,Qt::red);
    QString red(tr("Red"));
    addColor(Qt::red,red);
    colorIndexStart=findText(red); // keep the starting point of static colors
    addColor(Qt::darkRed,tr("Dark Red"));
    addColor(Qt::yellow,tr("Yellow"));
    addColor(Qt::darkYellow,tr("Dark Yellow"));
    addColor(Qt::green,tr("Green"));
    addColor(Qt::darkGreen,tr("Dark Green"));
    addColor(Qt::cyan,tr("Cyan"));
    addColor(Qt::darkCyan,tr("Dark Cyan"));
    addColor(Qt::blue,tr("Blue"));
    addColor(Qt::darkBlue,tr("Dark Blue"));
    addColor(Qt::magenta,tr("Magenta"));
    addColor(Qt::darkMagenta,tr("Dark Magenta"));
    addItem(QIcon(":/ui/color07.png"), tr("Black / White"),Qt::black);
    //addColor(Qt::black,tr("Black"));
    //addColor(Qt::white,tr("White"));
    addColor(Qt::gray,tr("Gray"));
    addColor(Qt::darkGray,tr("Dark Gray"));
    addColor(Qt::lightGray,tr("Light Gray"));
//static colors ends here
    // last item is reserved for "Others.." to trigger color picker
    addItem(QIcon(":/ui/colorxx.png"), tr("Others.."));

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotColorChanged(int)));

    if (showUnchanged || showByLayer ) {
        setCurrentIndex(0);
    } else {
        setCurrentIndex(findData(Qt::black)); //default to Qt::black
    }

    slotColorChanged(currentIndex());
}

/**
 * Sets the color shown in the combobox to the given color.
 */
void QG_ColorBox::setColor(const RS_Color& color) {
    currentColor = color;

    int cIndex;

    if (color.isByLayer() && showByLayer) {
        cIndex=0;
    } else if (color.isByBlock() && showByLayer) {
        cIndex=1;
    } else {
        cIndex=findData(color.toQColor());
        if(cIndex == -1 ) {
            cIndex=count() - 1; // the last item is "Others.."
        } else {
            if ( itemData(cIndex) == QVariant::Invalid) cIndex=count() - 1; // invalid input color, set to "Others..", setting to Black/White is another option
        }
    }
    setCurrentIndex(cIndex);

    if (currentIndex()!= count() -1 ) {
        slotColorChanged(currentIndex());
    }
}

/**
 * generate icon from color, then add to color box
 */
void QG_ColorBox::addColor(Qt::GlobalColor color, QString text)
{
    QPixmap pixmap(":/ui/color00.png");
    int w = pixmap.width();
    int h = pixmap.height();
    QPainter painter(&pixmap);
    painter.fillRect(1, 1, w-2, h-2, color);
    addItem(QIcon(pixmap),text,color);
}

/**
 * Sets the color of the pixmap next to the "By Layer" item
 * to the given color.
 */
void QG_ColorBox::setLayerColor(const RS_Color& color) {
    if (! showByLayer ) return;
    QPixmap pixmap;
    pixmap = QPixmap(":/ui/color00.png");
    int w = pixmap.width();
    int h = pixmap.height();
    QPainter painter(&pixmap);
    painter.fillRect(1, 1, w-2, h-2, color);
    painter.end();

    setItemIcon(0, QIcon(pixmap));
    setItemText(0, tr("By Layer"));

    // needed for the first time a layer is added:
    if (currentIndex()!= count() -1 ) {
        slotColorChanged(currentIndex());
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
    if ( index >= colorIndexStart ) {
        if(index < count() -1 ) {
                QVariant q0=itemData(index);
                if(q0 != QVariant::Invalid ) {
            currentColor=itemData(index).value<QColor>();
                } else {
            currentColor=Qt::black; //default to black color
                }
        } else { // color picker for "Others.."
            currentColor = QColorDialog::getColor(currentColor, this);
        }
    }

    //printf("Current color is (%d): %s\n",
    //       index, currentColor.name().latin1());

    emit colorChanged(currentColor);
}


