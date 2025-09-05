/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */
#include "qg_dlgpolyline.h"
#include "rs_polyline.h"
#include "rs_graphic.h"

/*
 *  Constructs a QG_DlgPolyline as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgPolyline::QG_DlgPolyline(QWidget *parent, LC_GraphicViewport *pViewport, RS_Polyline* polyline)
    :LC_EntityPropertiesDlg(parent, "PolylineProperties", pViewport) {
    setupUi(this);
    setEntity(polyline);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgPolyline::~QG_DlgPolyline(){
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgPolyline::languageChange(){
    retranslateUi(this);
}

void QG_DlgPolyline::setEntity(RS_Polyline* e) {
    m_entity = e;


    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(m_entity, lay, tr("Pen"));
    toUIBool(m_entity->isClosed(), cbClosed);
}

void QG_DlgPolyline::updateEntity() {
    m_entity->setClosed(cbClosed->isChecked(),0);
    m_entity->setPen(wPen->getPen());
    m_entity->setLayer(cbLayer->getLayer());
    m_entity->update();
    m_entity->calculateBorders();
}
