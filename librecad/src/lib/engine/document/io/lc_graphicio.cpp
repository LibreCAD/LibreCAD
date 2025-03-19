/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <QString>
#include <QDir>
#include "lc_graphicio.h"
#include "rs.h"
#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_settings.h"
#include "lc_graphicviewport.h"

LC_GraphicIO::LC_GraphicIO() {}

bool LC_GraphicIO::open(RS_Graphic* graphic,  const QString &filename, RS2::FormatType type) {
    RS_DEBUG->print("RS_Graphic::open(%s)", filename.toLatin1().data());

    bool ret;
    graphic->setFilename(filename);
    QFileInfo finfo(filename);
    QString autosaveFileName = createAutoSaveFileName(finfo);

    // clean all:
    graphic->newDoc();

    // import file:
    ret = RS_FileIO::instance()->fileImport(*graphic, filename, type);

    if (ret) {
        RS_GraphicView *gv = graphic->getGraphicView(); // fixme - eliminate this dependency!
        if (gv != nullptr) {
            // fixme - sand - review and probably move initialization of UCS - as normal support of VIEWPORT will be available
            // todo - not sure whether this is right place for setting up current wcs.
            // Actually, it seems that it's better to rely on reading viewport (were setting for the offset and zoom are set.
            // however, must probably with proper support of VIEW, they will be reworked too..
            // So let it have here for now so far
            LC_GraphicViewport* viewport = gv->getViewPort();
            viewport->initAfterDocumentOpen();
        }

        graphic->markSaved(finfo.lastModified());
    }
    return ret;
}

bool LC_GraphicIO::doSave(RS_Graphic* graphic, bool sameFile) {
    bool result = false;
    QString actualName;
    RS2::FormatType actualType = graphic->getFormatType();

//	- This is not an AutoSave operation.  This is a manual
//	  save operation.  So, ...
//		- Set working file name to the drawing file name.
//		- Backup drawing file (if necessary).
//	------------------------------------------------------
    QString filename = graphic->getFilename();

    QFileInfo finfo(filename);
    QDateTime fileLastModifyTime = finfo.lastModified();
    QDateTime graphicLastSaveTime = graphic->getLastSaveTime();
    //bug#3414993

    //modifiedTime should only be used for the same filename
    if (sameFile && graphicLastSaveTime.isValid() && fileLastModifyTime != graphicLastSaveTime) {
        //file modified by others
        RS_DIALOGFACTORY->commandMessage(QObject::tr("File on disk modified. Please save to another file to avoid data loss! File modified: %1").arg(filename));
        return false;
    }

    actualName = filename;
    if (LC_GET_ONE_BOOL("Defaults","AutoBackupDocument", true)) {
        backupDrawingFile(filename);
    }

    /*	Save drawing file if able to created associated object. */
    if (!actualName.isEmpty()) {
        result = RS_FileIO::instance()->fileExport(*graphic, actualName, actualType);
        QFileInfo finfo(actualName);
        graphic->markSaved(finfo.lastModified());
//        currentFileName = actualName;
    }

    /*	Remove AutoSave file after user has successfully saved file.*/
    if (result) {
        /*	Autosave file object	*/
        QString autosaveFilename = graphic->getAutoSaveFileName();
        QFile autosaveFile(autosaveFilename);

        if (autosaveFile.exists()) {
            autosaveFile.remove();
        }
    }
    return result;
}

bool LC_GraphicIO::save(RS_Graphic* graphic) {
    bool ret = false;
    if (graphic->isModified()) {
        doSave(graphic, true);
    } else {
        // file not modified
        ret = true;
    }
    return ret;
}

bool LC_GraphicIO::autoSave(RS_Graphic* graphic) {
    bool ret = false;
    if (graphic->isModified()) {
        RS2::FormatType actualType = graphic->getFormatType();
        if (actualType == RS2::FormatUnknown) {
            actualType = RS2::FormatDXFRW;
        }
        QString autosaveFileName = graphic->getAutoSaveFileName();
        if (!autosaveFileName.isEmpty()) {
            ret = RS_FileIO::instance()->fileExport(*graphic, autosaveFileName, actualType);
            QFileInfo finfo(autosaveFileName);
            graphic->markSaved(finfo.lastModified());
        }
    } else {
        // file not modified
        ret = true;
    }
    return ret;
}
/**
 * Loads the given file into this graphic.
 */
