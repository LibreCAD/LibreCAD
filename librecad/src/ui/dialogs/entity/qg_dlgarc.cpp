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
#include "qg_dlgarc.h"
#include "rs_arc.h"
#include "rs_graphic.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_DlgArc as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgArc::QG_DlgArc(QWidget *parent, LC_GraphicViewport *pViewport, RS_Arc* arc)
    :LC_EntityPropertiesDlg (parent, "ArcProperties", pViewport){
    setupUi(this);
    setEntity(arc);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgArc::languageChange(){
    retranslateUi(this);
}

void QG_DlgArc::setEntity(RS_Arc* a) {
    m_entity = a;
    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(m_entity, lay, tr("Pen"));

    toUI(m_entity->getCenter(), leCenterX, leCenterY);
    toUIValue(m_entity->getRadius(), leRadius);
    toUIAngleDeg(m_entity->getAngle1(), leAngle1);
    toUIAngleDeg(m_entity->getAngle2(), leAngle2);
    toUIBool(m_entity->isReversed(), cbReversed);

    // fixme - sand - refactor to common function
    if (LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false)){
        lId->setText(QString("ID: %1").arg(m_entity->getId()));
    }
    else{
        lId->setVisible(false);
    }
}

void QG_DlgArc:: updateEntity() {
    m_entity->setCenter(toWCS(leCenterX, leCenterY, m_entity->getCenter()));
    m_entity->setRadius(toWCSValue(leRadius, m_entity->getRadius()));

    m_entity->setAngle1(toWCSAngle(leAngle1, m_entity->getAngle1()));
    m_entity->setAngle2(toWCSAngle(leAngle2, m_entity->getAngle2()));

    if (m_entity->isReversed() != cbReversed->isChecked()) {
        m_entity->revertDirection();
    }

    m_entity->setPen(wPen->getPen());
    m_entity->setLayer(cbLayer->getLayer());
    m_entity->calculateBorders();
}
