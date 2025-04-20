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

#include "rs_actiondrawtext.h"

#include "qg_textoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_text.h"

struct RS_ActionDrawText::ActionData {
	RS_Vector pos;
	RS_Vector secPos;
};

RS_ActionDrawText::RS_ActionDrawText(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Text",actionContext, RS2::ActionDrawText)
	, m_actionData(std::make_unique<ActionData>())
	,m_textChanged(true){
}

RS_ActionDrawText::~RS_ActionDrawText() = default;


void RS_ActionDrawText::init(int status){
    RS_PreviewActionInterface::init(status);
    switch (status) {
        case ShowDialog: {
            reset();
            RS_Text tmp(nullptr, *m_textData);
            
            if (RS_DIALOGFACTORY->requestTextDialog(&tmp, m_viewport)){
                const RS_TextData &editedData = tmp.getData();
                m_textData.reset(new RS_TextData(editedData));
                setStatus(SetPos);
                updateOptions();
            } else {
                hideOptions();
                setStatus(-1);
                finish(false);
            }
            break;
        }
        case SetPos:{
            updateOptions();
            deletePreview();
            m_preview->setVisible(true);
            preparePreview();
            break;
        }
        default:
            if (status < 0) {
                m_actionData.release();
                m_textData.release();
            }
            break;
    }
}

void RS_ActionDrawText::reset(){
    const QString text = m_textData.get() ? m_textData->text : "";
    m_textData.reset(new RS_TextData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0),
                               1.0, 1.0,
                               RS_TextData::VABaseline,
                               RS_TextData::HALeft,
                               RS_TextData::None,
                               text,
                               "standard",
                               0.0,
                               RS2::Update));
}

void RS_ActionDrawText::doTrigger() {
    RS_DEBUG->print("RS_ActionDrawText::trigger()");
    if (m_actionData->pos.valid){
        m_textData->angle = toWorldAngleFromUCSBasisDegrees(m_ucsBasicAngleDegrees);
        auto *text = new RS_Text(m_container, *m_textData);
        text->update();

        undoCycleAdd(text);

        m_textChanged = true;
        m_actionData->secPos = {};
        if (m_snappedToRelZero){
            m_snappedToRelZero = false;
            setStatus(-1);
        }
        else {
            setStatus(SetPos);
        }
    }
}

void RS_ActionDrawText::preparePreview(){
    m_textData->angle = toWorldAngleFromUCSBasisDegrees(m_ucsBasicAngleDegrees);
    if (m_textData->halign == RS_TextData::HAFit || m_textData->halign == RS_TextData::HAAligned){
        if (m_actionData->secPos.valid){
            auto *text = new RS_Line(m_textData->insertionPoint, m_actionData->secPos);
            previewEntity(text);
        }
    } else {
        m_textData  ->insertionPoint = m_actionData->pos;
        auto *text = new RS_Text(m_preview.get(), *m_textData);
        text->update();
        previewEntity(text);
    }
    m_textChanged = false;
}

void RS_ActionDrawText::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetPos:{
            bool snapped = trySnapToRelZeroCoordinateEvent(e);
            if (!snapped) {
                m_actionData->pos = mouse;
                preparePreview();
            }
            break;
        }
        case SetSecPos:{
            mouse = getSnapAngleAwarePoint(e, m_textData->insertionPoint, mouse, true);
            m_actionData->secPos = mouse;
            preparePreview();
            break;
        }
        default:
            break;
    }
    appendInfoCursorZoneMessage(tr("Text: ")/*.append("\n")*/.append(m_textData->text), 2, false);
}

void RS_ActionDrawText::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Vector pos = e->snapPoint;
    switch (status){
        case SetPos:{
            pos = getRelZeroAwarePoint(e, pos);
            break;
        }
        case SetSecPos:{
            pos = getSnapAngleAwarePoint(e, m_textData->insertionPoint, pos, false);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(pos);
}

void RS_ActionDrawText::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    setStatus(-1);
}

void RS_ActionDrawText::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case ShowDialog:
            break;
        case SetPos: {
            m_textData->insertionPoint = mouse;
            if (m_textData->halign == RS_TextData::HAFit || m_textData->halign == RS_TextData::HAAligned)
                setStatus(SetSecPos);
            else
                trigger();
            break;
        }
        case SetSecPos: {
            m_textData->secondPoint = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

// fixme - sand - cmd - expand by other attributes (angle?, height?)
bool RS_ActionDrawText::doProcessCommand(int status, const QString &c) {
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
            updateMouseWidgetTRCancel(tr("Specify insertion point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetSecPos:
            updateMouseWidgetTRCancel(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
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
    m_textData->text = t;
    m_textChanged = true;
}

const QString &RS_ActionDrawText::getText() const{
    return m_textData->text;
}

void RS_ActionDrawText::setUcsAngleDegrees(double ucsRelAngleDegrees){
    m_ucsBasicAngleDegrees = ucsRelAngleDegrees;
    m_textData->angle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
    m_textChanged = true;
}

double RS_ActionDrawText::getUcsAngleDegrees() const{
    return m_ucsBasicAngleDegrees;
}

LC_ActionOptionsWidget* RS_ActionDrawText::createOptionsWidget(){
    return new QG_TextOptions();
}
