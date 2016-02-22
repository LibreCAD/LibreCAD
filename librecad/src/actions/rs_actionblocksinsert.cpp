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

#include <QAction>
#include <QMouseEvent>
#include "rs_actionblocksinsert.h"
#include "rs_insert.h"
#include "rs_block.h"

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_math.h"
#include "rs_preview.h"

/**
 * Constructor.
 */
RS_ActionBlocksInsert::RS_ActionBlocksInsert(RS_EntityContainer& container,
											 RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Blocks Insert",
							   container, graphicView)
	,block(nullptr)
	,lastStatus(SetUndefined)
{
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
			data->name = block->getName();
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
                         NULL,
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
    case SetTargetPoint:
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

    default:
        break;
    }
}



void RS_ActionBlocksInsert::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionBlocksInsert::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

	data->insertionPoint = e->getCoordinate();
    trigger();
}



void RS_ActionBlocksInsert::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetTargetPoint:
        if (checkCommand("angle", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetAngle);
        } else if (checkCommand("factor", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetFactor);
        } else if (checkCommand("columns", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetColumns);
        } else if (checkCommand("rows", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetRows);
        } else if (checkCommand("columnspacing", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetColumnSpacing);
        } else if (checkCommand("rowspacing", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetRowSpacing);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
				data->angle = RS_Math::deg2rad(a);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetFactor: {
            bool ok;
            double f = RS_Math::eval(c, &ok);
            if (ok) {
                setFactor(f);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetColumns: {
            bool ok;
            int cols = (int)RS_Math::eval(c, &ok);
            if (ok) {
				data->cols = cols;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetRows: {
            bool ok;
            int rows = (int)RS_Math::eval(c, &ok);
            if (ok) {
				data->rows = rows;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetColumnSpacing: {
            bool ok;
            double cs = (int)RS_Math::eval(c, &ok);
            if (ok) {
				data->spacing.x = cs;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetRowSpacing: {
            bool ok;
            int rs = (int)RS_Math::eval(c, &ok);
            if (ok) {
				data->spacing.y = rs;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
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
        ;
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


void RS_ActionBlocksInsert::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionBlocksInsert::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionBlocksInsert::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter angle:"),
                                            "");
        break;
    case SetFactor:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter factor:"),
                                            "");
        break;
    case SetColumns:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter columns:"),
                                            "");
        break;
    case SetRows:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter rows:"),
                                            "");
        break;
    case SetColumnSpacing:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter column spacing:"),
                                            "");
        break;
    case SetRowSpacing:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter row spacing:"),
                                            "");
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionBlocksInsert::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
