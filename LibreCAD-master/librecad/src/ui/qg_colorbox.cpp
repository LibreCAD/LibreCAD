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

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include "rs_color.h"

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_ColorBox::QG_ColorBox(QWidget* parent, const char* name)
    :QComboBox(parent)
    ,currentColor(new RS_Color())
,showByLayer(false)
,showUnchanged(false)
,unchanged(false)
{
    setObjectName(name);
    setEditable ( false );
}

QG_ColorBox::~QG_ColorBox(){}

/**
 * Constructor that calls init and provides a fully functional
 * combobox for choosing colors.
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
QG_ColorBox::QG_ColorBox(bool showByLayer, bool showUnchanged,
                         QWidget* parent, const char* name)
    : QComboBox(parent)
    ,currentColor(new RS_Color())
    ,unchanged(false)
{

    setObjectName(name);
    setEditable ( false );
    init(showByLayer, showUnchanged);
}

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

    addItem(QIcon(":/ui/colorxx.png"), tr("Custom"));

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
    addItem(QIcon(":/ui/color07.png"), tr("Black / White"),QColor(Qt::black));
    //colorIndexBlack=findData(QColor(Qt::black)); //record the number for special color black/white
    //std::cout<<"colorIndexBlack="<<colorIndexBlack<<std::endl;
    addColor(Qt::gray,tr("Gray"));
    addColor(Qt::darkGray,tr("Dark Gray"));
    addColor(Qt::lightGray,tr("Light Gray"));
//static colors ends here

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotColorChanged(int)));

    if (showUnchanged || showByLayer ) {
        setCurrentIndex(0);
    } else {
        setCurrentIndex(findData(QColor(Qt::black))); //default to Qt::black
    }

    slotColorChanged(currentIndex());
}

/**
 * Sets the color shown in the combobox to the given color.
 */
void QG_ColorBox::setColor(const RS_Color& color) {
    *currentColor = color;
//std::cout<<"initial color: "<<color<<std::endl;
    int cIndex;

    if (color.isByLayer() && showByLayer) {
        cIndex=0;
    } else if (color.isByBlock() && showByLayer) {
        cIndex=1;
    } else {
        for(cIndex=colorIndexStart;cIndex< count() - 1;cIndex++) {
	//searching for the color, allowing difference up to 2
		QColor q(itemData(cIndex).value<QColor>());
		if( abs(q.red() - color.red())
		   +abs(q.green() - color.green())
		   +abs(q.blue() - color.blue()) <= 3
		) {
			break;
          }
        }
        /*color not found, default to "others...*/
/*        if(cIndex == count() - 1) {
       	    cIndex=findData(QColor(Qt::black)); //default to Qt::black
        }*/
    }
    setCurrentIndex(cIndex);
//std::cout<<"Default color for choosing: cIndex="<<cIndex<<" "<<RS_Color(itemData(cIndex).value<QColor>())<<std::endl;

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
    addItem(QIcon(pixmap),text,QColor(color));
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
    currentColor->resetFlags();
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
            *currentColor = RS_Color(RS2::FlagByLayer);
            break;
        case 1:
            *currentColor = RS_Color(RS2::FlagByBlock);
            break;
        default:
            break;
        }
    }

    if (itemText(index) == tr("Custom"))
    {
       RS_Color selectedColor = QColorDialog::getColor(*currentColor, this);
       if (selectedColor.isValid())
            *currentColor = selectedColor;
    }
    else if (index >= colorIndexStart)
    {
        if(index < count() )
        {
            QVariant q0=itemData(index);
            if(q0 != QVariant::Invalid )
            {
                *currentColor=itemData(index).value<QColor>();
            }
            else
            {
                *currentColor=Qt::black; //default to black color
            }
        }
    }

    emit colorChanged(*currentColor);
}

RS_Color QG_ColorBox::getColor() const{
    return *currentColor;
}

bool QG_ColorBox::isUnchanged() const{
    return unchanged;
}


