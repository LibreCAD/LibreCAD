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

#ifndef LC_DOCUMENTSSTORAGE_H
#define LC_DOCUMENTSSTORAGE_H

#include <QObject>
#include "rs.h"

class RS_Graphic;
class RS_GraphicView;
class RS_Document;
class QFileInfo;

class LC_DocumentsStorage: public QObject{
    Q_OBJECT
public:
    LC_DocumentsStorage();
    bool saveDocument(RS_Document *document,RS_GraphicView * graphicView, bool &cancelled);
    bool saveBlockAs(RS_Graphic* block, const QString& fileName);
    bool autoSaveDocument(RS_Document *document,RS_GraphicView * graphicView, QString& autosaveFileName);
    bool saveDocumentAs(const RS_Document *document,RS_GraphicView * graphicView, bool &cancelled);
    bool exportGraphics(RS_Graphic *document,const QString &fileName, RS2::FormatType formatType);
    bool loadDocument(const RS_Document *document, const QString &fileName, RS2::FormatType type) const;
    bool loadDocument(const RS_Document *document, const QString &fileName) const;
    bool loadDocumentFromTemplate(const RS_Document *document, RS_GraphicView *graphicView, const QString &fileName, RS2::FormatType type) const;
protected:
    bool doSaveGraphicAs(RS_Graphic* graphic, RS_GraphicView *graphicView, bool &cancelled, const QString& currentFileName = "");
    bool autoSaveGraphic(RS_Graphic *graphic, QString& fileName);
    bool loadGraphicFromTemplate(RS_Graphic *graphic, const QString &templateFileName, RS2::FormatType type) const;
    bool loadGraphic(RS_Graphic *graphic, const QString &filename, RS2::FormatType type) const;
    bool doSave(RS_Graphic *graphic, bool sameFile);
    bool saveGraphicAs(RS_Graphic *graphic, const QString &filename, RS2::FormatType type, bool forceSave);
    bool backupDrawingFile(const QString &drawingFileName);
    bool backupDrawingFile(const QString &drawingFileName, const QString &backupSuffix);
    QString createAutoSaveFileName(const QFileInfo &fileInfo) const;
    QString createAutoSaveFileName(const QFileInfo &fileInfo, const QString &filePrefix) const;
    QString createAutoSaveFileName(const QString &path, const QString &filePrefix, const QString &fileName) const;
};

#endif // LC_DOCUMENTSSTORAGE_H
