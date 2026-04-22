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

#include "lc_action_draw_text.h"

#include "lc_text_options_filler.h"
#include "lc_text_options_widget.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_text.h"

struct LC_ActionDrawText::ActionData {
    RS_Vector pos;
    RS_Vector secPos;
};

LC_ActionDrawText::LC_ActionDrawText(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawText", actionContext, RS2::ActionDrawText), m_actionData(std::make_unique<ActionData>()),
      m_textChanged(true) {
}

LC_ActionDrawText::~LC_ActionDrawText() = default;

void LC_ActionDrawText::doSaveOptions() {
    save("Angle", m_ucsBasicAngleDegrees);
}

void LC_ActionDrawText::doLoadOptions() {
    m_ucsBasicAngleDegrees = loadDouble("Angle", 0.0);
}

void LC_ActionDrawText::init(const int status) {
    RS_PreviewActionInterface::init(status);
    switch (status) {
        case ShowDialog: {
            reset();
            RS_Text tmp(nullptr, *m_textData);

            if (RS_DIALOGFACTORY->requestTextDialog(&tmp, m_viewport)) {
                const RS_TextData& editedData = tmp.getData();
                m_textData.reset(new RS_TextData(editedData));
                setStatus(SetPos);
                updateOptions();
            }
            else {
                hideOptions();
                setStatus(-1);
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
            if (status < 0) {
                m_actionData.reset();
                m_textData.reset();
            }
            break;
    }
}

void LC_ActionDrawText::reset() {
    const QString text = (m_textData != nullptr) ? m_textData->text : "";
    m_textData.reset(new RS_TextData(RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0), 1.0, 1.0, RS_TextData::VABaseline, RS_TextData::HALeft,
                                     RS_TextData::None, text, "standard", 0.0, RS2::Update));
}

RS_Entity* LC_ActionDrawText::doTriggerCreateEntity() {
    if (m_actionData->pos.valid) {
        m_textData->angle = toWorldAngleFromUCSBasisDegrees(m_ucsBasicAngleDegrees);
        auto* text = new RS_Text(m_document, *m_textData);
        text->update();
        return text;
    }
    return nullptr;
}

void LC_ActionDrawText::doTriggerCompletion(const bool success) {
    if (success) {
        m_textChanged = true;
        m_actionData->secPos = {};
        if (m_snappedToRelZero) {
            m_snappedToRelZero = false;
            setStatus(-1);
        }
        else {
            setStatus(SetPos);
        }
    }
}

void LC_ActionDrawText::preparePreview() {
    m_textData->angle = toWorldAngleFromUCSBasisDegrees(m_ucsBasicAngleDegrees);
    if (m_textData->halign == RS_TextData::HAFit || m_textData->halign == RS_TextData::HAAligned) {
        if (m_actionData->secPos.valid) {
            const auto* text = new RS_Line(m_textData->insertionPoint, m_actionData->secPos);
            previewEntity(text);
        }
    }
    else {
        m_textData->insertionPoint = m_actionData->pos;
        auto* text = new RS_Text(m_preview.get(), *m_textData);
        text->update();
        previewEntity(text);
    }
    m_textChanged = false;
}

void LC_ActionDrawText::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPos: {
            const bool snapped = trySnapToRelZeroCoordinateEvent(e);
            if (!snapped) {
                m_actionData->pos = mouse;
                preparePreview();
            }
            break;
        }
        case SetSecPos: {
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

void LC_ActionDrawText::onMouseLeftButtonRelease([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    RS_Vector pos = e->snapPoint;
    switch (status) {
        case SetPos: {
            pos = getRelZeroAwarePoint(e, pos);
            break;
        }
        case SetSecPos: {
            pos = getSnapAngleAwarePoint(e, m_textData->insertionPoint, pos, false);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(pos);
}

void LC_ActionDrawText::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    setStatus(-1);
}

void LC_ActionDrawText::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case ShowDialog:
            break;
        case SetPos: {
            m_textData->insertionPoint = coord;
            if (m_textData->halign == RS_TextData::HAFit || m_textData->halign == RS_TextData::HAAligned) {
                setStatus(SetSecPos);
            }
            else {
                trigger();
            }
            break;
        }
        case SetSecPos: {
            m_textData->secondPoint = coord;
            trigger();
            break;
        }
        default:
            break;
    }
}

// fixme - sand - cmd - expand by other attributes (angle?, height?)
bool LC_ActionDrawText::doProcessCommand(const int status, const QString& command) {
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

QStringList LC_ActionDrawText::getAvailableCommands() {
    QStringList cmd;
    if (getStatus() == SetPos) {
        cmd += command("text");
    }
    return cmd;
}

void LC_ActionDrawText::updateActionPrompt() {
    switch (getStatus()) {
        case SetPos:
            updatePromptTRCancel(tr("Specify insertion point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetSecPos:
            updatePromptTRCancel(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
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

RS2::CursorType LC_ActionDrawText::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionDrawText::setText(const QString& t) {
    m_textData->text = t;
    m_textChanged = true;
}

const QString& LC_ActionDrawText::getText() const {
    return m_textData->text;
}

void LC_ActionDrawText::setUcsAngleDegrees(const double ucsRelAngleDegrees) {
    m_ucsBasicAngleDegrees = ucsRelAngleDegrees;
    m_textData->angle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
    m_textChanged = true;
}

double LC_ActionDrawText::getUcsAngleDegrees() const {
    return m_ucsBasicAngleDegrees;
}

bool LC_ActionDrawText::isInVisualSnapStatus(int status) {
    return status == SetPos || status == SetSecPos;
}

bool LC_ActionDrawText::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setUcsAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

LC_ActionOptionsWidget* LC_ActionDrawText::createOptionsWidget() {
    return new LC_TextOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawText::createOptionsFiller() {
    return  new LC_TextOptionsFiller();
}
