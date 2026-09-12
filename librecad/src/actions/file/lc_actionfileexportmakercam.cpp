/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
** Copyright (C) 2018 Andrey Yaromenok (ayaromenok@gmail.com)
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

#include "lc_actionfileexportmakercam.h"

#include <QSaveFile>
#include <QTextStream>

#include "lc_makercamsvg.h"
#include "lc_xmlwriterqxmlstreamwriter.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_settings.h"

class LC_MakerCamSVG;

namespace {
    bool getSetting(const QString& entry) {
        return LC_GET_INT("" + entry, 0);
    }

    // create an SVG generator
    std::unique_ptr<LC_MakerCamSVG> getGenerator() {
        LC_GROUP_GUARD("ExportMakerCam");
        {
            auto generator = std::make_unique<LC_MakerCamSVG>(std::make_unique<LC_XMLWriterQXmlStreamWriter>(),
                                                              LC_GET_BOOL("ExportInvisibleLayers"), LC_GET_BOOL("ExportConstructionLayers"),
                                                              LC_GET_BOOL("WriteBlocksInline"), LC_GET_BOOL("ConvertEllipsesToBeziers"),
                                                              LC_GET_BOOL("ExportImages"), LC_GET_BOOL("BakeDashDotLines"),
                                                              LC_GET_STR("DefaultElementWidth", "1.0").toDouble(),
                                                              LC_GET_STR("DefaultDashLinePatternLength").toDouble());
            const bool exportPoints = getSetting("ExportPoints");
            generator->setExportPoints(exportPoints);
            return generator;
        }
    }
}

LC_ActionFileExportMakerCam::LC_ActionFileExportMakerCam(LC_ActionContext* actionContext)
    : RS_ActionInterface("Export as CAM/plain SVG...", actionContext, RS2::ActionFileExportMakerCam) {
}

void LC_ActionFileExportMakerCam::init(const int status) {
    RS_ActionInterface::init(status);
    trigger();
}

bool LC_ActionFileExportMakerCam::writeSvg(const QString& fileName, RS_Graphic& graphic) {
    if (fileName.isEmpty()) {
        LC_ERR << __func__ << "(): empty file name, no SVG is generated";
        return false;
    }

    const auto generator = getGenerator();
    if (!generator->generate(&graphic)) {
        LC_ERR << __func__ << "(): failed in generating the SVG for " << fileName;
        return false;
    }

    // QSaveFile reports a write that fails after the file is open, and leaves an
    // existing file alone until the new one is complete.
    QSaveFile file{fileName};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LC_ERR << __func__ << "(): failed in creating file " << fileName << ", no SVG is generated";
        return false;
    }

    QTextStream out(&file);
    out << QString::fromStdString(generator->resultAsString());
    out.flush();
    if (out.status() != QTextStream::Ok || !file.commit()) {
        LC_ERR << __func__ << "(): failed in writing file " << fileName;
        return false;
    }

    return true;
}

void LC_ActionFileExportMakerCam::trigger() {
    RS_DEBUG->print("LC_ActionFileExportMakerCam::trigger()");

    if (m_graphic != nullptr) {
        const bool accepted = RS_DIALOGFACTORY->requestOptionsMakerCamDialog();

        if (accepted) {
            const QString filename = RS_DIALOGFACTORY->requestFileSaveAsDialog(tr("Export as"), "", "Scalable Vector Graphics (*.svg)");
            // An empty name means the dialog was cancelled
            if (!filename.isEmpty() && !writeSvg(filename, *m_graphic)) {
                RS_DIALOGFACTORY->requestWarningDialog(
                    tr("Cannot write the file\n%1\nPlease check the filename and permissions.").arg(filename));
            }
        }
    }

    finish();
}
