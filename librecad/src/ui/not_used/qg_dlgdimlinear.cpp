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
#include "qg_dlgdimlinear.h"
#include "rs_dimlinear.h"
#include "rs_graphic.h"
/*
 *  Constructs a QG_DlgDimLinear as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgDimLinear::QG_DlgDimLinear(QWidget *parent, LC_GraphicViewport *pViewport,RS_DimLinear* dim)
    :LC_EntityPropertiesDlg(parent, "DimLinearProperties", pViewport){
    setupUi(this);
    setEntity(dim);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgDimLinear::~QG_DlgDimLinear(){
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgDimLinear::languageChange(){
    retranslateUi(this);
}

void QG_DlgDimLinear::setEntity(RS_DimLinear* d) {
    m_entity = d;

    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(m_entity, lay, tr("Pen"));
    wLabel->setLabel(m_entity->getLabel(false));
    toUIAngleDeg(m_entity->getAngle(), leAngle);
}

void QG_DlgDimLinear::updateEntity() {
    m_entity->setLabel(wLabel->getLabel());
    m_entity->setAngle(toWCSAngle(leAngle, m_entity->getAngle()));

    m_entity->setPen(wPen->getPen());
    m_entity->setLayer(cbLayer->getLayer());

    m_entity->updateDim(true);
}
