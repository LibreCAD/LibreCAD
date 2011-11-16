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

#include "rs_actionblockscreate.h"

#include <QAction>
#include "rs_creation.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"

/**
 * Constructor.
 */
RS_ActionBlocksCreate::RS_ActionBlocksCreate(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Blocks Create",
                           container, graphicView) {

    referencePoint = RS_Vector(false);
}



RS_ActionBlocksCreate::~RS_ActionBlocksCreate() {}



QAction* RS_ActionBlocksCreate::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
        // (tr("Create Block"),
    QAction* action = new QAction(tr("&Create Block"), NULL);
    //action->zetStatusTip(tr("Create Block"));
        action->setIcon(QIcon(":/extui/menublock.png"));
    return action;
}



void RS_ActionBlocksCreate::init(int status) {
    RS_PreviewActionInterface::init(status);

}



void RS_ActionBlocksCreate::trigger() {
    //deletePreview();

    //RS_Modification m(*container, graphicView);
    //m.paste(data.insertionPoint);
    //std::cout << *RS_Clipboard::instance();

    if (graphic!=NULL) {
        RS_BlockList* blockList = graphic->getBlockList();
        if (blockList!=NULL) {
            RS_BlockData d =
                RS_DIALOGFACTORY->requestNewBlockDialog(blockList);

            if (!d.name.isEmpty()) {
                RS_Creation creation(container, graphicView);
                creation.createBlock(d, referencePoint, true);

                RS_InsertData id(
                    d.name,
                    referencePoint,
                    RS_Vector(1.0,1.0),
                    0.0,
                    1, 1, RS_Vector(0.0,0.0)
                );
                creation.createInsert(id);
            }
        }
    }

    graphicView->redraw(RS2::RedrawDrawing);

    setStatus(getStatus()+1); // clear mouse button hints
    updateMouseButtonHints();
//    if(RS_DIALOGFACTORY!=NULL){
//        RS_DIALOGFACTORY->requestPreviousToolBar();
//    }
    graphicView->killSelectActions();
    finish(false);
}


void RS_ActionBlocksCreate::mouseMoveEvent(QMouseEvent* e) {
    snapPoint(e);

    switch (getStatus()) {
    case SetReferencePoint:
        //data.insertionPoint = snapPoint(e);

        /*if (block!=NULL) {
            deletePreview();
            //preview->addAllFrom(*block);
            //preview->move(data.insertionPoint);
                RS_Creation creation(preview, NULL, false);
                creation.createInsert(data);
            drawPreview();
    }*/
        break;

    default:
        break;
    }
}



void RS_ActionBlocksCreate::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionBlocksCreate::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    switch (getStatus()) {
    case SetReferencePoint:
        referencePoint = e->getCoordinate();
        trigger();
        break;

    default:
        break;

    }
}



void RS_ActionBlocksCreate::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionBlocksCreate::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionBlocksCreate::updateToolBar() {
//    if (isFinished()) {
//        RS_DIALOGFACTORY->requestPreviousToolBar();
//        RS_DIALOGFACTORY->requestPreviousToolBar();
//    }
//}


// EOF
