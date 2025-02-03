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
#include <QFileInfo>

#include "qg_dlgimage.h"

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
    entity = e;

    RS_Graphic* graphic = entity->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = entity->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(entity, lay, "Pen");

    toUI(entity->getInsertionPoint(), leInsertX, leInsertY);

    toUIValue(entity->getImageWidth(), leWidth);
    toUIValue(entity->getImageHeight(), leHeight);

    scale = entity->getUVector().magnitude();
    toUIValue(scale, leScale);

    double uAngle = entity->getUVector().angle();
    toUIAngleDeg(uAngle, leAngle);

    lePath->setText(entity->getFile());

    leSize->setText(QString("%1 x %2").arg(entity->getWidth()).arg(entity->getHeight()));
    toUIValue(RS_Units::scaleToDpi(scale, entity->getGraphicUnit()), leDPI);
}


void QG_DlgImage::changeWidth() {
    double width = toWCSValue(leWidth, entity->getWidth());
    scale = width / entity->getWidth();

    toUIValue(entity->getHeight()*scale, leHeight);
    toUIValue(scale, leScale);
}

void QG_DlgImage::changeHeight() {
    double height = toWCSValue(leHeight, entity->getHeight());
    scale = height / entity->getHeight();

    toUIValue(entity->getWidth()*scale, leWidth);
    toUIValue(scale, leScale);
}
void QG_DlgImage::changeScale() {
    scale = toWCSValue(leScale, scale);
    toUIValue(entity->getWidth()*scale, leWidth);
    toUIValue(entity->getHeight()*scale, leHeight);
    toUIValue(RS_Units::scaleToDpi(scale, entity->getGraphicUnit()), leDPI);
}

void QG_DlgImage::changeDPI(){
    double oldDpi = RS_Units::scaleToDpi(scale, entity->getGraphicUnit()); // todo - what if scale was changed? Save dpi in dlg?
    double dpi = toWCSValue(leDPI, oldDpi);
    scale = RS_Units::dpiToScale(dpi, entity->getGraphicUnit());
    toUIValue(scale, leScale);
    toUIValue(entity->getWidth()*scale, leWidth);
    toUIValue(entity->getHeight()*scale, leHeight);
}

void QG_DlgImage::updateEntity() {
    entity->setInsertionPoint(toWCS(leInsertX, leInsertY, entity->getInsertionPoint()));

    double orgScale = entity->getUVector().magnitude();
    scale /= orgScale;
    double orgAngle = entity->getUVector().angle();

    double angle = toWCSAngle(leAngle, orgAngle);

    entity->scale(entity->getInsertionPoint(), RS_Vector(scale, scale));
    entity->rotate(entity->getInsertionPoint(), angle - orgAngle);

    if (QFileInfo(lePath->text()).isFile())
        entity->setFile(lePath->text());

    entity->setPen(wPen->getPen());
    entity->setLayer(cbLayer->getLayer());

    entity->update();
}

void QG_DlgImage::setImageFile() {
    lePath->setText(RS_DIALOGFACTORY->requestImageOpenDialog()); // fixme - is it bad dependency?
}
