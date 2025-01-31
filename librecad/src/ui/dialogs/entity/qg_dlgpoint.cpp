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
#include "qg_dlgpoint.h"

#include "rs_point.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgPoint as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgPoint::QG_DlgPoint(QWidget *parent, LC_GraphicViewport *pViewport)
    :LC_EntityPropertiesDlg(parent, "PointProperties", pViewport){
    setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgPoint::~QG_DlgPoint(){
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgPoint::languageChange(){
    retranslateUi(this);
}

void QG_DlgPoint::initAttributes(RS_Layer* layer, RS_LayerList& layerList){
    if (layer) {
        cbLayer->setLayer(*layer);
    }
    cbLayer->init(layerList, false, false);
    wPen->setPen(point, layer, "Pen");
}

void QG_DlgPoint::setEntity(RS_Point& p) {
    point = &p;
    setAttributes(point);
    setProperties();
}

void QG_DlgPoint::updateEntity() {
    updateProperties();
    updateAttributes();
}

void QG_DlgPoint::setAttributes(RS_Entity* e) {
    RS_Graphic* graphic = e->getGraphic();
    if (graphic != nullptr){
        RS_Layer* lay = e->getLayer(false);
        RS_LayerList &layerList = *(graphic->getLayerList());
        initAttributes(lay, layerList);
    }
}

void QG_DlgPoint::setProperties() {
    RS_Vector uiPos = toUIVector(point->getPos());
    QString sx = asString(uiPos.x);
    lePosX->setText(sx);
    QString sy = asString(uiPos.y);
    lePosY->setText(sy);
}

void QG_DlgPoint::updateAttributes() {
    const RS_Pen &rsPen = wPen->getPen();
    RS_Layer *layer = cbLayer->getLayer();
    point->setPen(rsPen);
    point->setLayer(layer);
}

void QG_DlgPoint::updateProperties() {
    const QString &sx = lePosX->text();
    const QString &sy = lePosY->text();

    double uix = toDouble(sx);
    double uiy = toDouble(sy);

    const RS_Vector &uiPos = RS_Vector(uix,uiy);
    RS_Vector entityPos = toEntityVector(uiPos);

    point->setPos(entityPos);
}
