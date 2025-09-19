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

#include "lc_documentsstorage.h"

#include <QApplication>

#include "qg_filedialog.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_document.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

LC_DocumentsStorage::LC_DocumentsStorage() = default;

bool LC_DocumentsStorage::saveDocument(RS_Document* document, RS_GraphicView * graphicView,  bool &cancelled) {
    bool result = false;
    cancelled = false;

    if (document != nullptr) {
        document->setGraphicView(graphicView);
        RS_Graphic* graphic = document->getGraphic();
        auto fileName = graphic->getFilename();
        if (fileName.isEmpty()) {
            result = doSaveGraphicAs(graphic, graphicView, cancelled);
        } else {
            QFileInfo info(fileName);
            if (!info.isWritable()) {
                return false;
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            result = doSave(graphic, true);
            QApplication::restoreOverrideCursor();
        }
    }
    return result;
}

bool LC_DocumentsStorage::doSaveGraphicAs(RS_Graphic* graphic, RS_GraphicView *graphicView, bool &cancelled, const QString& currentFileName){
    RS2::FormatType saveFormat = RS2::FormatDXFRW;
    QG_FileDialog dlg(graphicView);
    QString fileName = dlg.getSaveFile(&saveFormat, currentFileName);
    bool result;
    if (fileName.isEmpty()) {
        // cancel is not an error - returns true
        result = true;
        cancelled = true;
    } else {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor));
        graphic->setGraphicView(graphicView);
        result = saveGraphicAs(graphic, fileName, saveFormat, true);
        QApplication::restoreOverrideCursor();
    }
    return result;
}

bool LC_DocumentsStorage::autoSaveDocument(RS_Document* document, RS_GraphicView * graphicView, QString& autosaveFileName){
    bool result = false;
    if (document != nullptr) {
        document->setGraphicView(graphicView);
        result = autoSaveGraphic(document->getGraphic(), autosaveFileName);
    }
    return result;
}

bool LC_DocumentsStorage::saveBlockAs(RS_Graphic *block, const QString &fileName){
    bool result = false;
    if (!fileName.isEmpty()) {
        result = RS_FileIO::instance()->fileExport(*block, fileName, RS2::FormatDXFRW);
    }
    return result;
}

bool LC_DocumentsStorage::saveDocumentAs(const RS_Document* document, RS_GraphicView * graphicView, bool &cancelled) {
    cancelled = false;
    bool result = false;
    if (document != nullptr) {
        RS_Graphic* graphic = document->getGraphic();
        auto fileName = graphic->getFilename();
        result = doSaveGraphicAs(graphic, graphicView, cancelled, fileName);
    }
    return result;
}

bool LC_DocumentsStorage::loadDocument(const RS_Document *document, const QString &fileName) const {
    return loadDocument(document, fileName, RS2::FormatUnknown);
}

bool LC_DocumentsStorage::loadDocument(const RS_Document* document, const QString& fileName, RS2::FormatType type) const {
    bool result = false;
    if (document != nullptr && !fileName.isEmpty()) {
        // cosmetics..
        qApp->processEvents(QEventLoop::AllEvents, 1000);
        result = loadGraphic(document->getGraphic(), fileName, type);
    } else {
        //statusBar()->showMessage(tr("Opening aborted"), 2000);
    }
    return result;
}

bool LC_DocumentsStorage::loadDocumentFromTemplate(const RS_Document* document,RS_GraphicView * graphicView, const QString& fileName, RS2::FormatType type) const {
    bool result = false;

    if (document==nullptr || fileName.isEmpty()) {
        return result;
    }

    result = loadGraphicFromTemplate(document->getGraphic(), fileName, type);

    if (result) {
        graphicView->zoomAuto(false);
    }
    return result;
}

/**
 * Loads the given file into this graphic.
 */
