/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actionmodifyattributes.h"

#include "rs_modification.h"
#include "rs_snapper.h"



RS_ActionModifyAttributes::RS_ActionModifyAttributes(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Change Attributes",
                    container, graphicView) {}


QAction* RS_ActionModifyAttributes::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Attributes"), tr("&Attributes"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Attributes"),  NULL);
    action->setStatusTip(tr("Modify Entity Attributes"));

    return action;
}


void RS_ActionModifyAttributes::init(int status) {
    RS_ActionInterface::init(status);

    trigger();
}



void RS_ActionModifyAttributes::trigger() {

    RS_DEBUG->print("RS_ActionModifyAttributes::trigger()");

    RS_AttributesData data;
    data.pen = RS_Pen();
    data.layer = "0";
    data.changeColor = false;
    data.changeLineType = false;
    data.changeWidth = false;
    data.changeLayer = false;

    if (graphic!=NULL) {
        if (RS_DIALOGFACTORY->requestAttributesDialog(data,
                *graphic->getLayerList())) {
            RS_Modification m(*container, graphicView);
            m.changeAttributes(data);
        }
    }

    finish();
    graphicView->killSelectActions();

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionModifyAttributes::updateMouseButtonHints() {
    switch (getStatus()) {
        //case Acknowledge:
        //RS_DIALOGFACTORY->updateMouseWidget(tr("Acknowledge"), tr("Cancel"));
        //break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyAttributes::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::DelCursor);
}



void RS_ActionModifyAttributes::updateToolBar() {
    if (!isFinished()) {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    }
}


// EOF
