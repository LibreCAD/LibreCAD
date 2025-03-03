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

#include "rs_actiondrawtext.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_text.h"
#include "qg_textoptions.h"

struct RS_ActionDrawText::Points {
	RS_Vector pos;
	RS_Vector secPos;
};

RS_ActionDrawText::RS_ActionDrawText(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Text",
						   container, graphicView)
		, pPoints(std::make_unique<Points>())
		,textChanged(true){
    actionType=RS2::ActionDrawText;
}

RS_ActionDrawText::~RS_ActionDrawText() = default;

// FIXME - INSERTION INTO RELATIVE POINT SUPOPRT
void RS_ActionDrawText::init(int status){
    RS_ActionInterface::init(status);

    switch (status) {
        case ShowDialog: {
            reset();
            RS_Text tmp(nullptr, *data);
            if (RS_DIALOGFACTORY->requestTextDialog(&tmp)){
                data.reset(new RS_TextData(tmp.getData()));
                setStatus(SetPos);
                updateOptions();
            } else {
                hideOptions();
                setFinished();
            }
            break;
        }
        case SetPos:{
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

void RS_ActionDrawText::reset(){
    const QString text = data.get() ? data->text : "";
    data.reset(new RS_TextData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0),
                               1.0, 1.0,
                               RS_TextData::VABaseline,
                               RS_TextData::HALeft,
                               RS_TextData::None,
                               text,
                               "standard",
                               0.0,
                               RS2::Update));
}

void RS_ActionDrawText::trigger(){
    RS_DEBUG->print("RS_ActionDrawText::trigger()");

    if (pPoints->pos.valid){
        deletePreview();

        auto *text = new RS_Text(container, *data);
        text->update();
        container->addEntity(text);

        addToDocumentUndoable(text);

        graphicView->redraw(RS2::RedrawDrawing);

        textChanged = true;
        pPoints->secPos = {};
        setStatus(SetPos);
    }
}

void RS_ActionDrawText::preparePreview(){
    if (data->halign == RS_TextData::HAFit || data->halign == RS_TextData::HAAligned){
        if (pPoints->secPos.valid){
            auto *text = new RS_Line(pPoints->pos, pPoints->secPos);
            previewEntity(text);
        }
    } else {
        data->insertionPoint = pPoints->pos;
        auto *text = new RS_Text(preview.get(), *data);
        text->update();
        previewEntity(text);
    }
    textChanged = false;
}

void RS_ActionDrawText::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawText::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if (getStatus() == SetPos){
        if (isShift(e)){
            RS_Vector relZero = graphicView->getRelativeZero();
            if (relZero.valid){
                mouse = relZero;
            }
        }
        RS_Vector mov = mouse - pPoints->pos;
        pPoints->pos = mouse;
        if (textChanged || pPoints->pos.valid == false || preview->isEmpty()){
            deletePreview();
            preparePreview();
        } else {
            preview->move(mov);
            preview->setVisible(true);
        }
        drawPreview();
    } else if (getStatus() == SetSecPos){
        pPoints->secPos = mouse;
        deletePreview();
        preparePreview();
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawText::mouseMoveEvent end");
}

void RS_ActionDrawText::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawText::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    finish(false);
}

void RS_ActionDrawText::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case ShowDialog:
            break;
        case SetPos: {
            data->insertionPoint = mouse;
            if (data->halign == RS_TextData::HAFit || data->halign == RS_TextData::HAAligned)
                setStatus(SetSecPos);
            else
                trigger();
            break;
        }
        case SetSecPos: {
            data->secondPoint = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawText::doProcessCommand(int status, const QString &c) {
    bool accept = true;
    switch (status) {
        case SetPos: {
            if (checkCommand("text", c)){
                deletePreview();
                graphicView->disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            }
            break;
        }
        case SetText: {
            setText(c);
            updateOptions();
            graphicView->enableCoordinateInput();
            setStatus(SetPos);
            accept = true;
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawText::getAvailableCommands(){
    QStringList cmd;
    if (getStatus() == SetPos){
        cmd += command("text");
    }
    return cmd;
}

void RS_ActionDrawText::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPos:
            updateMouseWidgetTRCancel(tr("Specify insertion point"));
            break;
        case SetSecPos:
            updateMouseWidgetTRCancel(tr("Specify second point"));
            break;
        case ShowDialog:
        case SetText:
            updateMouseWidgetTRBack(tr("Enter text:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionDrawText::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void RS_ActionDrawText::setText(const QString &t){
    data->text = t;
    textChanged = true;
}

const QString &RS_ActionDrawText::getText() const{
    return data->text;
}

void RS_ActionDrawText::setAngle(double a){
    data->angle = a;
    textChanged = true;
}

double RS_ActionDrawText::getAngle() const{
    return data->angle;
}

LC_ActionOptionsWidget* RS_ActionDrawText::createOptionsWidget(){
    return new QG_TextOptions();
}
