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

#include "rs_actionblocksinsert.h"
#include "rs_block.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_insertoptions.h"


/**
 * Constructor.
 */
RS_ActionBlocksInsert::RS_ActionBlocksInsert(RS_EntityContainer& container,
											 RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Blocks Insert",
							   container, graphicView)
	,block(nullptr)
	,lastStatus(SetUndefined){
	actionType = RS2::ActionBlocksInsert;
	reset();    // init data Member
}

RS_ActionBlocksInsert::~RS_ActionBlocksInsert()  = default;

void RS_ActionBlocksInsert::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();

    if (graphic) {
        block = graphic->getActiveBlock();
        if (block) {
            QString blockName = block->getName();
            data->name = blockName;
            if (document->is(RS2::EntityBlock)) {
                QString parentBlockName = ((RS_Block*)(document))->getName();
                if (parentBlockName == blockName) {
                    commandMessage(tr("Block cannot contain an insert of itself."));
                    finish(false);
                } else {
                    QStringList bnChain = block->findNestedInsert(parentBlockName);
                    if (!bnChain.empty()) {
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

void RS_ActionBlocksInsert::reset() {
	data.reset(new RS_InsertData("",
                         RS_Vector(0.0,0.0),
                         RS_Vector(1.0,1.0),
                         0.0,
                         1, 1,
                         RS_Vector(1.0,1.0),
                         nullptr,
						 RS2::Update));
}

void RS_ActionBlocksInsert::trigger() {
    deletePreview();

    //RS_Modification m(*container, graphicView);
//m.paste(img->insertionPoint);
    //std::cout << *RS_Clipboard::instance();

    if (block) {
        RS_Creation creation(container, graphicView);
        data->updateMode = RS2::Update;
        creation.createInsert(data.get());
    }

    graphicView->redraw(RS2::RedrawDrawing);

    //finish();
}

void RS_ActionBlocksInsert::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
        case SetTargetPoint: {
            data->insertionPoint = snapPoint(e);

            if (block) {
                deletePreview();
                //preview->addAllFrom(*block);
//preview->move(data->insertionPoint);
                RS_Creation creation(preview.get(), nullptr, false);
// Create insert as preview only
                data->updateMode = RS2::PreviewUpdate;
                creation.createInsert(data.get());
                drawPreview();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionBlocksInsert::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionBlocksInsert::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionBlocksInsert::onCoordinateEvent([[maybe_unused]]int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    data->insertionPoint = pos;
    trigger();
}

bool RS_ActionBlocksInsert::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetTargetPoint: {
            if (checkCommand("angle", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetAngle);
                accept= true;
            } else if (checkCommand("factor", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetFactor);
                accept= true;
            } else if (checkCommand("columns", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetColumns);
                accept= true;
            } else if (checkCommand("rows", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetRows);
                accept= true;
            } else if (checkCommand("columnspacing", c)) {
                deletePreview();
                lastStatus = (Status) status;
                accept= true;
                setStatus(SetColumnSpacing);
            } else if (checkCommand("rowspacing", c)) {
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetRowSpacing);
                accept= true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                accept= true;
                data->angle = RS_Math::deg2rad(a);
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetFactor: {
            bool ok;
            double f = RS_Math::eval(c, &ok);
            if (ok) {
                setFactor(f);
                accept= true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetColumns: {
            bool ok;
            int cols = (int)RS_Math::eval(c, &ok);
            if (ok) {
                data->cols = cols;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetRows: {
            bool ok;
            int rows = (int)RS_Math::eval(c, &ok);
            if (ok) {
                data->rows = rows;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetColumnSpacing: {
            bool ok;
            double cs = (int)RS_Math::eval(c, &ok);
            if (ok) {
                data->spacing.x = cs;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetRowSpacing: {
            bool ok;
            int rs = (int)RS_Math::eval(c, &ok);
            if (ok) {
                data->spacing.y = rs;
                accept  = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

double RS_ActionBlocksInsert::getAngle() const {
    return data->angle;
}

void RS_ActionBlocksInsert::setAngle(double a) {
    data->angle = a;
}

double RS_ActionBlocksInsert::getFactor() const {
    return data->scaleFactor.x;
}

void RS_ActionBlocksInsert::setFactor(double f) {
    data->scaleFactor = RS_Vector(f, f);
}

int RS_ActionBlocksInsert::getColumns() const {
    return data->cols;
}

void RS_ActionBlocksInsert::setColumns(int c) {
    data->cols = c;
}

int RS_ActionBlocksInsert::getRows() const {
    return data->rows;
}

void RS_ActionBlocksInsert::setRows(int r) {
    data->rows = r;
}

double RS_ActionBlocksInsert::getColumnSpacing() const {
    return data->spacing.x;
}

void RS_ActionBlocksInsert::setColumnSpacing(double cs) {
    data->spacing.x = cs;
}

double RS_ActionBlocksInsert::getRowSpacing() const {
    return data->spacing.y;
}

void RS_ActionBlocksInsert::setRowSpacing(double rs) {
    data->spacing.y = rs;
}

QStringList RS_ActionBlocksInsert::getAvailableCommands() {
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

void RS_ActionBlocksInsert::updateMouseButtonHints() {
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

LC_ActionOptionsWidget* RS_ActionBlocksInsert::createOptionsWidget() {
    return new QG_InsertOptions();
}
