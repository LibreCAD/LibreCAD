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
#include "qg_dlgline.h"

#include "rs_line.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_DlgLine as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgLine::QG_DlgLine(QWidget *parent, LC_GraphicViewport *pViewport, RS_Line* line)
    :LC_EntityPropertiesDlg(parent, "LineProperties", pViewport){
    setupUi(this);
    setEntity(line);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgLine::languageChange(){
    retranslateUi(this);
}

void QG_DlgLine::setEntity(RS_Line* l) {
    m_entity = l;


    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(true);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(m_entity, lay, tr("Pen"));
    toUI(m_entity->getStartpoint(), leStartX, leStartY);
    toUI(m_entity->getEndpoint(), leEndX, leEndY);

    // fixme - sand - refactor to common function
    if (LC_GET_ONE_BOOL("Appearance","ShowEntityIDs", false)){
        lId->setText(QString("ID: %1").arg(m_entity->getId()));
    }
    else{
        lId->setVisible(false);
    }
}

void QG_DlgLine::updateEntity() {
    m_entity->setStartpoint(toWCS(leStartX, leStartY, m_entity->getStartpoint()));
    m_entity->setEndpoint(toWCS(leEndX, leEndY, m_entity->getEndpoint()));

    m_entity->setPen(wPen->getPen());
    m_entity->setLayer(cbLayer->getLayer());
}
