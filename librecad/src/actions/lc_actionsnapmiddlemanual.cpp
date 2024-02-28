/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
** Copyright (C) 2024 Dongxu Li <dongxuli2011@gmail.com>
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

#include "lc_actionsnapmiddlemanual.h"

#include "rs_line.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"

constexpr double g_defaultRatio = 0.5;

struct LC_ActionSnapMiddleManual::Points {
    Points(const RS_Pen& currentAppPen):
        currentAppPen{currentAppPen}
    {}

    double percentage = g_defaultRatio;

    RS_Vector startPoint{false};
    RS_Vector endPoint{false};

    RS_Pen currentAppPen{};
};

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
                                                      RS_PreviewActionInterface("Snap Middle Manual", container, graphicView, RS2::ActionSnapMiddleManual),
                                                      m_pPoints{std::make_unique<Points>(input_currentAppPen)}
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::LC_ActionSnapMiddleManual");
}


LC_ActionSnapMiddleManual::~LC_ActionSnapMiddleManual() = default;


void LC_ActionSnapMiddleManual::init(int status)
{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::init");

    document->setActivePen(m_pPoints->currentAppPen);

    RS_PreviewActionInterface::init(status);

    m_pPoints->percentage = g_defaultRatio;

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
            mouse = snapToAngle(mouse, m_pPoints->startPoint, 15.0);
        }

        deletePreview();

        RS_Line *line = new RS_Line(preview.get(), m_pPoints->startPoint, mouse);

        preview->addEntity(line);
        line->setLayerToActive();
        line->setPenToActive();

        drawPreview();
    }
    else if (getStatus() == SetPercentage)
    {
        if (predecessor != nullptr)
        {
            if (predecessor->getName().compare("Snap Middle Manual") == 0)
            {
                predecessor->init(-1);
                init(-1);
            }
        }
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
            snapped = snapToAngle(snapped, m_pPoints->startPoint, 15.0);
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
                finish();
                signalUnsetSnapMiddleManual();
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
            m_pPoints->startPoint = mouse;
            setStatus(SetEndPoint);
            graphicView->moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;

        case SetEndPoint:
            /* Refuse zero length lines. */
            if ((mouse - m_pPoints->startPoint).squared() > RS_TOLERANCE2)
            {
                m_pPoints->endPoint = mouse;

                const RS_Vector middleManualPoint = m_pPoints->startPoint + (m_pPoints->endPoint - m_pPoints->startPoint) * m_pPoints->percentage;

                graphicView->moveRelativeZero(middleManualPoint);

                if (predecessor != nullptr)
                {
                    if (predecessor->getName().compare("Default") != 0)
                    {
                        signalUnsetSnapMiddleManual();
                        document->setActivePen(m_pPoints->currentAppPen);
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
                bool ok = false;
                m_pPoints->percentage = RS_Math::eval(inputCommand, &ok)/100.;
                if (ok)
                {
                    setStatus(SetStartPoint);
                    inputCommandEvent->accept();
                    updateMouseButtonHints();
                }
                else
                {
                    m_pPoints->percentage = g_defaultRatio;
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
                setStatus(-1);
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

