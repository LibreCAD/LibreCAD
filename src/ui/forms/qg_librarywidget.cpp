/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#include "qg_librarywidget.h"

#include "rs_system.h"
#include "rs_painterqt.h"
#include "rs_staticgraphicview.h"
#include "rs_actionlibraryinsert.h"
#include <QDateTime>
#include <QImageWriter>
#include <QDesktopServices>
#include "qg_actionhandler.h"
#include "qg_listviewitem.h"

/*
 *  Constructs a QG_LibraryWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LibraryWidget::QG_LibraryWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LibraryWidget::~QG_LibraryWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LibraryWidget::languageChange()
{
    retranslateUi(this);
}

void QG_LibraryWidget::init() {
    actionHandler = NULL;

    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    for (QStringList::Iterator it = directoryList.begin(); it!=directoryList.end(); ++it) {
        appendTree(NULL, (*it));
    }
}


void QG_LibraryWidget::setActionHandler(QG_ActionHandler* ah) {
    actionHandler = ah;
}

/**
 * Escape releases focus.
 */
void QG_LibraryWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {

    case Qt::Key_Escape:
        emit escape();
        break;

    default:
        QWidget::keyPressEvent(e);
        break;
    }
}


/**
 * Insert.
 */
void QG_LibraryWidget::insert() {
    Q3IconViewItem* item = ivPreview->currentItem();
    QString dxfPath = getItemPath(item);

    if (QFileInfo(dxfPath).isReadable()) {
        if (actionHandler!=NULL) {
            RS_ActionInterface* a =
                actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a!=NULL) {
                RS_ActionLibraryInsert* action = (RS_ActionLibraryInsert*)a;
                action->setFile(dxfPath);
            } else {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "QG_LibraryWidget::insert:"
                                "Cannot create action RS_ActionLibraryInsert");
            }
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LibraryWidget::insert: Can't read file: '%s'", dxfPath.latin1());
    }
}



/**
 * Appends the given directory to the given list view item. Called recursively until all
 * library directories are appended.
 */
void QG_LibraryWidget::appendTree(QG_ListViewItem* item, QString directory) {
    QStringList::Iterator it;
    QDir dir(directory);

    // read subdirectories of this directory:
    if (dir.exists()) {
        QStringList lDirectoryList = dir.entryList(QDir::Dirs, QDir::Name);

        QG_ListViewItem* newItem;
        QG_ListViewItem* searchItem;
        for( it=lDirectoryList.begin(); it!=lDirectoryList.end(); ++it ) {
            if( (*it)!="." && (*it)!="..") {

                newItem=NULL;

                // Look for an item already existing and take this
                //   instead of making a new one:
                if (item!=NULL) {
                    searchItem = (QG_ListViewItem*)item->firstChild();
                } else {
                    searchItem = (QG_ListViewItem*)lvDirectory->firstChild();
                }

                while (searchItem!=NULL) {
                    if (searchItem->text(0)==(*it)) {
                        newItem=searchItem;
                        break;
                    }
                    searchItem = (QG_ListViewItem*)searchItem->nextSibling();
                }

                // Create new item if no existing was found:
                if (newItem==NULL) {
                    if (item) {
                        newItem = new QG_ListViewItem(item, (*it));
                    } else {
                        newItem = new QG_ListViewItem(lvDirectory, (*it));
                    }
                }

                appendTree(newItem, directory+QDir::separator()+(*it));
            }
        }
    }
}


/**
 * Updates the icon preview.
 */
void QG_LibraryWidget::updatePreview(Q3ListViewItem* item) {
    if (item==NULL) {
        return;
    }
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // dir from the point of view of the library browser (e.g. /mechanical/screws)
    QString directory = getItemDir(item);
    ivPreview->clear();

    // List of all directories that contain part libraries:
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    QStringList::Iterator it;
    QDir itemDir;
    QStringList itemPathList;

    // look in all possible system directories for DXF files in the current library path:
    for (it=directoryList.begin(); it!=directoryList.end(); ++it) {
        itemDir.setPath((*it)+directory);

        if (itemDir.exists()) {
            QStringList itemNameList =
                itemDir.entryList("*.dxf", QDir::Files, QDir::Name);
            QStringList::Iterator it2;
            for (it2=itemNameList.begin(); it2!=itemNameList.end(); ++it2) {
                itemPathList += itemDir.path()+QDir::separator()+(*it2);
            }
        }
    }

    // Sort entries:
    itemPathList.sort();

    // Fill items into icon view:
    Q3IconViewItem* newItem;
    for (it=itemPathList.begin(); it!=itemPathList.end(); ++it) {
        QString label = QFileInfo(*it).baseName();
        QPixmap pixmap = getPixmap(directory, QFileInfo(*it).fileName(), (*it));
        newItem = new Q3IconViewItem(ivPreview, label, pixmap);
    }
    QApplication::restoreOverrideCursor();
}


/**
 * @return Directory (in terms of the List view) to the given item (e.g. /mechanical/screws)
 */
QString QG_LibraryWidget::getItemDir(Q3ListViewItem* item) {
    QString ret = "";

    if (item==NULL) {
        return ret;
    }

    Q3ListViewItem* parent = item->parent();
    return getItemDir(parent) + QDir::separator() + QString("%1").arg(item->text(0));
}



/**
 * @return Path of the DXF file that is represented by the given item.
 */
