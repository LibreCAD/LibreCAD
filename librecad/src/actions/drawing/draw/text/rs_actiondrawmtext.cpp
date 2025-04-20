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

#include "rs_actiondrawmtext.h"

#include "qg_mtextoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_mtext.h"
#include "rs_preview.h"

RS_ActionDrawMText::RS_ActionDrawMText(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Text",actionContext, RS2::ActionDrawMText)
    ,m_pos(std::make_unique<RS_Vector>()),m_textChanged(true){
}

RS_ActionDrawMText::~RS_ActionDrawMText() = default;

void RS_ActionDrawMText::init(int status){
    RS_PreviewActionInterface::init(status);
    switch (status) {
        case ShowDialog: {
            reset();
            RS_MText tmp(nullptr, *m_mtextData);
            if (RS_DIALOGFACTORY->requestMTextDialog(&tmp, m_viewport)){
                const RS_MTextData &editedData = tmp.getData();
                m_mtextData.reset(new RS_MTextData(editedData));
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
            m_preview->setVisible(true);
            preparePreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawMText::reset() {
    const QString text= (m_mtextData != nullptr) ? m_mtextData->text : "";
    m_mtextData = std::make_unique<RS_MTextData>(RS_Vector(0.0,0.0),
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

void RS_ActionDrawMText::doTrigger() {
    RS_DEBUG->print("RS_ActionDrawText::trigger()");
    if (m_pos->valid){
        auto text = std::make_unique<RS_MText>(m_container, *m_mtextData);
        text->update();
        undoCycleAdd(text.get());
        text.release();
        m_textChanged = true;
        setStatus(SetPos);
    }
}

void RS_ActionDrawMText::preparePreview() {
    m_mtextData->insertionPoint = *m_pos;
    auto text = std::make_unique<RS_MText>(m_preview.get(), *m_mtextData);
    text->update();
    previewEntity(text.get());
    text.release();
    m_textChanged = false;
    m_preview->setVisible(true);
}

void RS_ActionDrawMText::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    if (status == SetPos){
        RS_Vector mouse = e->snapPoint;
        mouse = getRelZeroAwarePoint(e, mouse);
        RS_Vector mov = mouse - *m_pos;
        *m_pos = mouse;
        if (m_textChanged || m_pos->valid == false || m_preview->isEmpty()){
            preparePreview();
        } else {
            m_preview->move(mov);
            m_preview->setVisible(true);
        }
    }
}

void RS_ActionDrawMText::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawMText::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    finish(false);
}

void RS_ActionDrawMText::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case ShowDialog:
            break;
        case SetPos: {
            m_mtextData->insertionPoint = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawMText::doProcessCommand(int status, const QString &c) {
    bool accept = true;

    switch (status) {
        case SetPos: {
            if (checkCommand("text", c)){
                deletePreview();
                disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            }
            break;
        }
        case SetText: {
            setText(c);
            updateOptions();
            enableCoordinateInput();
            setStatus(SetPos);
            accept = true;
            break;
        }
        default:
            break;
    }
    return accept;
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
            updateMouseWidgetTRCancel(tr("Specify insertion point"));
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
RS2::CursorType RS_ActionDrawMText::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void RS_ActionDrawMText::setText(const QString &t){
    m_mtextData->text = t;
    m_textChanged = true;
}

QString RS_ActionDrawMText::getText(){
    return m_mtextData->text;
}

void RS_ActionDrawMText::setUcsAngleDegrees(double ucsRelAngleDegrees){
    m_mtextData->angle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
    m_textChanged = true;
}

double RS_ActionDrawMText::getUcsAngleDegrees(){
    return toUCSBasisAngleDegrees(m_mtextData->angle);
}

LC_ActionOptionsWidget* RS_ActionDrawMText::createOptionsWidget(){
    return new QG_MTextOptions();
}
