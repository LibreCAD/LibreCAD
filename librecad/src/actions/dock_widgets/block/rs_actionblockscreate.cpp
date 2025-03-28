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


#include <QMouseEvent>
#include "rs_actionblockscreate.h"

#include "rs_creation.h"
#include "rs_block.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_modification.h"


/**
 * Constructor.
 */
RS_ActionBlocksCreate::RS_ActionBlocksCreate(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Blocks Create",actionContext, RS2::ActionBlocksCreate)
    ,m_referencePoint(new RS_Vector{}){
}

RS_ActionBlocksCreate::~RS_ActionBlocksCreate() = default;


void RS_ActionBlocksCreate::init(int status) {
    RS_PreviewActionInterface::init(status);
}

void RS_ActionBlocksCreate::trigger() {
    if (m_graphic != nullptr) {
        RS_BlockList* blockList = m_graphic->getBlockList();
        if (blockList != nullptr) {
            RS_BlockData d =
                RS_DIALOGFACTORY->requestNewBlockDialog(blockList);

            if (!d.name.isEmpty()) {
                RS_Creation creation(m_container, getViewPort());
                creation.createBlock(&d, *m_referencePoint, true);

                RS_InsertData id(
                    d.name,
                    *m_referencePoint,
                    RS_Vector(1.0,1.0),
                    0.0,
                    1, 1, RS_Vector(0.0,0.0)
                );
                creation.createInsert(&id);
            }
        }
    }

    redrawDrawing();
    setStatus(getStatus()+1); // clear mouse button hints
    updateMouseButtonHints();
    m_graphicView->killSelectActions();
    finish(false);
}

void RS_ActionBlocksCreate::mouseMoveEvent(QMouseEvent* e) {
    snapPoint(e);

    switch (getStatus()) {
        case SetReferencePoint: {
            //data.insertionPoint = snapPoint(e);
/*if (block) {
          deletePreview();
          //preview->addAllFrom(*block);
          //preview->move(data.insertionPoint);
  RS_Creation creation(preview, nullptr, false);
              creation.createInsert(data);
          drawPreview();
  }*/
            break;
        }
        default:
            break;
    }
}

void RS_ActionBlocksCreate::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        fireCoordinateEvent(snapPoint(e));
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}

void RS_ActionBlocksCreate::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint: {
            *m_referencePoint = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionBlocksCreate::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionBlocksCreate::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
