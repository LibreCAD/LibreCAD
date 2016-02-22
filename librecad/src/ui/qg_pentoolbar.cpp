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

#include "qg_pentoolbar.h"

#include "qg_colorbox.h"
#include "qg_widthbox.h"
#include "qg_linetypebox.h"
#include "rs_pen.h"

/**
 * Constructor.
 */
QG_PenToolBar::QG_PenToolBar( const QString & title, QWidget * parent )
		: QToolBar(title, parent)
		, currentPen(new RS_Pen{})
		, colorBox(new QG_ColorBox{true, false, this, "colorbox"})
		, widthBox(new QG_WidthBox{true, false, this, "widthbox"})
		, lineTypeBox(new QG_LineTypeBox{true, false, this, "lineTypebox"})
{
    colorBox->setToolTip(tr("Line color"));
	connect(colorBox.get(), SIGNAL(colorChanged(const RS_Color&)),
            this, SLOT(slotColorChanged(const RS_Color&)));

    widthBox->setToolTip(tr("Line width"));
	connect(widthBox.get(), SIGNAL(widthChanged(RS2::LineWidth)),
            this, SLOT(slotWidthChanged(RS2::LineWidth)));

    lineTypeBox->setToolTip(tr("Line type"));
	connect(lineTypeBox.get(), SIGNAL(lineTypeChanged(RS2::LineType)),
            this, SLOT(slotLineTypeChanged(RS2::LineType)));

	currentPen->setColor(colorBox->getColor());
	currentPen->setWidth(widthBox->getWidth());
	currentPen->setLineType(lineTypeBox->getLineType());

	addWidget(colorBox.get());
	addWidget(widthBox.get());
	addWidget(lineTypeBox.get());
}


/**
 * Destructor
 */
QG_PenToolBar::~QG_PenToolBar() = default;


/**
 * Called by the layer list if this object was added as a listener
 * to a layer list.
 */
void QG_PenToolBar::layerActivated(RS_Layer* l) {

    //printf("QG_PenToolBar::layerActivated\n");

	if (l==nullptr) return;

    //colorBox->setColor(l->getPen().getColor());
    //widthBox->setWidth(l->getPen().getWidth());

    colorBox->setLayerColor(l->getPen().getColor());
    widthBox->setLayerWidth(l->getPen().getWidth());
    lineTypeBox->setLayerLineType(l->getPen().getLineType());

    //if (colorBox->getColor().getFlag(C_BY_LAYER)) {
    //printf("  Color by layer\n");
    //colorBox->setColor(l->getPen().getColor());
    //}
}


RS_Pen QG_PenToolBar::getPen() const {
	return *currentPen;
}

/**
 * Called by the layer list (if this object was previously
 * added as a listener to a layer list).
 */
void QG_PenToolBar::layerEdited(RS_Layer*) {}


/**
 * Called when the color was changed by the user.
 */
void QG_PenToolBar::slotColorChanged(const RS_Color& color) {
	currentPen->setColor(color);
    //printf("  color changed\n");

	emit penChanged(*currentPen);
}

/**
 * Called when the width was changed by the user.
 */
void QG_PenToolBar::slotWidthChanged(RS2::LineWidth w) {
	currentPen->setWidth(w);
    //printf("  width changed\n");

	emit penChanged(*currentPen);
}

/**
 * Called when the linetype was changed by the user.
 */
void QG_PenToolBar::slotLineTypeChanged(RS2::LineType w) {
	currentPen->setLineType(w);
    //printf("  line type changed\n");

	emit penChanged(*currentPen);
}

