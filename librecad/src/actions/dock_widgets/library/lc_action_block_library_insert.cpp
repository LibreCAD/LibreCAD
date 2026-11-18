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

#include "lc_action_block_library_insert.h"

#include <QFileInfo>
#include <QPointer>

#include "lc_block_library_insert_options_filler.h"
#include "lc_block_library_insert_options_widget.h"
#include "lc_documentsstorage.h"
#include "lc_offsetimportsession.h"
#include "rs_creation.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_units.h"

/**
 * Data needed to insert library items.
 */
struct LC_ActionBlockLibraryInsert::RS_LibraryInsertData {
    QString file;
    RS_Vector insertionPoint;
    double factor = 0.;
    double angle = 0.;
    RS_Graphic* graphic{nullptr};
};

struct LC_ActionBlockLibraryInsert::ActionData {
    std::unique_ptr<RS_Graphic> prev;
    QPointer<LC_OffsetImportSession> importSession;
    RS_LibraryInsertData data;
};

// fixme - sand - UCS - support of UCS for inserting blocks (angle)!!!

/**
 * Constructor.
 */
LC_ActionBlockLibraryInsert::LC_ActionBlockLibraryInsert(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionLibraryInsert", actionContext, RS2::ActionLibraryInsert),
      m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionBlockLibraryInsert::~LC_ActionBlockLibraryInsert() = default;

void LC_ActionBlockLibraryInsert::doSaveOptions() {
    save("Angle", getAngle());
    save("Factor", getFactor());
}

void LC_ActionBlockLibraryInsert::doLoadOptions() {
    const double angle = loadDouble("Angle", 0.0);
    const double factor = loadDouble("Factor", 1.0);

    setAngle(angle);
    setFactor(factor);
}

void LC_ActionBlockLibraryInsert::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

// fixme - blocks - review usage of this methods, why it's called from outside? Should it be part of the action or widget?
void LC_ActionBlockLibraryInsert::setFile(const QString& file) {
    m_actionData->data.file = file;
    if (m_actionData->importSession) {
        m_actionData->importSession->cancel();
        delete m_actionData->importSession.data();
        m_actionData->importSession = nullptr;
    }
    m_actionData->prev.reset();
    deletePreview();

    const auto type = RS_FileIO::detectFormat(file);
    if (!LC_OffsetImportSession::supportsType(type)) {
        m_actionData->prev = std::make_unique<RS_Graphic>();
        if (!LC_DocumentsStorage{}.loadDocument(m_actionData->prev.get(), file, type)) {
            m_actionData->prev.reset();
            commandMessage(tr("Cannot open file '%1'").arg(file));
        }
        updateActionPrompt();
        return;
    }

    auto* session = new LC_OffsetImportSession(this);
    m_actionData->importSession = session;
    const QPointer<LC_ActionBlockLibraryInsert> alive(this);
    const QPointer<LC_OffsetImportSession> pending(session);
    if (!session->start(file, type, RS2::DxfTextEncoding::Declared, false,
        [alive, pending, file](LC_OffsetImportSession::Outcome outcome) {
            if (!alive || !pending || alive->m_actionData->importSession != pending) return;
            alive->m_actionData->importSession = nullptr;
            pending->deleteLater();
            if (outcome.status != LC_OffsetImportSession::Status::Complete || !outcome.graphic) {
                alive->commandMessage(alive->tr("Cannot open file '%1'").arg(file));
                alive->updateActionPrompt();
                return;
            }
            if (outcome.sourceUnverified || !outcome.sourceStamp.matches(file)) {
                alive->commandMessage(alive->tr("The library drawing changed while it was loading; select it again."));
                alive->updateActionPrompt();
                return;
            }
            outcome.graphic->onLoadingCompleted();
            alive->m_actionData->prev = std::move(outcome.graphic);
            alive->updateActionPrompt();
        })) {
        m_actionData->importSession = nullptr;
        delete session;
        commandMessage(tr("Cannot start loading file '%1'").arg(file));
    }
    updateActionPrompt();
}

void LC_ActionBlockLibraryInsert::reset() const {
    if (m_actionData->importSession) {
        m_actionData->importSession->cancel();
        delete m_actionData->importSession.data();
        m_actionData->importSession = nullptr;
    }
    auto& data = m_actionData->data;
    data.insertionPoint = {};
    data.factor = 1.0;
    data.angle = 0.0;
    m_actionData->prev.reset();
}

bool LC_ActionBlockLibraryInsert::isSourceLoading() const noexcept {
    return m_actionData->importSession && m_actionData->importSession->busy();
}

bool LC_ActionBlockLibraryInsert::hasSource() const noexcept {
    return bool(m_actionData->prev);
}

bool LC_ActionBlockLibraryInsert::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (!m_actionData->prev) {
        commandMessage(tr("The library drawing is not ready yet."));
        return false;
    }
    auto insertData = m_actionData->data;
    insertData.graphic = m_actionData->prev.get();
    insertData.angle = toWorldAngleFromUCSBasis(m_actionData->data.angle);
    RS_Graphic* insertGraphic = insertData.graphic;
    if (insertGraphic != nullptr) {
        const QString insertFileName = QFileInfo(insertData.file).completeBaseName();
        const LC_LibraryInsertData pasteData(insertData.insertionPoint, insertData.factor, insertData.angle, insertFileName, insertGraphic);
        RS_Modification::libraryInsert(pasteData, m_graphic, ctx);
        // fixme- create separate method for library insert!
    }
    return true;
}

