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

#include "qg_dlgimage.h"

#include <QFileInfo>

#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_image.h"
#include "rs_units.h"

/*
 *  Constructs a QG_DlgImage as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgImage::QG_DlgImage(QWidget *parent, LC_GraphicViewport *pViewport, RS_Image* e)
    :LC_EntityPropertiesDlg(parent, "ImageProperties", pViewport) {
    setupUi(this);
    setEntity(e);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgImage::languageChange(){
    retranslateUi(this);
}

void QG_DlgImage::setEntity(RS_Image* e) {
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

    toUI(m_entity->getInsertionPoint(), leInsertX, leInsertY);

    toUIValue(m_entity->getImageWidth(), leWidth);
    toUIValue(m_entity->getImageHeight(), leHeight);

    m_scale = m_entity->getUVector().magnitude();
    toUIValue(m_scale, leScale);

    double uAngle = m_entity->getUVector().angle();
    toUIAngleDeg(uAngle, leAngle);

    lePath->setText(m_entity->getFile());

    leSize->setText(QString("%1 x %2").arg(m_entity->getWidth()).arg(m_entity->getHeight()));
    toUIValue(RS_Units::scaleToDpi(m_scale, m_entity->getGraphicUnit()), leDPI);
}


void QG_DlgImage::changeWidth() {
    double width = toWCSValue(leWidth, m_entity->getWidth());
    m_scale = width / m_entity->getWidth();

    toUIValue(m_entity->getHeight()*m_scale, leHeight);
    toUIValue(m_scale, leScale);
}

void QG_DlgImage::changeHeight() {
    double height = toWCSValue(leHeight, m_entity->getHeight());
    m_scale = height / m_entity->getHeight();

    toUIValue(m_entity->getWidth()*m_scale, leWidth);
    toUIValue(m_scale, leScale);
}
void QG_DlgImage::changeScale() {
    m_scale = toWCSValue(leScale, m_scale);
    toUIValue(m_entity->getWidth()*m_scale, leWidth);
    toUIValue(m_entity->getHeight()*m_scale, leHeight);
    toUIValue(RS_Units::scaleToDpi(m_scale, m_entity->getGraphicUnit()), leDPI);
}

void QG_DlgImage::changeDPI(){
    double oldDpi = RS_Units::scaleToDpi(m_scale, m_entity->getGraphicUnit()); // todo - what if scale was changed? Save dpi in dlg?
    double dpi = toWCSValue(leDPI, oldDpi);
    m_scale = RS_Units::dpiToScale(dpi, m_entity->getGraphicUnit());
    toUIValue(m_scale, leScale);
    toUIValue(m_entity->getWidth()*m_scale, leWidth);
    toUIValue(m_entity->getHeight()*m_scale, leHeight);
}

void QG_DlgImage::updateEntity() {
    m_entity->setInsertionPoint(toWCS(leInsertX, leInsertY, m_entity->getInsertionPoint()));

    double orgScale = m_entity->getUVector().magnitude();
    m_scale /= orgScale;
    double orgAngle = m_entity->getUVector().angle();

    double angle = toWCSAngle(leAngle, orgAngle);

    m_entity->scale(m_entity->getInsertionPoint(), RS_Vector(m_scale, m_scale));
    m_entity->rotate(m_entity->getInsertionPoint(), angle - orgAngle);

    if (QFileInfo(lePath->text()).isFile())
        m_entity->setFile(lePath->text());

    m_entity->setPen(wPen->getPen());
    m_entity->setLayer(cbLayer->getLayer());

    m_entity->update();
}

void QG_DlgImage::setImageFile() {
    lePath->setText(RS_DIALOGFACTORY->requestImageOpenDialog()); // fixme - is it bad dependency?
}
