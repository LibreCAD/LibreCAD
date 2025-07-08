/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
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


#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QKeyEvent>
#include <QListView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QAbstractItemView>

#include "qg_librarywidget.h"
#include "lc_containertraverser.h"
#include "lc_documentsstorage.h"
#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"
#include "qg_actionhandler.h"
#include "rs_actioninterface.h"
#include "rs_actionlibraryinsert.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_painter.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {
    // fixme - sand - rework, use reusable class
    void writePng(const QString& pngPath, QPixmap pixmap)
    {
        QImageWriter iio;
        QImage img = pixmap.toImage();
        img = img.scaled(64,64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        // iio.setImage(img);
        iio.setFileName(pngPath);
        iio.setFormat("PNG");
        if (!iio.write(img)) {
            RS_DEBUG->print(RS_Debug::D_ERROR,
                            "QG_LibraryWidget::getPathToPixmap: Cannot write thumbnail: '%s'",
                            pngPath.toLatin1().data());
        }
    }
}

/*
 *  Constructs a QG_LibraryWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 * @author Rallaz
 */
QG_LibraryWidget::QG_LibraryWidget(QG_ActionHandler *action_handler, QWidget* parent, const char* name, Qt::WindowFlags fl)
    : LC_GraphicViewAwareWidget(parent, name, fl), actionHandler{action_handler}{
    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->setSpacing(2);
    vboxLayout->setContentsMargins(2, 2, 2, 2);
    dirView = new QTreeView(this);
    dirView->setRootIsDecorated(true);
    dirView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vboxLayout->addWidget(dirView);
    ivPreview = new QListView(this);
    ivPreview->setViewMode(QListView::IconMode);
    vboxLayout->addWidget(ivPreview);
    bInsert = new QPushButton(tr("Insert"), this);
    vboxLayout->addWidget(bInsert);

    QHBoxLayout *refreshButtonsLayout = new QHBoxLayout();
    bRefresh = new QPushButton(tr("Refresh"), this);
    refreshButtonsLayout->addWidget(bRefresh);
    bRebuild = new QPushButton(tr("Rebuild"), this);
    refreshButtonsLayout->addWidget(bRebuild);
    vboxLayout->addLayout(refreshButtonsLayout);

    buildTree();

    connect(dirView, &QTreeView::expanded, this, &QG_LibraryWidget::expandView);
    connect(dirView, &QTreeView::collapsed, this, &QG_LibraryWidget::collapseView);
    connect(dirView, &QTreeView::clicked, this, &QG_LibraryWidget::updatePreview);
    connect(bInsert, &QPushButton::clicked, this, &QG_LibraryWidget::insert);
    connect(bRefresh, &QPushButton::clicked, this, &QG_LibraryWidget::refresh);
    connect(bRebuild, &QPushButton::clicked, this, &QG_LibraryWidget::buildTree);

    updateWidgetSettings();
}

QG_LibraryWidget::~QG_LibraryWidget() = default;

void QG_LibraryWidget::setGraphicView([[maybe_unused]]RS_GraphicView* gview) {
    // todo - add further processing later
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
    QItemSelectionModel* selIconView = ivPreview->selectionModel();
    QModelIndex idx = selIconView->currentIndex();
    QStandardItem * item = iconModel->itemFromIndex ( idx );
    if (item == nullptr) {
        return;
    }

    QString dxfPath = getItemPath(item);

    if (QFileInfo(dxfPath).isReadable()) {
        if (actionHandler) {
		std::shared_ptr<RS_ActionInterface> a =
                actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a) {
                auto* action = static_cast<RS_ActionLibraryInsert*>(a.get());
                action->setFile(std::move(dxfPath));
            } else {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "QG_LibraryWidget::insert:"
                                "Cannot create action RS_ActionLibraryInsert");
            }
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LibraryWidget::insert: Can't read file: '%s'", dxfPath.toLatin1().data());
    }
}


/**
 * Refresh
 */
void QG_LibraryWidget::refresh() {
    scanTree();
    updatePreview(dirView->selectionModel()->currentIndex());
}


/**
 * Scan library tree for new items
 */
void QG_LibraryWidget::scanTree() {
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    foreach(auto& directory, directoryList) {
        appendTree(nullptr, directory);
    }

    QString customPath= LC_GET_ONE_STR("Paths", "Library", "");

    if(customPath.size()>0){
        //todo: make the custom path more flexible
        appendTree(nullptr,customPath);
    }
}


/**
 * (Re)build dirModel and iconModel from scratch
 */
