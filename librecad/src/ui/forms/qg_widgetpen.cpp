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

#include <QVariant>
#include "qg_widgetpen.h"
#include "qevent.h"
#include "qg_colorbox.h"
#include "qg_widthbox.h"
#include "qg_linetypebox.h"
#include "rs_debug.h"
#include "rs_entity.h"

/*
 *  Constructs a QG_WidgetPen as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_WidgetPen::QG_WidgetPen(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);
//    cbColor->setFocusPolicy(Qt::StrongFocus);
//    cbWidth->setFocusPolicy(Qt::StrongFocus);
//    cbLineType->setFocusPolicy(Qt::StrongFocus);
//    setTabOrder(cbColor, cbWidth); // cbColor to cbWidth
//    setTabOrder(cbWidth, cbLineType); // cbColor to cbWidth to cbLineType
}

void QG_WidgetPen::setPen(RS_Pen pen, bool showByLayer, 
                          bool showUnchanged, const QString& title) {
    cbColor->init(showByLayer, showUnchanged);
    cbWidth->init(showByLayer, showUnchanged);
    cbLineType->init(showByLayer, showUnchanged);
    if (!showUnchanged) {
       cbColor->setColor(pen.getColor());
       cbWidth->setWidth(pen.getWidth());
       cbLineType->setLineType(pen.getLineType());
    }

    if (!title.isEmpty()) {
        bgPen->setTitle(title);
    }
}


void QG_WidgetPen::setPen(RS_Entity* entity, RS_Layer* layer, const QString &title){
    RS_Pen entityPen = entity->getPen(false);
    RS_Pen entityResolvedPen = entity->getPen(true);

    RS_Color originalColor = entityPen.getColor();
    RS_Color resolvedColor = entityResolvedPen.getColor();
    resolvedColor.applyFlags(originalColor);
    entityResolvedPen.setColor(resolvedColor);

    entityResolvedPen.setLineType(entityPen.getLineType());
    entityResolvedPen.setWidth(entityPen.getWidth());

    setPen(entityResolvedPen, layer, title);
}


void QG_WidgetPen::setPen(RS_Pen pen, RS_Layer* layer, bool showUnchanged, const QString &title){
    setPen(pen, true, showUnchanged, title);
    if (layer != nullptr){
        RS_Pen layerPen = layer->getPen();
        RS_Color layerColor = layerPen.getColor();
        cbColor->setLayerColor(layerColor);
    }
}
void QG_WidgetPen::setPen(RS_Pen pen, RS_Layer* layer, const QString &title)
{
    setPen(pen, layer, false, title);
}

RS_Pen QG_WidgetPen::getPen() {
    RS_Pen pen;

    pen.setColor(cbColor->getColor());
    pen.setWidth(cbWidth->getWidth());
    pen.setLineType(cbLineType->getLineType());

    return pen;
}

bool QG_WidgetPen::isColorUnchanged() {
    return cbColor->isUnchanged();
}

bool QG_WidgetPen::isLineTypeUnchanged() {
    return cbLineType->isUnchanged();
}

bool QG_WidgetPen::isWidthUnchanged() {
    return cbWidth->isUnchanged();
}
/*
 *  Destroys the object and frees any allocated resources
 */
QG_WidgetPen::~QG_WidgetPen()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_WidgetPen::languageChange()
{
    retranslateUi(this);
}

/*
 *  When tabbing in to the widget, passes the focus to the 
 *  cbColor subwidget.
 */
void QG_WidgetPen::focusInEvent(QFocusEvent *event)
{
    int reason = event->reason();
    RS_DEBUG->print(RS_Debug::D_ERROR,"QG_WidgetPen::focusInEvent, reason '%d'",reason);
//	if ( reason == Qt::BacktabFocusReason )
//	    cbLineType->setFocus();
//	else
//	    cbColor->setFocus();
}

void QG_WidgetPen::focusOutEvent(QFocusEvent *event)
{
    int reason = event->reason();
    RS_DEBUG->print(RS_Debug::D_ERROR,"QG_WidgetPen::focusOutEvent, reason '%d'",reason);
}