QString QG_LibraryWidget::getItemPath(Q3IconViewItem* item) {
    QString dir = getItemDir(lvDirectory->currentItem());
    if (item!=NULL) {
        // List of all directories that contain part libraries:
        QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
        QStringList::Iterator it;
        QDir itemDir;

        // look in all possible system directories for DXF files in the current library path:
        for (it=directoryList.begin(); it!=directoryList.end(); ++it) {
            itemDir.setPath((*it)+dir);
            if (itemDir.exists()) {
                QString f = (*it) + dir + QDir::separator() + item->text() + ".dxf";
                if (QFileInfo(f).isReadable()) {
                    return f;
                }
            }
        }

        return "";
    } else {
        return "";
    }
}



/**
 * @return Pixmap that serves as icon for the given DXF File.
 * The existing PNG file is returned or created and returned..
 *
 * @param dir Library directory (e.g. "/mechanical/screws")
 * @param dxfFile File name (e.g. "screw1.dxf")
 * @param dxfPath Full path to the existing DXF file on disk 
 *                          (e.g. /home/tux/.qcad/library/mechanical/screws/screw1.dxf)
 */
QPixmap QG_LibraryWidget::getPixmap(const QString& dir, const QString& dxfFile,
                                    const QString& dxfPath) {
    QString pngFile = getPathToPixmap(dir, dxfFile, dxfPath);
    QFileInfo fiPng(pngFile);

    // found existing thumbnail:
    if (fiPng.isFile()) {
        return QPixmap(pngFile);
    }
    // default thumbnail:
    else {
        return QPixmap(64,64);
    }
}



/**
 * @return Path to the thumbnail of the given DXF file. If no thumbnail exists, one is
 * created in the user's home. If no thumbnail can be created, an empty string is returned.
 */
QString QG_LibraryWidget::getPathToPixmap(const QString& dir,
        const QString& dxfFile,
        const QString& dxfPath) {

    // the thumbnail must be created in the user's home.
    QString iconCacheLocation=QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QDir::separator() + "iconCache" + QDir::separator();

    RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: "
                    "dir: '%s' dxfFile: '%s' dxfPath: '%s'",
                    dir.latin1(), dxfFile.latin1(), dxfPath.latin1());

    // List of all directories that contain part libraries:
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    directoryList.prepend(iconCacheLocation);
    QStringList::Iterator it;

    QFileInfo fiDxf(dxfPath);
    QString itemDir;
    QString pngPath;

    // look in all possible system directories for PNG files
    //  in the current library path:
    for (it=directoryList.begin(); it!=directoryList.end(); ++it) {
        itemDir = (*it)+dir;
        pngPath = itemDir + QDir::separator() + fiDxf.baseName() + ".png";
        RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: checking: '%s'",
                        pngPath.latin1());
        QFileInfo fiPng(pngPath);

        // the thumbnail exists:
        if (fiPng.isFile()) {
            RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: dxf date: %s, png date: %s",
                            fiDxf.lastModified().toString().latin1(), fiPng.lastModified().toString().latin1());
            if (fiPng.lastModified() > fiDxf.lastModified()) {
                RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: thumbnail found: '%s'",
                                pngPath.latin1());
                return pngPath;
            } else {
                RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: thumbnail needs to be updated: '%s'",
                                pngPath.latin1());
            }
        }
    }

    // create all directories needed:
    RS_SYSTEM->createPaths(iconCacheLocation + dir);

    QString foo=iconCacheLocation + dir + QDir::separator() + fiDxf.baseName() + ".png";
    pngPath = iconCacheLocation + dir + QDir::separator() + fiDxf.baseName() + ".png";

    QPixmap* buffer = new QPixmap(128,128);
    RS_PainterQt painter(buffer);
    painter.setBackgroundColor(RS_Color(255,255,255));
    painter.eraseRect(0,0, 128,128);

    RS_StaticGraphicView gv(128,128, &painter);
    RS_Graphic graphic;
    if (graphic.open(dxfPath, RS2::FormatUnknown)) {
        for (RS_Entity* e=graphic.firstEntity(RS2::ResolveAll);
                e!=NULL; e=graphic.nextEntity(RS2::ResolveAll)) {
            RS_Pen pen = e->getPen();
            pen.setColor(Qt::black);
            e->setPen(pen);
        }

        gv.setContainer(&graphic);
        gv.zoomAuto(false);
        // gv.drawEntity(&graphic, true);

        for (RS_Entity* e=graphic.firstEntity(RS2::ResolveAll);
                e!=NULL; e=graphic.nextEntity(RS2::ResolveAll)) {
            gv.drawEntity(&painter, e);
        }

        QImageWriter iio;
        QImage img;
        img = *buffer;
        img = img.smoothScale(64,64);
        // iio.setImage(img);
        iio.setFileName(pngPath);
        iio.setFormat("PNG");
        if (!iio.write(img)) {
            pngPath = "";
            RS_DEBUG->print(RS_Debug::D_ERROR,
                            "QG_LibraryWidget::getPathToPixmap: Cannot write thumbnail: '%s'",
                            pngPath.latin1());
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LibraryWidget::getPathToPixmap: Cannot open file: '%s'",
                        dxfPath.latin1());
    }

    // GraphicView deletes painter
    painter.end();
    delete buffer;

    return pngPath;
}