void QG_LibraryWidget::buildTree() {
    dirModel = std::make_unique<QStandardItemModel>();
    iconModel = std::make_unique<QStandardItemModel>();
    scanTree();
    dirView->setModel(dirModel.get());
    ivPreview->setModel(iconModel.get());
    dirModel->setHorizontalHeaderLabels ( QStringList(tr("Directories")));
}


/**
 * Appends the given directory to the given list view item. Called recursively until all
 * library directories are appended.
 *
 * @author Rallaz
 */
void QG_LibraryWidget::appendTree(QStandardItem* item, QString directory) {
//    QStringList::Iterator it;
    QDir dir(directory);

	if (!dir.exists()) {
	    return;
	}

    // read subdirectories of this directory:
    QStringList lDirectoryList = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);

	if (!item) item = dirModel->invisibleRootItem();

    foreach (const auto& lDirectory, lDirectoryList) {
		QStandardItem* newItem=nullptr;

        // Look for an item already existing and take this
        //   instead of making a new one:
        for (int j = 0; j < item->rowCount(); ++j) {
			QStandardItem* const searchItem = item->child (j);
            if (searchItem->text() == lDirectory) {
                newItem=searchItem;
                break;
            }
        }

        // Create new item if no existing was found:
        if (!newItem) {
            newItem = new QStandardItem(QIcon(":/icons/folderclosed.lci"), lDirectory);
            item->setChild(item->rowCount(), newItem);
        }
        appendTree(newItem, directory+QDir::separator()+lDirectory);
    }
    item->sortChildren ( 0, Qt::AscendingOrder );
}

/**
 * Change the icon item when is expanded.
 *
 * @author Rallaz
 */
void QG_LibraryWidget::expandView( QModelIndex idx ){
    QStandardItem * item = dirModel->itemFromIndex ( idx );
    if (item != nullptr) {
        item->setIcon(QIcon(":/icons/fileopen.lci"));
    }
}

/**
 * Change the icon item when is collapsed.
 *
 * @author Rallaz
 */
void QG_LibraryWidget::collapseView( QModelIndex idx ){
    QStandardItem * item = dirModel->itemFromIndex ( idx );
    if (item != nullptr) {
        item->setIcon(QIcon(":/icons/folderclosed.lci"));
    }
}

/**
 * Updates the icon preview.
 *
 * @author Rallaz
 */
