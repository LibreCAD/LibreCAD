/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_snapper.h"
#include "rs_constructionline.h"
#include "rs_dialogfactory.h"



RS_ActionDimension::RS_ActionDimension(const char* name,
                                       RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface(name,
                           container, graphicView) {

    reset();
}



RS_ActionDimension::~RS_ActionDimension() {}



void RS_ActionDimension::reset() {
    data = RS_DimensionData(RS_Vector(false),
                            RS_Vector(false),
                            RS2::VAlignMiddle,
                            RS2::HAlignCenter,
                            RS2::Exact,
                            1.0,
                            "",
                            "Standard",
                            0.0);
	diameter = false;
}



void RS_ActionDimension::init(int status) {
    RS_PreviewActionInterface::init(status);
    //reset();
}



void RS_ActionDimension::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



void RS_ActionDimension::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true, true);
    }
}



void RS_ActionDimension::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDimension::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarDim);
        }
    }
}

// EOF
