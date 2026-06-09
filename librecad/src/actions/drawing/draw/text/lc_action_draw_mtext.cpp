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

#include "lc_action_draw_mtext.h"

#include "lc_m_text_options_filler.h"
#include "lc_m_text_options_widget.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_mtext.h"
#include "rs_preview.h"

LC_ActionDrawMText::LC_ActionDrawMText(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawMText", actionContext, RS2::ActionDrawMText), m_pos(std::make_unique<RS_Vector>()),
      m_textChanged(true) {
}

LC_ActionDrawMText::~LC_ActionDrawMText() = default;

void LC_ActionDrawMText::doSaveOptions() {
    // save("Angle", m_ucsBasicAngleDegrees);
}

void LC_ActionDrawMText::doLoadOptions() {
    // m_ucsBasicAngleDegrees = loadDouble("Angle", 0.0);
}

bool LC_ActionDrawMText::isInVisualSnapStatus(int status) {
    return status == SetPos;
}

void LC_ActionDrawMText::init(const int status) {
    RS_PreviewActionInterface::init(status);
    switch (status) {
        case ShowDialog: {
            reset();
            RS_MText tmp(nullptr, *m_mtextData);
            if (RS_DIALOGFACTORY->requestMTextDialog(&tmp, m_viewport)) {
                const RS_MTextData& editedData = tmp.getData();
                m_mtextData.reset(new RS_MTextData(editedData));
                setStatus(SetPos);
                updateOptions();
            }
            else {
                hideOptions();
                finish();
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

void LC_ActionDrawMText::reset() {
    const QString text = (m_mtextData != nullptr) ? m_mtextData->text : "";
    m_mtextData = std::make_unique<RS_MTextData>(RS_Vector(0.0, 0.0), 1.0, 100.0, RS_MTextData::VATop, RS_MTextData::HALeft,
                                                 RS_MTextData::LeftToRight, RS_MTextData::Exact, 1.0, text, "standard", 0.0, RS2::Update);
}

RS_Entity* LC_ActionDrawMText::doTriggerCreateEntity() {
    if (m_pos->valid) {
        const auto text = new RS_MText(m_document, *m_mtextData);
        text->update();
        return text;
    }
    return nullptr;
}

void LC_ActionDrawMText::doTriggerCompletion(const bool success) {
    if (success) {
        m_textChanged = true;
        setStatus(SetPos);
    }
}

void LC_ActionDrawMText::preparePreview() {
    m_mtextData->insertionPoint = *m_pos;
    auto text = std::make_unique<RS_MText>(m_preview.get(), *m_mtextData);
    text->update();
    previewEntity(text.get());
    text.release();
    m_textChanged = false;
    m_preview->setVisible(true);
}

void LC_ActionDrawMText::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status == SetPos) {
        RS_Vector mouse = e->snapPoint;
        mouse = getRelZeroAwarePoint(e, mouse);
        const RS_Vector mov = mouse - *m_pos;
        *m_pos = mouse;
        if (m_textChanged || !m_pos->valid || m_preview->isEmpty()) {
            preparePreview();
        }
        else {
            m_preview->move(mov);
            m_preview->setVisible(true);
        }
    }
}

void LC_ActionDrawMText::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawMText::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    finish();
}

void LC_ActionDrawMText::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case ShowDialog:
            break;
        case SetPos: {
            m_mtextData->insertionPoint = coord;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawMText::doProcessCommand(const int status, const QString& command) {
    bool accept = true;

    switch (status) {
        case SetPos: {
            if (checkCommand("text", command)) {
                deletePreview();
                disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            }
            break;
        }
        case SetText: {
            setText(command);
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

QStringList LC_ActionDrawMText::getAvailableCommands() {
    QStringList cmd;
    if (getStatus() == SetPos) {
        cmd += command("text");
    }
    return cmd;
}

void LC_ActionDrawMText::updateActionPrompt() {
    switch (getStatus()) {
        case SetPos:
            updatePromptTRCancel(tr("Specify insertion point"));
            break;
        case ShowDialog:
        case SetText:
            updatePromptTRBack(tr("Enter text:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawMText::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionDrawMText::setText(const QString& t) {
    m_mtextData->text = t;
    m_textChanged = true;
}

QString LC_ActionDrawMText::getText() const {
    return m_mtextData->text;
}

void LC_ActionDrawMText::setUcsAngleDegrees(const double ucsRelAngleDegrees) {
    m_mtextData->angle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
    m_textChanged = true;
}

void LC_ActionDrawMText::setUcsAngle(const double ucsRelAngle) {
    m_mtextData->angle = toWorldAngleFromUCSBasis(ucsRelAngle);
    m_textChanged = true;
}

double LC_ActionDrawMText::getUcsAngleDegrees() const {
    return toUCSBasisAngleDegrees(m_mtextData->angle);
}

bool LC_ActionDrawMText::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setUcsAngle(angleRad);
        return true;
    }
    return false;
}

LC_ActionOptionsWidget* LC_ActionDrawMText::createOptionsWidget() {
    return new LC_MTextOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawMText::createOptionsFiller() {
    return new LC_MTextOptionsFiller();
}
