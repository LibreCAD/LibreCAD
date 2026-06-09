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


#include "lc_action_block_insert.h"

#include <memory>

#include "lc_block_insert_options_filler.h"
#include "lc_block_insert_options_widget.h"
#include "rs_block.h"
#include "rs_creation.h"
#include "rs_graphic.h"
#include "rs_insert.h"

/**
 * Constructor.
 */
// fixme - sand - ucs - SUPPORT UCS, ANGLES FOR INSERTION!
LC_ActionBlockInsert::LC_ActionBlockInsert(LC_ActionContext *actionContext)
    : LC_SingleEntityCreationAction("ActionBlocksInsert", actionContext, RS2::ActionBlocksInsert){
    reset(); // init data Member
    m_data = std::make_unique<RS_InsertData>("", RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0,
                                 1, 1, RS_Vector(1.0, 1.0), nullptr, RS2::Update);
}

LC_ActionBlockInsert::~LC_ActionBlockInsert() = default;

void LC_ActionBlockInsert::doSaveOptions() {
    save("Angle", getAngle());
    save("Factor", getFactor());
    save("Columns", getColumns());
    save("Rows", getRows());
    save("ColumnSpacing", getColumnSpacing());
    save("RowSpacing", getRowSpacing());
}

void LC_ActionBlockInsert::doLoadOptions() {
    const double angle = loadDouble("Angle", 1.0);
    setAngle(angle);
    const double factor = loadDouble("Factor", getFactor());
    setFactor(factor);
    const int columns = loadInt("Columns", getColumns());
    setColumns(columns);
    const int rows = loadInt("Rows", getRows());
    setRows(rows);
    const double columnSpacing = loadDouble("ColumnSpacing", getColumnSpacing());
    setColumnSpacing(columnSpacing);
    const double rowSpacing = loadDouble("RowSpacing", getRowSpacing());
    setRowSpacing(rowSpacing);
}

void LC_ActionBlockInsert::init(const int status){
    RS_PreviewActionInterface::init(status);
    reset();

    if (m_graphic != nullptr) {
        m_block = m_graphic->getActiveBlock();
        if (m_block != nullptr) {
            const QString blockName = m_block->getName();
            m_data->name = blockName;
            if (m_document->is(RS2::EntityBlock)) {
                const QString parentBlockName = static_cast<RS_Block*>(m_document)->getName();
                if (parentBlockName == blockName) {
                    commandMessage(tr("Block cannot contain an insert of itself."));
                    finish();
                } else {
                    const QStringList bnChain = m_block->findNestedInsert(parentBlockName);
                    if (!bnChain.empty()) {
                        // fixme - sand - think where to report the error...
                        commandMessage(blockName
                                       + tr(" has nested insert of current block in:\n")
                                       + bnChain.join("->")
                                       + tr("\nThis block cannot be inserted."));
                        finish();
                    }
                }
            }
        } else {
            finish();
        }
    }
}

void LC_ActionBlockInsert::reset(){}

RS_Entity* LC_ActionBlockInsert::doTriggerCreateEntity() {
    if (m_block != nullptr) {
        m_data->updateMode = RS2::Update;
        const auto insertData = m_data.get();
        const auto insertDataCopy = new RS_InsertData(*insertData);
        insertDataCopy->angle = toWorldAngleFromUCSBasis(insertData->angle);

        const auto ins = new RS_Insert(m_document, *insertDataCopy);
        ins->update();
        return ins;
    }
    return nullptr;
}

void LC_ActionBlockInsert::doTriggerCompletion([[maybe_unused]]bool success) {
}

void LC_ActionBlockInsert::onMouseMoveEvent(const int status, const LC_MouseEvent* event){
    switch (status) {
        case SetTargetPoint: {
            m_data->insertionPoint = event->snapPoint;
            if (m_block != nullptr) {
                m_data->updateMode = RS2::PreviewUpdate;
                const auto insertData = m_data.get();
                const auto insertDataCopy = new RS_InsertData(*insertData);
                insertDataCopy->angle = toWorldAngleFromUCSBasis(insertData->angle);
                insertDataCopy->updateMode = RS2::Update;

                const auto insert = new RS_Insert(m_document, *insertDataCopy);
                // insert->update();
                previewEntity(insert);
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionBlockInsert::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

bool LC_ActionBlockInsert::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "spacingX") {
        setColumnSpacing(distance);
        return true;
    }
    if (tag == "spacingY") {
        setRowSpacing(distance);
        return true;
    }
    return false;
}

void LC_ActionBlockInsert::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e){
    fireCoordinateEvent(e->snapPoint);
}

void LC_ActionBlockInsert::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e){
    initPrevious(status);
}

