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


#include "rs_actiondimaligned.h"
#include "rs_commandevent.h"
#include "rs_constructionline.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimaligned.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "lc_actiondimlinearbase.h"

RS_ActionDimAligned::RS_ActionDimAligned(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDimLinearBase("Draw aligned dimensions",
                        container, graphicView){
    actionType = RS2::ActionDimAligned;
    reset();
}

RS_ActionDimAligned::~RS_ActionDimAligned() = default;

#define DEBUG_DIM_SNAP_NO

void RS_ActionDimAligned::reset(){
    RS_ActionDimension::reset();
    edata.reset(new RS_DimAlignedData(RS_Vector(false),
                                      RS_Vector(false)));
    lastStatus = SetExtPoint1;
    updateOptions();
}


void RS_ActionDimAligned::preparePreview(){
    RS_Vector dirV = RS_Vector::polar(100.,edata->extensionPoint1.angleTo(edata->extensionPoint2)+ M_PI_2);
    RS_ConstructionLine cl(nullptr,RS_ConstructionLineData(edata->extensionPoint2,edata->extensionPoint2 + dirV));

    data->definitionPoint =cl.getNearestPointOnEntity(data->definitionPoint);
}

RS_Entity *RS_ActionDimAligned::createDim(RS_EntityContainer* parent){
    auto *dim = new RS_DimAligned(parent, *data, *edata);
    return dim;
}

bool RS_ActionDimAligned::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetText: {
            accept = true;
            setText(c);
            updateOptions();
            setStatus(lastStatus);
            graphicView->enableCoordinateInput();
            break;
        }
        default:
            if (checkCommand("text", c)){
                accept = true;
                lastStatus = (Status) getStatus();
                graphicView->disableCoordinateInput();
                setStatus(SetText);
            }
            break;
    }
    return accept;
}

QStringList RS_ActionDimAligned::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetExtPoint1:
        case SetExtPoint2:
        case SetDefPoint:
            cmd += command("text");
            break;

        default:
            break;
    }
    return cmd;
}

RS_Vector RS_ActionDimAligned::getExtensionPoint1(){
    return edata->extensionPoint1;
}

RS_Vector RS_ActionDimAligned::getExtensionPoint2(){
    return edata->extensionPoint2;
}

double RS_ActionDimAligned::getDimAngle(){
    return edata->extensionPoint1.angleTo(edata->extensionPoint2);
}


void RS_ActionDimAligned::setExtensionPoint1(RS_Vector p){
    edata->extensionPoint1 = p;
}

void RS_ActionDimAligned::setExtensionPoint2(RS_Vector p){
    edata->extensionPoint2 = p;
}
