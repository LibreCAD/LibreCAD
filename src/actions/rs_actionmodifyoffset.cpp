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

#include<QAction>
#include "rs_actionmodifyoffset.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
//#include "rs_commands.h"
//#include "rs_commandevent.h"
#include "rs_modification.h"


RS_ActionModifyOffset::RS_ActionModifyOffset(RS_EntityContainer& container,
                                             RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Modify Offset",
                               container, graphicView) {
    std::cout<<"RS_ActionModifyOffset::RS_ActionModifyOffset()\n";

    data.distance=0.;
    data.number=1;
    data.useCurrentAttributes = true;
    data.useCurrentLayer = true;
}



RS_ActionModifyOffset::~RS_ActionModifyOffset() {}

QAction* RS_ActionModifyOffset::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action = new QAction(tr("&Offset"),NULL);
    action->setIcon(QIcon(":/extui/arcspara.png"));//we need a new icon here
    return action;
}



void RS_ActionModifyOffset::trigger() {
    deletePreview();
    RS_Modification m(*container, graphicView);
    m.offset(data);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    finish(false);
}



void RS_ActionModifyOffset::mouseMoveEvent(QMouseEvent* e) {
//    RS_DEBUG->print("RS_ActionModifyOffset::mouseMoveEvent begin");
    position=snapPoint(e);

    deletePreview();
    preview->addSelectionFrom(*container);
    RS_Modification m(*preview, graphicView);
    m.offset(data);
    drawPreview();
}



void RS_ActionModifyOffset::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
            trigger();
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void RS_ActionModifyOffset::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetPosition:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify direction of offset"), tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionModifyOffset::showOptions() {
    RS_ActionInterface::showOptions();
std::cout<<"O0\n";
    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestModifyOffsetOptions(data.distance, true);
    }
}



void RS_ActionModifyOffset::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestModifyOffsetOptions(data.distance, false);
    }
}



void RS_ActionModifyOffset::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyOffset::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}


// EOF