void LC_ActionBlockInsert::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector &pos){
    m_data->insertionPoint = pos;
    trigger();
}

bool LC_ActionBlockInsert::doProcessCommand(int status, const QString &command){
    bool accept = false;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("factor", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetFactor);
                accept = true;
            } else if (checkCommand("columns", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetColumns);
                accept = true;
            } else if (checkCommand("rows", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetRows);
                accept = true;
            } else if (checkCommand("columnspacing", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                accept       = true;
                setStatus(SetColumnSpacing);
            } else if (checkCommand("rowspacing", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetRowSpacing);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok = false;
            const double a = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                m_data->angle = RS_Math::deg2rad(a);
            } else {
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
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetColumns: {
            bool ok = false;
            const int cols = static_cast<int>(RS_Math::eval(command, &ok));
            if (ok) {
                m_data->cols = cols;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetRows: {
            bool ok = false;
            const int rows = static_cast<int>(RS_Math::eval(command, &ok));
            if (ok) {
                m_data->rows = rows;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetColumnSpacing: {
            bool ok = false;
            const double cs = static_cast<int>(RS_Math::eval(command, &ok));
            if (ok) {
                m_data->spacing.x = cs;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetRowSpacing: {
            bool ok = false;
            const int rs = static_cast<int>(RS_Math::eval(command, &ok));
            if (ok) {
                m_data->spacing.y = rs;
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

double LC_ActionBlockInsert::getAngle() const{
    return m_data->angle;
}

void LC_ActionBlockInsert::setAngle(const double a) const {
    m_data->angle = a;
}

double LC_ActionBlockInsert::getFactor() const{
    return m_data->scaleFactor.x;
}

void LC_ActionBlockInsert::setFactor(const double f) const {
    m_data->scaleFactor = RS_Vector(f, f);
}

int LC_ActionBlockInsert::getColumns() const{
    return m_data->cols;
}

void LC_ActionBlockInsert::setColumns(const int c) const {
    m_data->cols = c;
}

int LC_ActionBlockInsert::getRows() const{
    return m_data->rows;
}

void LC_ActionBlockInsert::setRows(const int r) const {
    m_data->rows = r;
}

double LC_ActionBlockInsert::getColumnSpacing() const{
    return m_data->spacing.x;
}

void LC_ActionBlockInsert::setColumnSpacing(const double cs) const {
    m_data->spacing.x = cs;
}

double LC_ActionBlockInsert::getRowSpacing() const{
    return m_data->spacing.y;
}

void LC_ActionBlockInsert::setRowSpacing(const double rs) const {
    m_data->spacing.y = rs;
}

QStringList LC_ActionBlockInsert::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetTargetPoint:
            cmd += command("angle");
            cmd += command("factor");
            cmd += command("columns");
            cmd += command("rows");
            cmd += command("columnspacing");
            cmd += command("rowspacing");
            break;
        default:
            break;
    }

    return cmd;
}

void LC_ActionBlockInsert::updateActionPrompt(){
    switch (getStatus()) {
        case SetTargetPoint:
            updatePromptTRCancel(tr("Specify reference point"));
            break;
        case SetAngle:
            updatePrompt(tr("Enter angle:"));
            break;
        case SetFactor:
            updatePrompt(tr("Enter factor:"));
            break;
        case SetColumns:
            updatePrompt(tr("Enter columns:"));
            break;
        case SetRows:
            updatePrompt(tr("Enter rows:"));
            break;
        case SetColumnSpacing:
            updatePrompt(tr("Enter column spacing:"));
            break;
        case SetRowSpacing:
            updatePrompt(tr("Enter row spacing:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionBlockInsert::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget *LC_ActionBlockInsert::createOptionsWidget(){
    return new LC_BlockInsertOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionBlockInsert::createOptionsFiller() {
    return new LC_BlockInsertOptionsFiller();
}
