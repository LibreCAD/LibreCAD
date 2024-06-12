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

#include <QAction>
#include <QMouseEvent>

#include "rs_actiondrawmtext.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_mtext.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"
#include "qg_mtextoptions.h"

RS_ActionDrawMText::RS_ActionDrawMText(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Text",
						   container, graphicView)
        ,pos(std::make_unique<RS_Vector>())
		,textChanged(true){
	actionType=RS2::ActionDrawMText;
}

RS_ActionDrawMText::~RS_ActionDrawMText() = default;

void RS_ActionDrawMText::init(int status){
    RS_ActionInterface::init(status);

    switch (status) {
        case ShowDialog: {
            reset();
            RS_MText tmp(nullptr, *data);
            if (RS_DIALOGFACTORY->requestMTextDialog(&tmp)){
                data.reset(new RS_MTextData(tmp.getData()));
                setStatus(SetPos);
                updateOptions();
            } else {
                hideOptions();
                finish(true);
            }
            break;
        }
        case SetPos: {
            updateOptions();
            deletePreview();
            preview->setVisible(true);
            preparePreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawMText::reset() {
    const QString text= (data != nullptr) ? data->text : "";
    data = std::make_unique<RS_MTextData>(RS_Vector(0.0,0.0),
                                          1.0, 100.0,
                                          RS_MTextData::VATop,
                                          RS_MTextData::HALeft,
                                          RS_MTextData::LeftToRight,
                                          RS_MTextData::Exact,
                                          1.0,
                                          text,
                                          "standard",
                                          0.0,
                                          RS2::Update);
}

void RS_ActionDrawMText::trigger(){

    RS_DEBUG->print("RS_ActionDrawText::trigger()");

    if (pos->valid){
        deletePreview();

        auto *text = new RS_MText(container, *data);
        text->update();
        container->addEntity(text);

        addToDocumentUndoable(text);

        graphicView->redraw(RS2::RedrawDrawing);

        textChanged = true;
        setStatus(SetPos);
    }
}

void RS_ActionDrawMText::preparePreview() {
    data->insertionPoint = *pos;
    auto *text = new RS_MText(preview.get(), *data);
    text->update();
    previewEntity(text);
    textChanged = false;
}

void RS_ActionDrawMText::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawText::mouseMoveEvent begin");

    if (getStatus() == SetPos){
        RS_Vector mouse = snapPoint(e);
        bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
        if (shiftPressed){
            RS_Vector relZero = graphicView->getRelativeZero();
            if (relZero.valid){
               mouse = relZero;
            }
        }
        RS_Vector mov = mouse - *pos;
        *pos = mouse;
        if (textChanged || pos->valid == false || preview->isEmpty()){
            deletePreview();
            preparePreview();
        } else {
            preview->move(mov);
            preview->setVisible(true);
        }
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawText::mouseMoveEvent end");
}

void RS_ActionDrawMText::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        fireCoordinateEventForSnap(e);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        //init(getStatus()-1);
        finish(false);
    }
}

void RS_ActionDrawMText::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr){
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
        case ShowDialog:
            break;
        case SetPos: {
            data->insertionPoint = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawMText::commandEvent(RS_CommandEvent *e){
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)){
        commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
        case SetPos: {
            if (checkCommand("text", c)){
                deletePreview();
                graphicView->disableCoordinateInput();
                setStatus(SetText);
            }
            break;
        }
        case SetText: {
            setText(e->getCommand());
            updateOptions();
            graphicView->enableCoordinateInput();
            setStatus(SetPos);
            break;
        }
        default:
            break;
    }
}

QStringList RS_ActionDrawMText::getAvailableCommands() {
    QStringList cmd;
    if (getStatus()==SetPos) {
        cmd += command("text");
    }
    return cmd;
}

void RS_ActionDrawMText::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPos:
            updateMouseWidgetTRCancel("Specify insertion point");
            break;
        case ShowDialog:
        case SetText:
            updateMouseWidgetTRBack("Enter text:");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionDrawMText::updateMouseCursor(){
    setMouseCursor(RS2::CadCursor);
}

void RS_ActionDrawMText::setText(const QString &t){
    data->text = t;
    textChanged = true;
}

QString RS_ActionDrawMText::getText(){
    return data->text;
}

void RS_ActionDrawMText::setAngle(double a){
    data->angle = a;
    textChanged = true;
}

double RS_ActionDrawMText::getAngle(){
    return data->angle;
}

void RS_ActionDrawMText::createOptionsWidget(){
    m_optionWidget = std::make_unique<QG_MTextOptions>();
}

// EOFI