bool LC_GraphicIO::loadTemplate(RS_Graphic* graphic, const QString &templateFileName, RS2::FormatType type) {
    QString autosaveFilePrefix = LC_GET_ONE_STR("Path", "AutosaveFilePrefix", "#");
    QString autosaveFilename = createAutoSaveFileName(QDir::tempPath (), autosaveFilePrefix, tr("Unnamed")+".dxf");

    // clean all:
    graphic->newDoc();

    // import template file:
    bool ret = RS_FileIO::instance()->fileImport(*graphic, templateFileName, type);

    QFileInfo finfo;
    graphic->markSaved(finfo.lastModified());
    graphic->setAutosaveFileName(autosaveFilename);
    return ret;
}

bool LC_GraphicIO::saveAs(RS_Graphic* graphic, const QString &filename, RS2::FormatType type, bool forceSave) {
    bool ret = false;

    // Check/memorize if file name we want to use as new file
   // name is the same as the actual file name.

    auto const filenameSaved = graphic->getFilename();
    auto const autosaveFilenameSaved = graphic->getAutoSaveFileName();
    bool filenameIsSame = filename == filenameSaved;
    auto const formatTypeSaved = graphic->getFormatType();

    graphic->setFilename(filename);
    graphic->setFormatType(type);

    QFileInfo finfo(filename);

    // Construct new autosave filename by prepending # to the filename
    // part, using the same directory as the destination file.
    QString autosaveFileName = createAutoSaveFileName(finfo);
    graphic->setAutosaveFileName(autosaveFileName);

    // When drawing is saved using a different name than the actual
    // drawing file name, make LibreCAD think that drawing file
    // has been modified, to make sure the drawing file saved.
    if (!filenameIsSame || forceSave) {
        graphic->setModified(true);
    }
    ret = doSave(graphic, filenameIsSame);

    if (ret) {
        // Save was successful, remove old autosave file.
        QFile autoSaveFile(autosaveFilenameSaved);

        if (autoSaveFile.exists()) {
            RS_DEBUG->print("RS_Graphic::saveAs: Removing old autosave file %s",
                            autosaveFilenameSaved.toLatin1().data());
            autoSaveFile.remove();
        }
    } else {
        //do not modify filenames:
        graphic->setFilename(filenameSaved);
        graphic->setAutosaveFileName(autosaveFilenameSaved);
        graphic->setFormatType(formatTypeSaved);
    }
    return ret;
}

bool LC_GraphicIO::backupDrawingFile(const QString &drawingFileName) {
    QString backupFileSuffix = LC_GET_ONE_STR("Path", "BackupFileSuffix", "~");
    return backupDrawingFile(drawingFileName, backupFileSuffix);
}

bool LC_GraphicIO::backupDrawingFile(const QString &drawingFileName, const QString& backupSuffix) {
    bool ret = false;
    if (drawingFileName.length() > 0) {
        auto backupFileName = QString(drawingFileName + backupSuffix);
        QFile drawingFile = QFile(drawingFileName);
        if (drawingFile.exists()) {
            QFile backupFile = QFile(backupFileName);
            if (backupFile.exists()) {
                backupFile.remove();
            }
            ret = drawingFile.copy(backupFileName);
        }
    }
    return ret;
}

QString LC_GraphicIO::createAutoSaveFileName(const QFileInfo &fileInfo) const {
    QString autosaveFilePrefix = LC_GET_ONE_STR("Path", "AutosaveFilePrefix", "#");
    QString autosaveFileName = createAutoSaveFileName(fileInfo, autosaveFilePrefix);
    return autosaveFileName;
}

QString LC_GraphicIO::createAutoSaveFileName(const QFileInfo &fileInfo, const QString &filePrefix) const {
    return createAutoSaveFileName(fileInfo.path(), filePrefix, fileInfo.fileName());
}

QString LC_GraphicIO::createAutoSaveFileName(const QString& path, const QString &filePrefix,  const QString& fileName) const {
    // Construct new autosave filename by prepending # to the filename
    // part, using the same directory as the destination file.
    QString result = path + "/" + filePrefix + fileName;
    return result;
}