void QG_LibraryWidget::updatePreview(QModelIndex idx) {
    QStandardItem * item = dirModel->itemFromIndex ( idx );
    if (item == nullptr) {
        return;
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    // dir from the point of view of the library browser (e.g. /mechanical/screws)
    QString directory = getItemDir(item); //RLZ change to do-while
    iconModel->clear();

    // List of all directories that contain part libraries:
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    QDir itemDir;
    QStringList itemPathList;

    // look in all possible system directories for DXF files in the current library path:
    for (int i = 0; i < directoryList.size(); ++i) {
        itemDir.setPath(directoryList.at(i)+directory);

        if (itemDir.exists()) {
            QStringList itemNameList =
                itemDir.entryList(QStringList("*.dxf"), QDir::Files, QDir::Name);
            for (int j = 0; j < itemNameList.size(); ++j) {
                itemPathList += itemDir.path()+QDir::separator()+itemNameList.at(j);
            }
        }
    }

    // Sort entries:
    itemPathList.sort();

    // Fill items into icon view:
    for (int i = 0; i < itemPathList.size(); ++i) {
        QString label = QFileInfo(itemPathList.at(i)).completeBaseName();
        QIcon icon = getIcon(directory, QFileInfo(itemPathList.at(i)).fileName(), itemPathList.at(i));
        auto newItem = new QStandardItem(icon, label);
        iconModel->setItem(i, newItem);
    }
    QApplication::restoreOverrideCursor();
}

 //RLZ change to do-while
/**
 * @return Directory (in terms of the List view) to the given item (e.g. /mechanical/screws)
 */
QString QG_LibraryWidget::getItemDir(QStandardItem* item) {
    if (item == nullptr)
        return {};

    QStandardItem* parent = item->parent();
    return getItemDir(parent) + QDir::separator() + QString("%1").arg(item->text());
}


/**
 * @return Path of the DXF file that is represented by the given item.
 */
QString QG_LibraryWidget::getItemPath(QStandardItem* item) {
    if (item == nullptr)
        return {};
    QItemSelectionModel* selDirView = dirView->selectionModel();
    QModelIndex idx = selDirView->currentIndex();
    QStandardItem * dirItem = dirModel->itemFromIndex ( idx );
    QString dir = getItemDir(dirItem);

    // List of all directories that contain part libraries:
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");

    // look in all possible system directories for DXF files in the current library path:
    for (auto it=directoryList.begin(); it!=directoryList.end(); ++it) {
        QDir itemDir((*it)+dir);
        if (itemDir.exists()) {
            QString dxfFile = (*it) + dir + QDir::separator() + item->text() + ".dxf";
            if (QFileInfo(dxfFile).isReadable()) {
                return dxfFile;
            }
        }
    }

    return {};
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
QIcon QG_LibraryWidget::getIcon(const QString& dir, const QString& dxfFile,
                                    const QString& dxfPath) {
    QString pngFile = getPathToPixmap(dir, dxfFile, dxfPath);
    QFileInfo fiPng(pngFile);

    // found existing thumbnail:
    if (fiPng.isFile()) {
        return QIcon(pngFile);
    }
    // default thumbnail:
    else {
        return QIcon(QPixmap(64,64));
    }
}

// fixme - sand - files - generation of thumbnails should be extracted!!! Do this on general rework of the library

/**
 * @return Path to the thumbnail of the given DXF file. If no thumbnail exists, one is
 * created in the user's home. If no thumbnail can be created, an empty string is returned.
 */
QString QG_LibraryWidget::getPathToPixmap(const QString& dir,
        const QString& dxfFile,
        const QString& dxfPath) {

    // the thumbnail must be created in the user's home.
    QString iconCacheLocation=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "iconCache" + QDir::separator();

    RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: "
                    "dir: '%s' dxfFile: '%s' dxfPath: '%s'",
                    dir.toLatin1().data(), dxfFile.toLatin1().data(), dxfPath.toLatin1().data());

    // List of all directories that contain part libraries:
    QStringList directoryList = RS_SYSTEM->getDirectoryList("library");
    directoryList.prepend(iconCacheLocation);

    QFileInfo fiDxf(dxfPath);

    // look in all possible system directories for PNG files
    //  in the current library path:
    foreach (QString path, directoryList) {
        QString itemDir = path + dir;
        QString pngPath = itemDir + QDir::separator() + fiDxf.baseName() + ".png";
        RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: checking: '%s'",
                        pngPath.toLatin1().data());
        QFileInfo fiPng(pngPath);

        // the thumbnail exists:
        if (fiPng.isFile()) {
            RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: dxf date: %s, png date: %s",
                            fiDxf.lastModified().toString().toLatin1().data(), fiPng.lastModified().toString().toLatin1().data());
            if (fiPng.lastModified() > fiDxf.lastModified()) {
                RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: thumbnail found: '%s'",
                                pngPath.toLatin1().data());
                return pngPath;
            } else {
                RS_DEBUG->print("QG_LibraryWidget::getPathToPixmap: thumbnail needs to be updated: '%s'",
                                pngPath.toLatin1().data());
            }
        }
    }

    // create all directories needed:
    RS_SYSTEM->createPaths(iconCacheLocation + dir);

    QString pngPath = iconCacheLocation + dir + QDir::separator() + fiDxf.baseName() + ".png";

    QPixmap buffer(128,128); // fixme - sand - add settings for thumbnail size, generate per setting!
    RS_Painter painter(&buffer);
    painter.setBackground(RS_Color(255,255,255));
    painter.eraseRect(0,0, 128,128);

    LC_GraphicViewport viewport;
    viewport.setSize(128,128);

    RS_Graphic graphic;

    LC_DocumentsStorage storage;

    if (!storage.loadDocument(&graphic, dxfPath, RS2::FormatUnknown)) {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_LibraryWidget::getPathToPixmap: Cannot open file: '%s'",
                        dxfPath.toLatin1().data());
        return {};
    }

    viewport.setContainer(&graphic);
    viewport.initAfterDocumentOpen();
    viewport.zoomAuto(false);

    LC_PrintViewportRenderer renderer(&viewport, &painter);
    renderer.loadSettings();
    renderer.setupPainter(&painter);

    for(RS_Entity* e: lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
        if (e != nullptr && e->rtti() != RS2::EntityHatch) {
            RS_Pen pen = e->getPen();
            pen.setColor(Qt::black);
            e->setPen(pen);
            renderer.justDrawEntity(&painter, e);
        }
    }

    // GraphicView deletes painter
    painter.end();

    // Write to PNG
    writePng(pngPath, std::move(buffer));
    LC_LOG << "Writing to " << pngPath << " OK";
    return pngPath;
}

void QG_LibraryWidget::updateWidgetSettings(){
    LC_GROUP("Widgets"); {
        bool flatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        int iconSize = LC_GET_INT("DockWidgetsIconSize", 16);

        QSize size(iconSize, iconSize);

        QList<QToolButton *> widgets = this->findChildren<QToolButton *>();
        foreach(QToolButton *w, widgets) {
            w->setAutoRaise(flatIcons);
            w->setIconSize(size);
        }
    }
    LC_GROUP_END();
}
