/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
**********************************************************************/

#include "rs_actionfileexportmakercam.h"

#include <iostream>
#include <fstream>

#include <QAction>

#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_makercamsvg.h"
#include "rs_settings.h"
#include "rs_xmlwriterqxmlstreamwriter.h"

RS_ActionFileExportMakerCam::RS_ActionFileExportMakerCam(RS_EntityContainer& container,
                                                         RS_GraphicView& graphicView)
    : RS_ActionInterface("Export as MakerCAM SVG...", container, graphicView) {}

QAction* RS_ActionFileExportMakerCam::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {

    QAction* action = new QAction(tr("Export as &MakerCAM SVG..."), NULL);
//	action->setIcon(QIcon(":/ui/blockadd.png"));
    return action;
}

void RS_ActionFileExportMakerCam::init(int status) {

    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionFileExportMakerCam::trigger() {

    RS_DEBUG->print("RS_ActionFileExportMakerCam::trigger()");

    if (graphic != NULL) {

        bool accepted = RS_DIALOGFACTORY->requestOptionsMakerCamDialog();

        if (accepted) {

            QString filename = RS_DIALOGFACTORY->requestFileSaveAsDialog(tr("Export as"),
                                                                         "",
                                                                         "Scalable Vector Graphics (*.svg)");

            if (filename != "") {

                RS_SETTINGS->beginGroup("/ExportMakerCam");

                RS_MakerCamSVG* generator = new RS_MakerCamSVG(new RS_XMLWriterQXmlStreamWriter(),
                                                               (bool)RS_SETTINGS->readNumEntry("/ExportInvisibleLayers"),
                                                               (bool)RS_SETTINGS->readNumEntry("/ExportConstructionLayers"),
                                                               (bool)RS_SETTINGS->readNumEntry("/WriteBlocksInline"),
                                                               (bool)RS_SETTINGS->readNumEntry("/ConvertEllipsesToBeziers"));

                RS_SETTINGS->endGroup();

                if (generator->generate(graphic)) {

                    std::ofstream file;
                    file.open(filename.toStdString());
                    file << generator->resultAsString();
                    file.close();
                }

                delete generator;
                generator = NULL;
            }
        }
    }

    finish(false);
}
