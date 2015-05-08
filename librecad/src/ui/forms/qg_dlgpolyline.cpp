/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
**
** This file is free software; you can redistribute it and/or modify
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
#include "qg_dlgpolyline.h"

#include "rs_polyline.h"
#include "rs_graphic.h"
/*#include "rs_layer.h"
#include "qg_widgetpen.h"
#include "qg_layerbox.h"*/

/*
 *  Constructs a QG_DlgPolyline as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgPolyline::QG_DlgPolyline(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgPolyline::~QG_DlgPolyline()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgPolyline::languageChange()
{
    retranslateUi(this);
}

void QG_DlgPolyline::setPolyline(RS_Polyline& e) {
    polyline = &e;
    //pen = spline->getPen();
    wPen->setPen(polyline->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = polyline->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = polyline->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }
	
    cbClosed->setChecked(polyline->isClosed());
}



void QG_DlgPolyline::updatePolyline() {
    polyline->setClosed(cbClosed->isChecked(), 0.0);
    polyline->setPen(wPen->getPen());
    polyline->setLayer(cbLayer->currentText());
        polyline->update();
}

