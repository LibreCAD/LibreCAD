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

#include "lc_action_draw_image.h"

#include <QImage>

#include "lc_graphicviewport.h"
#include "lc_image_options_filler.h"
#include "lc_image_options_widget.h"
#include "rs_creation.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_document.h"
#include "rs_image.h"
#include "rs_preview.h"
#include "rs_units.h"

struct LC_ActionDrawImage::ImageData {
    RS_ImageData data;
    QImage img;
};

/**
 * Constructor.
 */
LC_ActionDrawImage::LC_ActionDrawImage(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawImage", actionContext, RS2::ActionDrawImage), m_imageData(std::make_unique<ImageData>()) {
}

LC_ActionDrawImage::~LC_ActionDrawImage() = default;

void LC_ActionDrawImage::doSaveOptions() {
    save("Angle", m_ucsAngleDegree);
    save("Factor", m_factor);
}

void LC_ActionDrawImage::doLoadOptions() {
    m_ucsAngleDegree =  loadDouble("Angle", 0.0);
    setUcsAngleDegrees(m_ucsAngleDegree);

    m_factor = loadDouble("Factor", 1.0);
    setFactor(m_factor);
}

bool LC_ActionDrawImage::isInVisualSnapStatus(int status) {
    return (status == SetTargetPoint)/* || (status == SetAngle)*/;
}

void LC_ActionDrawImage::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
    m_imageData->data.file = RS_DIALOGFACTORY->requestImageOpenDialog();
    if (!m_imageData->data.file.isEmpty()) {
        m_imageData->img = QImage(m_imageData->data.file);
        setStatus(SetTargetPoint);
    }
    else {
        setFinished();
    }
}

void LC_ActionDrawImage::reset()  {
    m_imageData->data = {0, {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, "", 50, 50, 0};
    setUcsAngleDegrees(m_ucsAngleDegree);
    setFactor(m_factor);
}

RS_Entity* LC_ActionDrawImage::doTriggerCreateEntity() {
    if (!m_imageData->data.file.isEmpty()) {
        auto* img = new RS_Image(m_document, m_imageData->data);
        img->update();
        return img;
    }
    return nullptr;
}

void LC_ActionDrawImage::doTriggerCompletion([[maybe_unused]] bool success) {
    m_viewport->zoomAuto();
    finish();
}

bool LC_ActionDrawImage::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setUcsAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

void LC_ActionDrawImage::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status == SetTargetPoint) {
        const bool snappedToRelZero = trySnapToRelZeroCoordinateEvent(e);
        if (!snappedToRelZero) {
            m_imageData->data.insertionPoint = e->snapPoint;
            // fixme - ucs - review this code
            //RS_Creation creation(preview, nullptr, false);
            //creation.createInsert(data);
            const double w = m_imageData->img.width();
            const double h = m_imageData->img.height();
            previewLine({0., 0.}, {w, 0.});
            previewLine({w, 0.}, {w, h});
            previewLine({w, h}, {0., h});
            previewLine({0., h}, {0., 0.});

            m_preview->scale({0., 0.}, {m_imageData->data.uVector.magnitude(), m_imageData->data.uVector.magnitude()});
            m_preview->rotate({0., 0.}, m_imageData->data.uVector.angle());
            m_preview->move(m_imageData->data.insertionPoint);
        }
    }
}

void LC_ActionDrawImage::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawImage::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    finish();
}

void LC_ActionDrawImage::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    m_imageData->data.insertionPoint = pos;
    trigger();
}

bool LC_ActionDrawImage::doProcessCommand(int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetAngle);
                accept = true;
            }
            else if (checkCommand("factor", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetFactor);
                accept = true;
            }
            else if (checkCommand("dpi", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetDPI);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            double wcsAngle;
            const bool ok = parseToWCSAngle(command, wcsAngle);
            if (ok) {
                setAngle(wcsAngle);
                accept = true;
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetFactor: {
            bool ok = false;
            const double f = RS_Math::eval(command, &ok);
            if (ok) {
                setFactor(f);
                accept = true;
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetDPI: {
            bool ok = false;
            const double dpi = RS_Math::eval(command, &ok);

            if (ok) {
                setFactor(RS_Units::dpiToScale(dpi, m_document->getGraphicUnit()));
                accept = true;
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

double LC_ActionDrawImage::getUcsAngleDegrees() const {
    return toUCSBasisAngleDegrees(m_imageData->data.uVector.angle());
}

void LC_ActionDrawImage::setUcsAngleDegrees(const double ucsRelAngleDegrees){
    const double wcsAngle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
    setAngle(wcsAngle);
    m_ucsAngleDegree = ucsRelAngleDegrees;
}

void LC_ActionDrawImage::setAngle(const double wcsAngle) const {
    const double l = m_imageData->data.uVector.magnitude();
    m_imageData->data.uVector.setPolar(l, wcsAngle);
    m_imageData->data.vVector.setPolar(l, wcsAngle + M_PI_2);
}

double LC_ActionDrawImage::getFactor() const {
    return m_imageData->data.uVector.magnitude();
}

void LC_ActionDrawImage::setFactor(const double f) {
    const double a = m_imageData->data.uVector.angle();
    m_imageData->data.uVector.setPolar(f, a);
    m_imageData->data.vVector.setPolar(f, a + M_PI_2);
    m_factor = f;
}

double LC_ActionDrawImage::dpiToScale(const double dpi) const {
    return RS_Units::dpiToScale(dpi, m_document->getGraphicUnit());
}

double LC_ActionDrawImage::scaleToDpi(const double scale) const {
    return RS_Units::scaleToDpi(scale, m_document->getGraphicUnit());
}

RS2::CursorType LC_ActionDrawImage::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

QStringList LC_ActionDrawImage::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetTargetPoint:
            cmd += command("angle");
            cmd += command("factor");
            cmd += command("dpi");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawImage::updateActionPrompt() {
    switch (getStatus()) {
        case SetTargetPoint:
            updatePromptTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updatePrompt(tr("Enter angle:"));
            break;
        case SetFactor:
            updatePrompt(tr("Enter factor:"));
            break;
        case SetDPI:
            updatePrompt(tr("Enter dpi:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawImage::createOptionsWidget() {
    return new LC_ImageOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawImage::createOptionsFiller() {
    return new LC_ImageOptionsFiller();
}
