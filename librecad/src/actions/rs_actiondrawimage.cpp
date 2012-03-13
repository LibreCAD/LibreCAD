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

#include "rs_actiondrawimage.h"

#include <QAction>
// #include <QDebug>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"

/**
 * Constructor.
 */
RS_ActionDrawImage::RS_ActionDrawImage(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Image",
                               container, graphicView) {
}



RS_ActionDrawImage::~RS_ActionDrawImage() {}


QAction* RS_ActionDrawImage::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    // tr("Image")
    QAction* action = new QAction(tr("Insert &Image"),  NULL);
    action->setIcon(QIcon(":/extui/menuimage.png"));
    //action->zetStatusTip(tr("Insert Image (Bitmap)"));
    return action;
}

void RS_ActionDrawImage::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();

    data.file = RS_DIALOGFACTORY->requestImageOpenDialog();
    // RVT_PORT should we really redarw here?? graphicView->redraw();

    if (!data.file.isEmpty()) {
        //std::cout << "file: " << data.file << "\n";
        //qDebug() << "file: " << data.file;

        img = QImage(data.file);
        setStatus(SetTargetPoint);
    } else {
        setFinished();
        //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    }
}



void RS_ActionDrawImage::reset() {
    data = RS_ImageData(0,
                        RS_Vector(0.0,0.0),
                        RS_Vector(1.0,0.0),
                        RS_Vector(0.0,1.0),
                        RS_Vector(1.0,1.0),
                        "",
                        50, 50, 0);
}



void RS_ActionDrawImage::trigger() {
    deletePreview();

    if (!data.file.isEmpty()) {
        RS_Creation creation(container, graphicView);
        creation.createImage(data);
    }

    graphicView->redraw(RS2::RedrawDrawing);

    //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    finish(false);
    updateToolBar();
}


void RS_ActionDrawImage::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
    case SetTargetPoint:
        data.insertionPoint = snapPoint(e);

        deletePreview();
        //RS_Creation creation(preview, NULL, false);
        //creation.createInsert(data);
        RS_Line* line;
        line = new RS_Line(preview,
                           RS_LineData(RS_Vector(0, 0),
                                       RS_Vector(img.width(), 0)));
        preview->addEntity(line);
        line = new RS_Line(preview,
                           RS_LineData(RS_Vector(img.width(), 0),
                                       RS_Vector(img.width(), img.height())));
        preview->addEntity(line);
        line = new RS_Line(preview,
                           RS_LineData(RS_Vector(img.width(),
                                                 img.height()), RS_Vector(0, img.height())));
        preview->addEntity(line);
        line = new RS_Line(preview,
                           RS_LineData(RS_Vector(0, img.height()),
                                       RS_Vector(0, 0)));
        preview->addEntity(line);
        preview->scale(RS_Vector(0,0),
                       RS_Vector(data.uVector.magnitude(), data.uVector.magnitude()));
        preview->rotate(RS_Vector(0,0), data.uVector.angle());
        preview->move(data.insertionPoint);

        drawPreview();
        break;

    default:
        break;
    }
}



void RS_ActionDrawImage::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        //init(getStatus()-1);
        finish(false);
    }
}



void RS_ActionDrawImage::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    data.insertionPoint = e->getCoordinate();
    trigger();
}



void RS_ActionDrawImage::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetTargetPoint:
        if (checkCommand("angle", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetAngle);
        } else if (checkCommand("factor", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetFactor);
        }
        break;

    case SetAngle: {
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok==true) {
            setAngle(RS_Math::deg2rad(a));
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
        break;

    case SetFactor: {
        bool ok;
        double f = RS_Math::eval(c, &ok);
        if (ok==true) {
            setFactor(f);
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        setStatus(lastStatus);
    }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawImage::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetTargetPoint:
        cmd += command("angle");
        cmd += command("factor");
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionDrawImage::showOptions() {
    RS_ActionInterface::showOptions();

    if(RS_DIALOGFACTORY!=NULL){
        RS_DIALOGFACTORY->requestOptions(this, true);
    }
}



void RS_ActionDrawImage::hideOptions() {
    RS_ActionInterface::hideOptions();
    if(RS_DIALOGFACTORY!=NULL){
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}


void RS_ActionDrawImage::updateMouseButtonHints() {
    if(RS_DIALOGFACTORY==NULL) return;
    switch (getStatus()) {
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter angle:"),
                                            "");
        break;
    case SetFactor:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter factor:"),
                                            "");
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDrawImage::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



//void RS_ActionDrawImage::updateToolBar() {
//    if(RS_DIALOGFACTORY==NULL) return;
//    if (isFinished()) {
//        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
//    }
//    /*
//    //not needed any more with new snap
//    return;
//    if (!isFinished()) {
//        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
//    } else {
//        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
//    }
//    */
//}


// EOF
