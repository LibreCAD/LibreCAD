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
#include "rs_actiondrawlinebisector.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLineBisector::Points {
	/** Mouse pos when choosing the 1st line */
	RS_Vector coord1;
	/** Mouse pos when choosing the 2nd line */
	RS_Vector coord2;
};

RS_ActionDrawLineBisector::RS_ActionDrawLineBisector(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Draw Bisectors", container, graphicView)
		,bisector(nullptr)
		,line1(nullptr)
		,line2(nullptr)
		,length(10.)
		,number(1)
		, pPoints(new Points{})
		,lastStatus(SetLine1)
{
	actionType=RS2::ActionDrawLineBisector;
}

RS_ActionDrawLineBisector::~RS_ActionDrawLineBisector() = default;


void RS_ActionDrawLineBisector::setLength(double l) {
	length = l;
}

double RS_ActionDrawLineBisector::getLength() const{
	return length;
}

void RS_ActionDrawLineBisector::setNumber(int n) {
	number = n;
}

int RS_ActionDrawLineBisector::getNumber() const {
	return number;
}


void RS_ActionDrawLineBisector::init(int status) {
	RS_PreviewActionInterface::init(status);
	if(status>=0) {
		RS_Snapper::suspend();
	}

	if (status<SetLine2) {
		if(line2 && line2->isHighlighted()){
			line2->setHighlighted(false);
		}
		if(status<0 && line1 && line1->isHighlighted()){
			line1->setHighlighted(false);
		}
		graphicView->redraw(RS2::RedrawDrawing);
	}
}

void RS_ActionDrawLineBisector::trigger() {
    RS_PreviewActionInterface::trigger();

	for(auto p: {line1, line2}){
		if(p && p->isHighlighted()){
			p->setHighlighted(false);
		}
	}
	graphicView->redraw(RS2::RedrawDrawing);

    RS_Creation creation(container, graphicView);
	creation.createBisector(pPoints->coord1,
							pPoints->coord2,
                            length,
                            number,
                            line1,
							line2);
}



void RS_ActionDrawLineBisector::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineBisector::mouseMoveEvent begin");

    RS_Vector mouse = RS_Vector(graphicView->toGraphX(e->x()),
                                graphicView->toGraphY(e->y()));

    switch (getStatus()) {
    case SetLine1:
        break;

    case SetLine2: {
			pPoints->coord2 = mouse;
            RS_Entity* en = catchEntity(e, RS2::ResolveAll);
			if(en==line1) break;
			if (en && en->rtti()==RS2::EntityLine) {
				if(line2 && line2->isHighlighted()){
					line2->setHighlighted(false);
				}
				line2 = static_cast<RS_Line*>(en);
				line2->setHighlighted(true);
				graphicView->redraw(RS2::RedrawDrawing);

                deletePreview();

				RS_Creation creation(preview.get(), nullptr, false);
				creation.createBisector(pPoints->coord1,
										pPoints->coord2,
                                        length,
                                        number,
                                        line1,
                                        line2);
                drawPreview();
			}else{
				if(line2 && line2->isHighlighted()){
					line2->setHighlighted(false);
					graphicView->redraw(RS2::RedrawDrawing);
				}
				line2=nullptr;

			}
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineBisector::mouseMoveEvent end");
}



void RS_ActionDrawLineBisector::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    } else {

        RS_Vector mouse = RS_Vector(graphicView->toGraphX(e->x()),
                                    graphicView->toGraphY(e->y()));

        switch (getStatus()) {
        case SetLine1: {
				pPoints->coord1 = mouse;
                RS_Entity* en = catchEntity(e, RS2::ResolveAll);
				if (en && en->rtti()==RS2::EntityLine) {
					line1 = static_cast<RS_Line*>(en);
					line1->setHighlighted(true);
					graphicView->redraw(RS2::RedrawDrawing);
					line2=nullptr;
					setStatus(SetLine2);
				}
            }
            break;

        case SetLine2:
			pPoints->coord2 = mouse;
            trigger();
            setStatus(SetLine1);
            break;
        }
    }

}


void RS_ActionDrawLineBisector::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetLine1:
    case SetLine2:
        lastStatus = (Status)getStatus();
        if (checkCommand("length", c)) {
            deletePreview();
            setStatus(SetLength);
        } else if (checkCommand("number", c)) {
            deletePreview();
            setStatus(SetNumber);
        }
        break;

    case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
                length = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetNumber: {
            bool ok;
            int n = (int)RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
                if(n>0 && n<=200)
                    number = n;
                else
                     RS_DIALOGFACTORY->commandMessage(tr("Number sector lines not in range: ", "number of bisector to create must be in [1, 200]")+QString::number(n));
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



QStringList RS_ActionDrawLineBisector::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetLine1:
    case SetLine2:
        cmd += command("length");
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionDrawLineBisector::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetLine1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first line"),
                                            tr("Cancel"));
        break;
    case SetLine2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second line"),
                                            tr("Back"));
        break;
    case SetLength:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter bisector length:"),
                                            tr("Back"));
        break;
    case SetNumber:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number of bisectors:"),
                                            tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawLineBisector::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawLineBisector::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawLineBisector::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