void LC_ActionBlockLibraryInsert::doTriggerCompletion(const bool success) {
    LC_UndoableDocumentModificationAction::doTriggerCompletion(success);
}

void LC_ActionBlockLibraryInsert::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (!m_actionData->prev) {
        deletePreview();
        return;
    }
    switch (status) {
        case SetTargetPoint: {
            m_actionData->data.insertionPoint = e->snapPoint;

            const auto& data = m_actionData->data;
            //if (block) {
            m_preview->addAllFrom(*m_actionData->prev, m_viewport);
            m_preview->move(data.insertionPoint);
            if (m_graphic != nullptr) {
                const RS_Vector scaleFactor = LC_CopyUtils::getInterGraphicsScaleFactor(
                    data.factor, m_actionData->prev.get(), m_graphic);
                m_preview->scale(data.insertionPoint, scaleFactor);
            }
            m_preview->rotate(data.insertionPoint, toWorldAngleFromUCSBasis(data.angle));
            // too slow:
            //RS_Creation creation(preview, NULL, false);
            //creation.createInsert(data);
            //}
            break;
        }
        default:
            break;
    }
}

void LC_ActionBlockLibraryInsert::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    if (!m_actionData->prev) {
        commandMessage(tr("The library drawing is still loading or could not be opened."));
        return;
    }
    fireCoordinateEvent(e->snapPoint);
}

void LC_ActionBlockLibraryInsert::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

void LC_ActionBlockLibraryInsert::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    if (!m_actionData->prev) {
        commandMessage(tr("The library drawing is still loading or could not be opened."));
        return;
    }
    m_actionData->data.insertionPoint = pos;
    trigger();
}

bool LC_ActionBlockLibraryInsert::doProcessCommand(int status, const QString& command) {
    bool accept = true;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", command)) {
                deletePreview();
                m_lastStatus = SetTargetPoint;
                setStatus(SetAngle);
                accept = true;
            }
            else if (checkCommand("factor", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetFactor);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok = false;
            const double a = RS_Math::eval(command, &ok);
            if (ok) {
                m_actionData->data.angle = RS_Math::deg2rad(a);
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
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionBlockLibraryInsert::getAvailableCommands() {
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

void LC_ActionBlockLibraryInsert::updateActionPrompt() {
    switch (getStatus()) {
        case SetTargetPoint:
            updatePromptTRCancel(isSourceLoading() ? tr("Loading library drawing...")
                : (hasSource() ? tr("Specify reference point") : tr("Library drawing unavailable; cancel to stop")));
            break;
        case SetAngle:
            updatePrompt(tr("Enter angle:"));
            break;
        case SetFactor:
            updatePrompt(tr("Enter factor:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

double LC_ActionBlockLibraryInsert::getAngle() const {
    return m_actionData->data.angle;
}

void LC_ActionBlockLibraryInsert::setAngle(const double a) const {
    m_actionData->data.angle = a;
}

double LC_ActionBlockLibraryInsert::getFactor() const {
    return m_actionData->data.factor;
}

void LC_ActionBlockLibraryInsert::setFactor(const double f) const {
    m_actionData->data.factor = f;
}

RS2::CursorType LC_ActionBlockLibraryInsert::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionBlockLibraryInsert::createOptionsWidget() {
    return new LC_BlockLibraryInsertOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionBlockLibraryInsert::createOptionsFiller() {
    return new LC_BlockLibraryInsertOptionsFiller();
}

bool LC_ActionBlockLibraryInsert::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}
