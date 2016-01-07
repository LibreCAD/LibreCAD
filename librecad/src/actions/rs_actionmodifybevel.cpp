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

#include "rs_actionmodifybevel.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionModifyBevel::Points {
	RS_Vector coord1;
	RS_Vector coord2;
	RS_BevelData data;
};

RS_ActionModifyBevel::RS_ActionModifyBevel(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Bevel Entities",
						   container, graphicView)
		,entity1(nullptr)
		,entity2(nullptr)
		, pPoints(new Points{})
		,lastStatus(SetEntity1)
{
	actionType=RS2::ActionModifyBevel;
}

RS_ActionModifyBevel::~RS_ActionModifyBevel() = default;


void RS_ActionModifyBevel::init(int status) {
    RS_ActionInterface::init(status);

    //snapMode = RS2::SnapFree;
    snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionModifyBevel::trigger() {

    RS_DEBUG->print("RS_ActionModifyBevel::trigger()");

    if (entity1 && entity1->isAtomic() &&
            entity2 && entity2->isAtomic()) {

        RS_Modification m(*container, graphicView);
		m.bevel(pPoints->coord1, (RS_AtomicEntity*)entity1,
				pPoints->coord2, (RS_AtomicEntity*)entity2,
				pPoints->data);

		pPoints->coord1 = {};
		pPoints->coord2 = {};
        entity1 = nullptr;
        entity2 = nullptr;
        setStatus(SetEntity1);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    }
}

void RS_ActionModifyBevel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent begin");

    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
    RS_Entity* se = catchEntity(e, RS2::ResolveAllButTextImage);

    switch (getStatus()) {
    case SetEntity1:
		pPoints->coord1 = mouse;
        entity1 = se;
        break;

    case SetEntity2:
                if (entity1 && RS_Information::isTrimmable(entity1)) {
				pPoints->coord2 = mouse;
                entity2 = se;
                }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent end");
}



void RS_ActionModifyBevel::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity1:
            if (entity1 && entity1->isAtomic()) {
                setStatus(SetEntity2);
            }
            break;

        case SetEntity2:
            if (entity2 && entity2->isAtomic() &&
                            RS_Information::isTrimmable(entity1, entity2)) {
                trigger();
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyBevel::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
        if (checkCommand("length1", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength1);
        } else if (checkCommand("length2", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength2);
        } else if (checkCommand("trim", c)) {
			pPoints->data.trim = !pPoints->data.trim;
            RS_DIALOGFACTORY->requestOptions(this, true, true);
        }
        break;

    case SetLength1: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
				pPoints->data.length1 = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetLength2: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
				pPoints->data.length2 = l;
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

void RS_ActionModifyBevel::setLength1(double l1) {
	pPoints->data.length1 = l1;
}

double RS_ActionModifyBevel::getLength1() const{
	return pPoints->data.length1;
}

void RS_ActionModifyBevel::setLength2(double l2) {
	pPoints->data.length2 = l2;
}

double RS_ActionModifyBevel::getLength2() const{
	return pPoints->data.length2;
}

void RS_ActionModifyBevel::setTrim(bool t) {
	pPoints->data.trim = t;
}

bool RS_ActionModifyBevel::isTrimOn() const{
	return pPoints->data.trim;
}

QStringList RS_ActionModifyBevel::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
        cmd += command("length1");
        cmd += command("length2");
        cmd += command("trim");
        break;
    default:
        break;
    }
    return cmd;
}

void RS_ActionModifyBevel::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionModifyBevel::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionModifyBevel::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first entity"),
                                            tr("Cancel"));
        break;
    case SetEntity2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second entity"),
                                            tr("Back"));
        break;
    case SetLength1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length 1:"),
                                            tr("Back"));
        break;
    case SetLength2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length 2:"),
                                            tr("Back"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionModifyBevel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
