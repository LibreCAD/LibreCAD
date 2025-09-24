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


#include "rs_actionblocksinsert.h"

#include "qg_insertoptions.h"
#include "rs_block.h"
#include "rs_creation.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_preview.h"

/**
 * Constructor.
 */
// fixme - sand - ucs - SUPPORT UCS, ANGLES FOR INSERTION!
RS_ActionBlocksInsert::RS_ActionBlocksInsert(LC_ActionContext *actionContext)
    : RS_PreviewActionInterface("Blocks Insert", actionContext, RS2::ActionBlocksInsert),
      m_block(nullptr),
      m_lastStatus(SetUndefined){
    reset(); // init data Member
}

RS_ActionBlocksInsert::~RS_ActionBlocksInsert() = default;

void RS_ActionBlocksInsert::init(int status){
    RS_PreviewActionInterface::init(status);
    reset();

    if (m_graphic != nullptr) {
        m_block = m_graphic->getActiveBlock();
        if (m_block != nullptr) {
            QString blockName = m_block->getName();
            m_data->name = blockName;
            if (m_document->is(RS2::EntityBlock)) {
                QString parentBlockName = static_cast<RS_Block*>(m_document)->getName();
                if (parentBlockName == blockName) {
                    commandMessage(tr("Block cannot contain an insert of itself."));
                    finish(false);
                } else {
                    QStringList bnChain = m_block->findNestedInsert(parentBlockName);
                    if (!bnChain.empty()) {
                        // fixme - sand - think where to report the error...
                        commandMessage(blockName
                                       + tr(" has nested insert of current block in:\n")
                                       + bnChain.join("->")
                                       + tr("\nThis block cannot be inserted."));
                        finish(false);
                    }
                }
            }
        } else {
            finish(false);
        }
    }
}

void RS_ActionBlocksInsert::reset(){
    m_data.reset(new RS_InsertData("", RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0), 0.0,
                                   1, 1, RS_Vector(1.0, 1.0), nullptr, RS2::Update));
}

void RS_ActionBlocksInsert::trigger(){
    deletePreview();

    if (m_block != nullptr) {
        RS_Creation creation(m_container, m_viewport);
        m_data->updateMode = RS2::Update;
        auto insertData = m_data.get();
        auto insertDataCopy = new RS_InsertData(*insertData);
        insertDataCopy->angle = toWorldAngleFromUCSBasis(insertData->angle);
        creation.createInsert(insertDataCopy);
    }

    redrawDrawing();
}

void RS_ActionBlocksInsert::onMouseMoveEvent(int status, LC_MouseEvent *e){
    switch (status) {
        case SetTargetPoint: {
            m_data->insertionPoint = e->snapPoint;
            if (m_block != nullptr) {
                //preview->addAllFrom(*block);
                //preview->move(data->insertionPoint);
                RS_Creation creation(m_preview.get(), m_viewport, false);
                // Create insert as preview only
                m_data->updateMode = RS2::PreviewUpdate;
                auto insertData = m_data.get();
                auto insertDataCopy = new RS_InsertData(*insertData);
                insertDataCopy->angle = toWorldAngleFromUCSBasis(insertData->angle);
                insertDataCopy->updateMode = RS2::Update;
                creation.createInsert(insertDataCopy);
            }
            break;
        }
        default:
            break;
    }
}

bool RS_ActionBlocksInsert::doUpdateAngleByInteractiveInput(const QString& tag, double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

bool RS_ActionBlocksInsert::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
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

void RS_ActionBlocksInsert::onMouseLeftButtonRelease([[maybe_unused]] int status, LC_MouseEvent *e){
    fireCoordinateEvent(e->snapPoint);
}

void RS_ActionBlocksInsert::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e){
    initPrevious(status);
}

void RS_ActionBlocksInsert::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector &pos){
    m_data->insertionPoint = pos;
    trigger();
}

bool RS_ActionBlocksInsert::doProcessCommand(int status, const QString &c){
    bool accept = false;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("factor", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetFactor);
                accept = true;
            } else if (checkCommand("columns", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetColumns);
                accept = true;
            } else if (checkCommand("rows", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetRows);
                accept = true;
            } else if (checkCommand("columnspacing", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                accept = true;
                setStatus(SetColumnSpacing);
            } else if (checkCommand("rowspacing", c)) {
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetRowSpacing);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
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
        case SetColumns: {
            bool ok;
            int cols = (int) RS_Math::eval(c, &ok);
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
            bool ok;
            int rows = (int) RS_Math::eval(c, &ok);
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
            bool ok;
            double cs = (int) RS_Math::eval(c, &ok);
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
            bool ok;
            int rs = (int) RS_Math::eval(c, &ok);
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

double RS_ActionBlocksInsert::getAngle() const{
    return m_data->angle;
}

void RS_ActionBlocksInsert::setAngle(double a){
    m_data->angle = a;
}

double RS_ActionBlocksInsert::getFactor() const{
    return m_data->scaleFactor.x;
}

void RS_ActionBlocksInsert::setFactor(double f){
    m_data->scaleFactor = RS_Vector(f, f);
}

int RS_ActionBlocksInsert::getColumns() const{
    return m_data->cols;
}

void RS_ActionBlocksInsert::setColumns(int c){
    m_data->cols = c;
}

int RS_ActionBlocksInsert::getRows() const{
    return m_data->rows;
}

void RS_ActionBlocksInsert::setRows(int r){
    m_data->rows = r;
}

double RS_ActionBlocksInsert::getColumnSpacing() const{
    return m_data->spacing.x;
}

void RS_ActionBlocksInsert::setColumnSpacing(double cs){
    m_data->spacing.x = cs;
}

double RS_ActionBlocksInsert::getRowSpacing() const{
    return m_data->spacing.y;
}

void RS_ActionBlocksInsert::setRowSpacing(double rs){
    m_data->spacing.y = rs;
}

QStringList RS_ActionBlocksInsert::getAvailableCommands(){
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

void RS_ActionBlocksInsert::updateMouseButtonHints(){
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
        case SetColumns:
            updateMouseWidget(tr("Enter columns:"));
            break;
        case SetRows:
            updateMouseWidget(tr("Enter rows:"));
            break;
        case SetColumnSpacing:
            updateMouseWidget(tr("Enter column spacing:"));
            break;
        case SetRowSpacing:
            updateMouseWidget(tr("Enter row spacing:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionBlocksInsert::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget *RS_ActionBlocksInsert::createOptionsWidget(){
    return new QG_InsertOptions();
}
