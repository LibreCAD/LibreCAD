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
#include "qg_dlgimage.h"

#include "rs_image.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgImage as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgImage::QG_DlgImage(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgImage::~QG_DlgImage()
{
    delete val;
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgImage::languageChange()
{
    retranslateUi(this);
}

void QG_DlgImage::setImage(RS_Image& e) {
    image = &e;
    val = new QDoubleValidator(leScale);
    //pen = spline->getPen();
    wPen->setPen(image->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = image->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = image->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }
    leInsertX->setValidator(val);
    leInsertY->setValidator(val);
    leWidth->setValidator(val);
    leHeight->setValidator(val);
    leScale->setValidator(val);
    leAngle->setValidator(val);
    scale = image->getUVector().magnitude();
    leInsertX->setText(QString("%1").arg(image->getInsertionPoint().x));
    leInsertY->setText(QString("%1").arg(image->getInsertionPoint().y));
    leWidth->setText(QString("%1").arg(image->getImageWidth()));
    leHeight->setText(QString("%1").arg(image->getImageHeight()));
    leScale->setText(QString("%1").arg(scale));
    leAngle->setText(QString("%1").arg( RS_Math::rad2deg(image->getUVector().angle()) ));
    lePath->setText(image->getFile());
    leSize->setText(QString("%1 x %2").arg(image->getWidth()).arg(image->getHeight()));    
    leDPI->setText(QString("%1").arg(RS_Units::scaleToDpi(scale,image->getGraphicUnit())));
}


void QG_DlgImage::changeWidth() {
    double width = leWidth->text().toDouble();
    scale = width / image->getWidth();
    leHeight->setText(QString("%1").arg(image->getHeight() * scale));
    leScale->setText(QString("%1").arg(scale));
}
void QG_DlgImage::changeHeight() {
    double height = leHeight->text().toDouble();
    scale = height / image->getHeight();
    leWidth->setText(QString("%1").arg(image->getWidth() * scale));
    leScale->setText(QString("%1").arg(scale));
}
void QG_DlgImage::changeScale() {
    scale = leScale->text().toDouble();
    leWidth->setText(QString("%1").arg(image->getWidth() * scale));
    leHeight->setText(QString("%1").arg(image->getHeight() * scale));
    leDPI->setText(QString("%1").arg(RS_Units::scaleToDpi(scale, image->getGraphicUnit())));
}

void QG_DlgImage::changeDPI(){
    scale = RS_Units::dpiToScale(leDPI->text().toDouble(), image->getGraphicUnit());
    leScale->setText(QString("%1").arg(scale));
    leWidth->setText(QString("%1").arg(image->getWidth() * scale));
    leHeight->setText(QString("%1").arg(image->getHeight() * scale));    
}

void QG_DlgImage::updateImage() {
    image->setPen(wPen->getPen());
    image->setLayer(cbLayer->currentText());
    image->setInsertionPoint(RS_Vector(leInsertX->text().toDouble(), leInsertY->text().toDouble()) );
    double orgScale = image->getUVector().magnitude();
    scale /= orgScale;
    double orgAngle = image->getUVector().angle();
    double angle = RS_Math::deg2rad( leAngle->text().toDouble() );
    image->scale(image->getInsertionPoint(), RS_Vector(scale, scale));
    image->rotate(image->getInsertionPoint(), angle - orgAngle);

    image->update();
}



