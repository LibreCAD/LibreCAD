/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
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


#include "rs_line.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"

#include "lc_actionsnapmiddlemanual.h"


/*
    This action class can snap (set) the relative-zero marker
    in the middle between two points chosen by the user.

    The middle point, by default, rests at a poistion 50% relative 
    to the position of the first point chosen, that is, 
    at the center of the imaginary line connecting the 
    two user-defined points.

    This percentage, however, can be modified by the user 
    in order to place the marker at a different position 
    along the imaginary line.
*/


LC_ActionSnapMiddleManual::LC_ActionSnapMiddleManual( RS_EntityContainer& container, 
                                                      RS_GraphicView& graphicView, RS_Pen input_currentAppPen) 
                                                      :
                                                      RS_PreviewActionInterface("Snap Middle Manual", container, graphicView), 
                                                      currentAppPen(input_currentAppPen)
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::LC_ActionSnapMiddleManual");

    actionType = RS2::ActionSnapMiddleManual;
}


LC_ActionSnapMiddleManual::~LC_ActionSnapMiddleManual()// = default;
{
    document->setActivePen(currentAppPen);

    signalUnsetSnapMiddleManual();
}


void LC_ActionSnapMiddleManual::init(int status)
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::init");

    RS_PreviewActionInterface::init(status);

    percentage = 50.0;

    drawSnapper();
}


void LC_ActionSnapMiddleManual::mouseMoveEvent(QMouseEvent* e)
{
    RS_Vector mouse = snapPoint(e);

    if (getStatus() == SetEndPoint)
    {
        /* Snapping to an angle of 15 degrees, if the shift key is pressed. */
        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = snapToAngle(mouse, startPoint, 15.0);
        }

        deletePreview();

        RS_Line *line = new RS_Line(startPoint, mouse);

        preview->addEntity(line);
        line->setLayerToActive();
        line->setPenToActive();

        drawPreview();
    }
}


void LC_ActionSnapMiddleManual::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        RS_Vector snapped = snapPoint(e);

        /* Snapping to an angle of 15 degrees, if the shift key is pressed. */
        if ((e->modifiers() & Qt::ShiftModifier) && (getStatus() == SetEndPoint))
        {
            snapped = snapToAngle(snapped, startPoint, 15.0);
        }

        RS_CoordinateEvent ce(snapped);

        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();

        switch (getStatus())
        {
            case SetPercentage:
            case SetStartPoint:
                init(-1);finish(false);
                break;

            default:
                setStatus(SetPercentage);
                init(getStatus());
        }
    }
}


void LC_ActionSnapMiddleManual::coordinateEvent(RS_CoordinateEvent* e)
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::coordinateEvent");

    if (e == nullptr)
    {
        RS_DEBUG->print("LC_ActionSnapMiddleManual::coordinateEvent: event was nullptr");
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetPercentage:
        case SetStartPoint:
            startPoint = mouse;
            setStatus(SetEndPoint);
            graphicView->moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;

        case SetEndPoint:
            /* Refuse zero length lines. */
            if ((mouse - startPoint).squared() > RS_TOLERANCE2)
            {
                endPoint = mouse;

                const RS_Vector middleManualPoint = startPoint + ((endPoint - startPoint) * (percentage / 100.0));

                graphicView->moveRelativeZero(middleManualPoint);

                if (predecessor != NULL)
                {
                    if (predecessor->getName().compare("Default") != 0)
                    {
                        document->setActivePen(currentAppPen);
			            RS_CoordinateEvent new_e (middleManualPoint);
                        predecessor->coordinateEvent(&new_e);
                        init(-1);
                    }
                }

                setStatus(SetPercentage);
                updateMouseButtonHints();
                init(getStatus());
            }
            break;
    }

    RS_DEBUG->print("LC_ActionSnapMiddleManual::coordinateEvent: OK");
}


void LC_ActionSnapMiddleManual::commandEvent(RS_CommandEvent* inputCommandEvent)
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::commandEvent");

    QString inputCommand = inputCommandEvent->getCommand().toLower();

    switch (getStatus())
    {
        case SetPercentage:
            {
                bool ok;
                percentage = inputCommand.QString::toDouble(&ok);
                if (ok)
                {
                    setStatus(SetStartPoint);
                    inputCommandEvent->accept();
                    updateMouseButtonHints();
                }
                else
                {
                    percentage = 50.0;
                }
            }
            break;

        case SetStartPoint:
            if (checkCommand("help", inputCommand))
            {
                RS_DIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
                inputCommandEvent->accept();
                return;
            }
            break;

        case SetEndPoint:
            if (checkCommand("close", inputCommand))
            {
                setStatus(SetPercentage);
                inputCommandEvent->accept();
                updateMouseButtonHints();
                return;
            }
            break;

        default:
            return;
    }

    RS_DEBUG->print("LC_ActionSnapMiddleManual::commandEvent: OK");
}


QStringList LC_ActionSnapMiddleManual::getAvailableCommands()
{
    QStringList actionCommandsList;

    switch (getStatus())
    {
        case SetEndPoint:
            actionCommandsList += command("close");
            break;
    }

    return actionCommandsList;
}


void LC_ActionSnapMiddleManual::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetPercentage:
            RS_DIALOGFACTORY->updateMouseWidget( tr("Specify percentage / start-point"), 
                                                 tr("Cancel"));
            break;

        case SetStartPoint:
            RS_DIALOGFACTORY->updateMouseWidget( tr("Specify start point"), 
                                                 tr("Cancel"));
            break;

        case SetEndPoint:
            RS_DIALOGFACTORY->updateMouseWidget( tr("Specify end point"), 
                                                 tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

