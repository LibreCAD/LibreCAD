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

#include "rs_actionlibraryinsert.h"

#include "lc_documentsstorage.h"
#include "qg_libraryinsertoptions.h"
#include "rs_creation.h"
#include "rs_graphic.h"
#include "rs_preview.h"
#include "rs_units.h"

struct RS_ActionLibraryInsert::ActionData {
    RS_Graphic* prev;
    RS_LibraryInsertData data;
};

// fixme - sand - UCS - support of UCS for inserting blocks (angle)!!!

/**
 * Constructor.
 */
RS_ActionLibraryInsert::RS_ActionLibraryInsert(LC_ActionContext *actionContext)
        :RS_PreviewActionInterface("Library Insert", actionContext, RS2::ActionLibraryInsert)
		, m_actionData(std::make_unique<ActionData>())
		,m_lastStatus(SetTargetPoint){
}

RS_ActionLibraryInsert::~RS_ActionLibraryInsert() = default;

void RS_ActionLibraryInsert::init(int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

void RS_ActionLibraryInsert::setFile(const QString& file) {
    m_actionData->data.file = file;
    LC_DocumentsStorage storage;
    delete m_actionData->prev;
    m_actionData->prev = new RS_Graphic();
    if (!storage.loadDocument(m_actionData->prev, file, RS2::FormatUnknown)) {
        commandMessage(tr("Cannot open file '%1'").arg(file));
    }
}

void RS_ActionLibraryInsert::reset() {
    m_actionData->data.insertionPoint = {};
    m_actionData->data.factor = 1.0;
    m_actionData->data.angle = 0.0;
    delete m_actionData->prev;
}

void RS_ActionLibraryInsert::trigger() {
    deletePreview();
    RS_Creation creation(m_container, m_viewport);
    auto insertData    = m_actionData->data;
    insertData.graphic = m_actionData->prev;
    insertData.angle = toWorldAngleFromUCSBasis(m_actionData->data.angle);
    creation.createLibraryInsert(insertData);
    redrawDrawing();
}

void RS_ActionLibraryInsert::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetTargetPoint: {
            m_actionData->data.insertionPoint = e->snapPoint;
            //if (block) {
            m_preview->addAllFrom(*m_actionData->prev, m_viewport);
            m_preview->move(m_actionData->data.insertionPoint);
            m_preview->scale(m_actionData->data.insertionPoint,
                           RS_Vector(m_actionData->data.factor, m_actionData->data.factor));
            // unit conversion:
            if (m_graphic) {
                double const uf = RS_Units::convert(1.0, m_actionData->prev->getUnit(),
                                                    m_graphic->getUnit());
                m_preview->scale(m_actionData->data.insertionPoint,
                               {uf, uf});
            }
            m_preview->rotate(m_actionData->data.insertionPoint, toWorldAngleFromUCSBasis(m_actionData->data.angle));
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

void RS_ActionLibraryInsert::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    fireCoordinateEvent(e->snapPoint);
}

void RS_ActionLibraryInsert::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

void RS_ActionLibraryInsert::onCoordinateEvent([[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    m_actionData->data.insertionPoint = pos;
    trigger();
}

bool RS_ActionLibraryInsert::doProcessCommand(int status, const QString &c) {
    bool accept = true;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", c)) {
                deletePreview();
                m_lastStatus = SetTargetPoint;
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("factor", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetFactor);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                m_actionData->data.angle = RS_Math::deg2rad(a);
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetFactor: {
            bool ok;
            double f = RS_Math::eval(c, &ok);
            if (ok) {
                setFactor(f);
                accept = true;
            } else {
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

QStringList RS_ActionLibraryInsert::getAvailableCommands() {
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

void RS_ActionLibraryInsert::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetTargetPoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"));
            break;
        case SetAngle:
            updateMouseWidget(tr("Enter angle:"));
            break;
        case SetFactor:
            updateMouseWidget(tr("Enter factor:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

double RS_ActionLibraryInsert::getAngle() const{
    return m_actionData->data.angle;
}

void RS_ActionLibraryInsert::setAngle(double a) {
    m_actionData->data.angle = a;
}

double RS_ActionLibraryInsert::getFactor() const{
    return m_actionData->data.factor;
}

void RS_ActionLibraryInsert::setFactor(double f) {
    m_actionData->data.factor = f;
}

RS2::CursorType RS_ActionLibraryInsert::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionLibraryInsert::createOptionsWidget() {
    return new QG_LibraryInsertOptions();
}
