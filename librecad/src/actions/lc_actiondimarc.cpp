/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
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


#include <iostream>

#include <QAction>
#include <QMouseEvent>

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"

#include "lc_actiondimarc.h"


LC_ActionDimArc::LC_ActionDimArc(RS_EntityContainer& container, RS_GraphicView& graphicView) 
                                 :
                                 RS_ActionDimension("Draw Arc Dimensions", container, graphicView)
{
    reset();
}


LC_ActionDimArc::~LC_ActionDimArc() = default;


void LC_ActionDimArc::reset()
{
    RS_ActionDimension::reset();

    actionType = RS2::ActionDimArc;

    dimArcData.radius    = 0.0;
    dimArcData.arcLength = 0.0;

    dimArcData.centre     = RS_Vector(false);
    dimArcData.endAngle   = RS_Vector(false);
    dimArcData.startAngle = RS_Vector(false);

    selectedArcEntity = nullptr;

    RS_DIALOGFACTORY->requestOptions (this, true, true);
}


void LC_ActionDimArc::trigger()
{
    RS_PreviewActionInterface::trigger();

    if (selectedArcEntity == nullptr)
    {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: selectedArcEntity is nullptr.\n");
        return;
    }

    if ( ! dimArcData.centre.valid)
    {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: dimArcData.centre is not valid.\n");
        return;
    }

    LC_DimArc* new_dimArc_entity { new LC_DimArc (container, *data, dimArcData) };

    new_dimArc_entity->setLayerToActive();
    new_dimArc_entity->setPenToActive();
    new_dimArc_entity->update();
    container->addEntity(new_dimArc_entity);

    if (document)
    {
        document->startUndoCycle();
        document->addUndoable(new_dimArc_entity);
        document->endUndoCycle();
    }

    RS_Vector relativeZeroPos { graphicView->getRelativeZero() };

    setStatus (SetEntity);

    graphicView->redraw (RS2::RedrawDrawing);
    graphicView->moveRelativeZero (relativeZeroPos);

    RS_Snapper::finish();
}


void LC_ActionDimArc::mouseMoveEvent(QMouseEvent* e)
{
    RS_DEBUG->print( "LC_ActionDimArc::mouseMoveEvent begin");

    switch (getStatus())
    {
        case SetPos:
        {
            setRadius (snapPoint(e));

            LC_DimArc *temp_dimArc_entity { new LC_DimArc (preview.get(), *data, dimArcData) };

            deletePreview();
            preview->addEntity(temp_dimArc_entity);

            drawPreview();
        }
        break;

        default:
            break;
    }

    RS_DEBUG->print("LC_ActionDimArc::mouseMoveEvent end");
}


void LC_ActionDimArc::mouseReleaseEvent(QMouseEvent* e)
{
    if (Qt::LeftButton == e->button())
    {
        switch (getStatus())
        {
            case SetEntity:
            {
                selectedArcEntity = catchEntity (e, RS2::ResolveAll);

                if (selectedArcEntity != nullptr)
                {
                    if (selectedArcEntity->rtti() == RS2::EntityArc)
                    {
                        dimArcData.centre     = selectedArcEntity->getCenter();
                        dimArcData.arcLength  = selectedArcEntity->getLength();

                        dimArcData.startAngle = RS_Vector(((RS_Arc *) selectedArcEntity)->getAngle1());
                        dimArcData.endAngle   = RS_Vector(((RS_Arc *) selectedArcEntity)->getAngle2());

                        data->definitionPoint = selectedArcEntity->getStartpoint();

                        if (((RS_Arc *) selectedArcEntity)->isReversed())
                        {
                            const RS_Vector tempAngle = RS_Vector(dimArcData.startAngle);

                            dimArcData.startAngle = dimArcData.endAngle;
                            dimArcData.endAngle   = tempAngle;

                            data->definitionPoint = selectedArcEntity->getEndpoint();
                        }

                        setStatus (SetPos);
                    }
                    else
                    {
                        RS_DEBUG->print( RS_Debug::D_ERROR, 
                                         "LC_ActionDimArc::mouseReleaseEvent: selectedArcEntity is not an arc.");

                        selectedArcEntity = nullptr;
                    }
                }
            }
            break;

            case SetPos:
            {
                RS_CoordinateEvent ce (snapPoint (e));
                coordinateEvent (&ce);
            }
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init (getStatus() - 1);
    }
}


void LC_ActionDimArc::showOptions()
{
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions (this, true);
}


void LC_ActionDimArc::hideOptions()
{
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions (this, false);
}


void LC_ActionDimArc::coordinateEvent(RS_CoordinateEvent* e)
{
    if (e == nullptr) return;

    switch (getStatus())
    {
        case SetPos:
            setRadius (e->getCoordinate());
            trigger();
            reset();
            setStatus (SetEntity);
            break;

        default:
            break;
    }
}


void LC_ActionDimArc::commandEvent(RS_CommandEvent* e)
{
    QString inputCommand (e->getCommand().toLower());

    if (checkCommand (QStringLiteral ("help"), inputCommand))
    {
        RS_DIALOGFACTORY->commandMessage(getAvailableCommands().join(", "));
        return;
    }

    if (checkCommand (QStringLiteral ("exit"), inputCommand))
    {
        init (-1);
        return;
    }
}


QStringList LC_ActionDimArc::getAvailableCommands()
{
    QStringList availableCommandsList { "help", "exit" };

    return availableCommandsList;
}


void LC_ActionDimArc::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetEntity:
            RS_DIALOGFACTORY->updateMouseWidget( tr("Select arc entity"),
                                                 tr("Cancel"));
            break;

        case SetPos:
            RS_DIALOGFACTORY->updateMouseWidget( tr("Specify dimension arc location"),
                                                 tr("Cancel"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}


void LC_ActionDimArc::setRadius(const RS_Vector& selectedPosition)
{
    const double minimum_dimArc_gap = 0.0;

    dimArcData.radius = selectedPosition.distanceTo (dimArcData.centre);

    const double minimumRadius = selectedArcEntity->getRadius() + minimum_dimArc_gap;

    if (dimArcData.radius < minimumRadius) dimArcData.radius = minimumRadius;
}