bool LC_DocumentsStorage::loadGraphicFromTemplate(RS_Graphic* graphic, const QString &templateFileName, RS2::FormatType type) const {
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

bool LC_DocumentsStorage::loadGraphic(RS_Graphic* graphic,  const QString &filename, RS2::FormatType type) const {
    graphic->newDoc();

    bool ret = RS_FileIO::instance()->fileImport(*graphic, filename, type);

    if (ret) {
        graphic->onLoadingCompleted();
        QFileInfo finfo(filename);
        auto autosaveFileName = createAutoSaveFileName(finfo);
        graphic->setAutosaveFileName(autosaveFileName);
        graphic->setFilename(filename);
        graphic->markSaved(finfo.lastModified());
    }
    return ret;
}

bool LC_DocumentsStorage::doSave(RS_Graphic* graphic, bool sameFile) {
    bool result = false;
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
        RS_DIALOGFACTORY->commandMessage(
            QObject::tr("File on disk modified. Please save to another file to avoid data loss! File modified: %1").arg(filename));
        return false;
    }

    const QString& actualName = filename;
    if (LC_GET_ONE_BOOL("Defaults","AutoBackupDocument", true)) {
        backupDrawingFile(filename);
    }

    /*	Save drawing file if able to created associated object. */
    if (!actualName.isEmpty()) {
        graphic->prepareForSave(); 
        result = RS_FileIO::instance()->fileExport(*graphic, actualName, actualType);
        QFileInfo actualFileInfo(actualName);
        graphic->markSaved(actualFileInfo.lastModified());
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

bool LC_DocumentsStorage::autoSaveGraphic(RS_Graphic* graphic, QString& fileName) {
    bool ret = false;
    if (graphic->isModified()) {
        RS2::FormatType actualType = graphic->getFormatType();
        if (actualType == RS2::FormatUnknown) {
            actualType = RS2::FormatDXFRW;
        }
        QString autosaveFileName = graphic->getAutoSaveFileName();
        if (!autosaveFileName.isEmpty()) {
            ret = RS_FileIO::instance()->fileExport(*graphic, autosaveFileName, actualType);
            /*
             fixme - sand - don't mark file as non-modified on auto-save.
             *QFileInfo finfo(autosaveFileName);
            graphic->markSaved(finfo.lastModified());
            */
        }
        fileName = autosaveFileName;
    } else {
        // file not modified
        ret = true;
    }
    return ret;
}

bool LC_DocumentsStorage::exportGraphics(RS_Graphic* graphic, const QString& fileName, RS2::FormatType formatType) {
    graphic->setFilename(fileName);
    graphic->setFormatType(formatType);
    if (!fileName.isEmpty()) {
        bool result = RS_FileIO::instance()->fileExport(*graphic, fileName, formatType);
        if (result) {
            QFileInfo finfo(fileName);
            graphic->markSaved(finfo.lastModified());
        }
        return result;
    }
    return false;
}

bool LC_DocumentsStorage::saveGraphicAs(RS_Graphic* graphic, const QString &filename, RS2::FormatType type, bool forceSave) {
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
        filenameIsSame = false;
    }
    ret = doSave(graphic, filenameIsSame);

    if (ret) {
        // Save was successful, remove old autosave file.
        QFile autoSaveFile(autosaveFilenameSaved);
        if (autoSaveFile.exists()) {
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

bool LC_DocumentsStorage::backupDrawingFile(const QString &drawingFileName) {
    QString backupFileSuffix = LC_GET_ONE_STR("Defaults", "BackupFileSuffix", "~");
    return backupDrawingFile(drawingFileName, backupFileSuffix);
}

bool LC_DocumentsStorage::backupDrawingFile(const QString &drawingFileName, const QString& backupSuffix) {
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

QString LC_DocumentsStorage::createAutoSaveFileName(const QFileInfo &fileInfo) const {
    QString autosaveFilePrefix = LC_GET_ONE_STR("Defaults", "AutosaveFilePrefix", "#");
    QString autosaveFileName = createAutoSaveFileName(fileInfo, autosaveFilePrefix);
    return autosaveFileName;
}

QString LC_DocumentsStorage::createAutoSaveFileName(const QFileInfo &fileInfo, const QString &filePrefix) const {
    return createAutoSaveFileName(fileInfo.path(), filePrefix, fileInfo.fileName());
}

QString LC_DocumentsStorage::createAutoSaveFileName(const QString& path, const QString &filePrefix,  const QString& fileName) const {
    // Construct new autosave filename by prepending # to the filename
    // part, using the same directory as the destination file.
    QString result = path + "/" + filePrefix + fileName;
    return result;
}
