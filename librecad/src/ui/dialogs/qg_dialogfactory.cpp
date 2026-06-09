/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) librecad.org
** Copyright (C) 2024 sand1024
** Copyright (C) 2018 webmite <ianm.main@gmail.com>
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "qg_dialogfactory.h"

#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QRegularExpression>

#include "lc_dlg_dimension.h"
#include "lc_dlg_entityproperties.h"
#include "lc_parabola.h"
#include "qc_applicationwindow.h"
#include "qg_blockdialog.h"
#include "qg_commandwidget.h"
#include "qg_dlg_attributes.h"
#include "qg_dlg_hatch.h"
#include "qg_dlg_mtext.h"
#include "qg_dlg_text.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_dlgoptionsmakercam.h"
#include "qg_layerdialog.h"
#include "qg_selectionwidget.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_hatch.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_text.h"

class LC_EntityPropertiesDlg;
/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param optionsToolbar
 * @param snapOptionsHolder
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, [[maybe_unused]] QToolBar* optionsToolbar,
                                   [[maybe_unused]] LC_SnapOptionsWidgetsHolder* snapOptionsHolder)
    : parent(parent) {
}

/**
 * Destructor
 */
QG_DialogFactory::~QG_DialogFactory() {
    RS_DEBUG->print("QG_DialogFactory::~QG_DialogFactory");
    RS_DEBUG->print("QG_DialogFactory::~QG_DialogFactory: OK");
}

/**
 * Shows a message dialog.
 */
void QG_DialogFactory::requestWarningDialog(const QString& warning) {
    QMessageBox::information(parent, QMessageBox::tr("Warning"), warning, QMessageBox::Ok);
}

/**
 * Shows a dialog for adding a layer. Doesn't add the layer.
 * This is up to the caller.
 *
 * @return a pointer to the newly created layer that
 * should be added.
 */
RS_Layer* QG_DialogFactory::requestNewLayerDialog(RS_LayerList* layerList) {
    RS_Layer* layer{nullptr};

    QString newLayerName;
    if (nullptr != layerList) {
        QString layerName = layerList->getActive()->getName();
        if (layerName.isEmpty() || layerName.compare("0") == 0) {
            layerName = QObject::tr("noname", "default layer name");
        }
        newLayerName = layerName;

        QString sBaseLayerName(layerName);
        int nlen{1};
        int i{0};
        const QRegularExpression re("^(.*\\D+|)(\\d*)$");
        const auto match(re.match(layerName));
        if (match.hasMatch()) {
            sBaseLayerName = match.captured(1);
            if (1 < match.lastCapturedIndex()) {
                const QString sNumLayerName = match.captured(2);
                nlen = sNumLayerName.length();
                i = sNumLayerName.toInt();
            }
        }

        while (layerList->find(newLayerName) != nullptr) {
            newLayerName = QString("%1%2").arg(sBaseLayerName).arg(++i, nlen, 10, QChar('0'));
        }
    }

    // Layer for parameter livery
    layer = new RS_Layer(newLayerName);

    QG_LayerDialog dlg(parent, "Layer Dialog");
    dlg.setLayer(layer);
    dlg.setLayerList(layerList);
    dlg.getQLineEdit()->selectAll();
    if (dlg.exec() != 0) {
        dlg.updateLayer();
    }
    else {
        delete layer;
        layer = nullptr;
    }

    return layer;
}

/**
 * Shows a dialog that asks the user if the selected layer
 * can be removed. Doesn't remove the layer. This is up to the caller.
 *
 * @return a pointer to the layer that should be removed.
 */
RS_Layer* QG_DialogFactory::requestLayerRemovalDialog(RS_LayerList* layerList) {
    RS_Layer* layer = nullptr;
    if (layerList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestLayerRemovalDialog(): " "layerList is nullptr");
        return nullptr;
    }
    /*
       if (!layerList) {
           if (container && container->rtti()==RS2::EntityGraphic) {
               layerList = (RS_LayerList*)container;
           } else {
               return nullptr;
           }
       }*/

    // Layer for parameter livery
    layer = layerList->getActive();

    if (layer != nullptr) {
        if (layer->getName() != "0") {
            QMessageBox msgBox(QMessageBox::Warning, QMessageBox::tr("Remove Layer"),
                               QMessageBox::tr("Layer \"%1\" and all " "entities on it will be removed.\n" "This action can NOT be undone.")
                              .arg(layer->getName()), QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec() == QMessageBox::Ok) {
            }
            else {
                layer = nullptr;
            }
        }
        else {
            QMessageBox::information(parent, QMessageBox::tr("Remove Layer"),
                                     QMessageBox::tr("Layer \"%1\" can never " "be removed.").arg(layer->getName()), QMessageBox::Ok);
        }
    }

    return layer;
}

/**
 * Shows a dialog that asks the user if the selected layers
 * can be removed. Doesn't remove the layers. This is up to the caller.
 *
 * @return a list of layers names to be removed.
 */
QStringList QG_DialogFactory::requestSelectedLayersRemovalDialog(RS_LayerList* layerList) {
    if (layerList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestSelectedLayersRemovalDialog(): " "layerList is nullptr");
        return QStringList();
    }

    QStringList names;
    bool layer0Selected{false};

    // first, try to add selected layers
    for (const auto layer : *layerList) {
        if (layer == nullptr) {
            continue;
        }
        if (!layer->isVisibleInLayerList()) {
            continue;
        }
        if (!layer->isSelectedInLayerList()) {
            continue;
        }
        if (layer->getName() == "0") {
            layer0Selected = true;
            continue;
        }
        names << layer->getName();
    }

    // then, if there're no selected layers,
    // try to add active layer instead
    if (names.isEmpty()) {
        const RS_Layer* layer = layerList->getActive();
        if ((layer != nullptr) && layer->isVisibleInLayerList()) {
            if (layer->getName() == "0") {
                layer0Selected = true;
            }
            else {
                names << layer->getName();
            }
        }
    }

    // still there're no layers, so return
    if (names.isEmpty()) {
        if (layer0Selected) {
            QMessageBox::information(parent, QMessageBox::tr("Remove Layer"), QMessageBox::tr("Layer \"0\" can never be removed."),
                                     QMessageBox::Ok);
        }
        return names; // empty, nothing to remove
    }

    // layers added, show confirmation dialog

    const QString title(QMessageBox::tr("Remove %n layer(s)", "", names.size()));

    QStringList textLines = {
        QMessageBox::tr("Listed layers and all entities on them will be removed."),
        "",
        QMessageBox::tr("Warning: this action can NOT be undone!"),
    };

    if (layer0Selected) {
        textLines << "" << QMessageBox::tr("Warning: layer \"0\" can never be removed.");
    }

    QStringList detailLines = {QMessageBox::tr("Layers for removal:"), "",};
    detailLines << names;

    QMessageBox msgBox(QMessageBox::Warning, title, textLines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel);

    msgBox.setDetailedText(detailLines.join("\n"));

    if (msgBox.exec() == QMessageBox::Ok) {
        return names;
    }

    return QStringList();
}

/**
 * Shows a dialog for editing a layer. A new layer is created and
 * returned. Modifying the layer is up to the caller.
 *
 * @return A pointer to a new layer with the changed attributes
 *         or nullptr if the dialog was cancelled.
 */
RS_Layer* QG_DialogFactory::requestEditLayerDialog(RS_LayerList* layerList) {
    RS_DEBUG->print("QG_DialogFactory::requestEditLayerDialog");

    RS_Layer* layer = nullptr;
    /*
       if (!layerList) {
           if (container && container->rtti()==RS2::EntityGraphic) {
               layerList = (RS_LayerList*)container;
           } else {
               return nullptr;
           }
       }
    */

    if (layerList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestEditLayerDialog(): " "layerList is nullptr");
        return nullptr;
    }

    // Layer for parameter livery
    if (layerList->getActive() != nullptr) {
        layer = new RS_Layer(*layerList->getActive());

        QG_LayerDialog dlg(parent, QMessageBox::tr("Layer Dialog"));
        dlg.setLayer(layer);
        dlg.setLayerList(layerList);
        dlg.setEditLayer(true);
        if (dlg.exec() != 0) {
            dlg.updateLayer();
        }
        else {
            delete layer;
            layer = nullptr;
        }
    }

    return layer;
}

/**
 * Shows a dialog for adding a block. Doesn't add the block.
 * This is up to the caller.
 *
 * @return a pointer to the newly created block that
 * should be added.
 */
RS_BlockData QG_DialogFactory::requestNewBlockDialog(RS_BlockList* blockList) {
    //RS_Block* block = nullptr;
    auto ret = RS_BlockData("", RS_Vector(false), false);

    if (blockList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestNewBlockDialog(): " "blockList is nullptr");
        return ret;
    }

    // Block for parameter livery
    //block = new RS_Block(container, "noname", RS_Vector(0.0,0.0));

    QG_BlockDialog dlg(parent);
    dlg.setBlockList(blockList);
    if (dlg.exec() != 0) {
        ret = dlg.getBlockData();
    }

    return ret;
}

/**
 * Shows a dialog for renaming the currently active block.
 *
 * @return a pointer to the modified block or nullptr on cancellation.
 */
RS_BlockData QG_DialogFactory::requestBlockAttributesDialog(RS_BlockList* blockList) {
    //RS_Block* block = nullptr;
    auto ret = RS_BlockData("", RS_Vector(false), false);

    if (blockList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestBlockAttributesDialog(): " "blockList is nullptr");
        return ret;
    }
    /*
       if (!blockList) {
           if (container && container->rtti()==RS2::EntityGraphic) {
               blockList = (RS_BlockList*)container;
           } else {
               return nullptr;
           }
       }*/

    // Block for parameter livery
    //block = blockList->getActive();

    //    QG_BlockDialog dlg(parent, "Rename Block");
    QG_BlockDialog dlg(parent);
    dlg.setBlockList(blockList);
    if (dlg.exec() != 0) {
        //dlg.updateBlock();
        //block->setData();
        ret = dlg.getBlockData();
    }
    //else {
    //	ret = RS_BlockData("", RS_Vector(false));
    //}

    return ret;
}

/**
 * Shows a dialog that asks the user if the selected block
 * can be removed. Doesn't remove the block. This is up to the caller.
 *
 * @return a pointer to the block that should be removed.
 */
RS_Block* QG_DialogFactory::requestBlockRemovalDialog(RS_BlockList* blockList) {
    RS_Block* block = nullptr;

    if (blockList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestBlockRemovalDialog(): " "blockList is nullptr");
        return nullptr;
    }

    // Block for parameter livery
    block = blockList->getActive();

    if (block != nullptr) {
        const int remove = QMessageBox::warning(parent, QMessageBox::tr("Remove Block"),
                                          QMessageBox::tr("Block \"%1\" and all " "its entities will be removed.").arg(block->getName()),
                                          QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
        if (remove == QMessageBox::Ok) {
        }
        else {
            block = nullptr;
        }
    }

    return block;
}

/**
 * Shows a dialog that asks the user if the selected blocks
 * can be removed. Doesn't remove the blocks. This is up to the caller.
 *
 * @return a list of blocks to be removed.
 */
QList<RS_Block*> QG_DialogFactory::requestSelectedBlocksRemovalDialog(RS_BlockList* blockList) {
    if (blockList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_DialogFactory::requestSelectedBlocksRemovalDialog(): " "blockList is nullptr");
        return QList<RS_Block*>();
    }

    QList<RS_Block*> blocks;

    // first, try to add selected blocks
    for (const auto block : *blockList) {
        if (block == nullptr) {
            continue;
        }
        if (!block->isVisibleInBlockList()) {
            continue;
        }
        if (!block->isSelectedInBlockList()) {
            continue;
        }
        blocks << block;
    }

    // then, if there're no selected blocks,
    // try to add active block instead
    if (blocks.isEmpty()) {
        RS_Block* block = blockList->getActive();
        if ((block != nullptr) && block->isVisibleInBlockList()) {
            blocks << block;
        }
    }

    // still nothing was added, so return
    if (blocks.isEmpty()) {
        return blocks; // empty, nothing to remove
    }

    // blocks added, show confirmation dialog

    const QString title(QMessageBox::tr("Remove %n block(s)", "", blocks.size()));

    const QString text(QMessageBox::tr("Listed blocks and all their entities will be removed."));

    QStringList detailLines = {QMessageBox::tr("Blocks for removal:"), "",};
    for (const auto block : std::as_const(blocks)) {
        detailLines << block->getName();
    }

    QMessageBox msgBox(QMessageBox::Warning, title, text, QMessageBox::Ok | QMessageBox::Cancel);

    msgBox.setDetailedText(detailLines.join("\n"));

    if (msgBox.exec() == QMessageBox::Ok) {
        return blocks;
    }

    return QList<RS_Block*>();
}

/**
 * Shows a dialog for choosing a file name. Opening the file is up to
 * the caller.
 *
 * @return File name with path and extension to determine the file type
 *         or an empty string if the dialog was cancelled.
 */
QString QG_DialogFactory::requestImageOpenDialog() {
    QString strFileName = "";

    // read default settings:
    LC_GROUP("Paths");
    const QString defDir = LC_GET_STR("OpenImage", RS_SYSTEM->getHomeDir());
    QString defFilter = LC_GET_STR("ImageFilter", "");
    LC_GROUP_END();

    QStringList filters;
    QString all = "";
    bool haveJpeg = false;
    for (const QByteArray& format : QImageReader::supportedImageFormats()) {
        if (format.toUpper() == "JPG" || format.toUpper() == "JPEG") {
            if (!haveJpeg) {
                haveJpeg = true;
                filters.append("jpeg (*.jpeg *.jpg)");
                all += " *.jpeg *.jpg";
            }
        }
        else {
            filters.append(QString("%1 (*.%1)").arg(QString(format)));
            all += QString(" *.%1").arg(QString(format));
        }
    }
    const QString strAllImageFiles = QObject::tr("All Image Files (%1)").arg(all);
    filters.append(strAllImageFiles);
    filters.append(QObject::tr("All Files (*.*)"));

    QFileDialog fileDlg(nullptr, "");
    fileDlg.setModal(true);
    fileDlg.setFileMode(QFileDialog::ExistingFile);
    fileDlg.setWindowTitle(QObject::tr("Open Image"));
    fileDlg.setDirectory(defDir);
    fileDlg.setNameFilters(filters);
    if (defFilter.isEmpty()) {
        defFilter = strAllImageFiles;
    }
    fileDlg.selectNameFilter(defFilter);

    if (QDialog::Accepted == fileDlg.exec()) {
        QStringList strSelectedFiles = fileDlg.selectedFiles();
        if (!strSelectedFiles.isEmpty()) {
            strFileName = strSelectedFiles.first();
        }

        // store new default settings:
        LC_GROUP_GUARD("Paths");
        {
            LC_SET("OpenImage", QFileInfo(strFileName).absolutePath());
            LC_SET("ImageFilter", fileDlg.selectedNameFilter());
        }
    }
    return strFileName;
}

/**
 * Shows attributes options dialog presenting the given data.
 */
bool QG_DialogFactory::requestAttributesDialog(RS_AttributesData& data, RS_LayerList& layerList) {
    QG_DlgAttributes dlg(parent);
    dlg.setData(&data, layerList);
    if (dlg.exec() != 0) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows a dialog to edit the given entity.
 */

// fixme - sand - files - remove from there, move to action or so (introduces additional dependencies)
bool QG_DialogFactory::requestModifyEntityDialog(RS_Entity* entity, [[maybe_unused]] LC_GraphicViewport* viewport) {
    if (entity == nullptr) {
        return false;
    }

    bool ret = false;

    // fixme Sand - RESTORE!!!! Rework via EntityPropertiesEditor or, even better, via switchToAction!!!
    // LC_DlgEntityProperties* dlg = new LC_DlgEntityProperties(parent, viewport, entity);
    // ret = dlg->exec() == QDialog::Accepted;
    // delete dlg;
    LC_EntityPropertiesDlg* editDialog{nullptr};
    bool hasDialog = true;

    switch (entity->rtti()) {
        case RS2::EntityDimAligned:
        case RS2::EntityDimAngular:
        case RS2::EntityDimDiametric:
        case RS2::EntityDimRadial:
        case RS2::EntityDimArc:
        case RS2::EntityDimOrdinate:
        case RS2::EntityDimLinear: {
            editDialog = new LC_DlgDimension(parent, viewport, dynamic_cast<RS_Dimension*>(entity));
            break;
        }
        case RS2::EntityMText: {
            editDialog = new QG_DlgMText(parent, viewport, dynamic_cast<RS_MText*>(entity), false);
            break;
        }
        case RS2::EntityText: {
            editDialog = new QG_DlgText(parent, viewport, dynamic_cast<RS_Text*>(entity), false);
            break;
        }
        case RS2::EntityHatch: {
            editDialog = new QG_DlgHatch(parent, viewport, dynamic_cast<RS_Hatch*>(entity), false);
            break;
        }
        default:
            hasDialog = false;
            break;
    }

    if (hasDialog) {
        if (editDialog->exec() != 0) {
            editDialog->updateEntity();
            ret = true;
        }
        delete editDialog;
    }

    return ret;
}

/**
 * Shows a dialog to edit the attributes of the given multi-line text entity.
 */
bool QG_DialogFactory::requestMTextDialog(RS_MText* text, LC_GraphicViewport* viewport) {
    if (text == nullptr) {
        return false;
    }

    QG_DlgMText dlg(parent, viewport, text, true);
    if (dlg.exec() != 0) {
        dlg.updateEntity();
        return true;
    }

    return false;
}

/**
 * Shows a dialog to edit the attributes of the given text entity.
 */
bool QG_DialogFactory::requestTextDialog(RS_Text* text, LC_GraphicViewport* viewport) {
    if (text == nullptr) {
        return false;
    }

    QG_DlgText dlg(parent, viewport, text, true);
    if (dlg.exec() != 0) {
        dlg.updateEntity();
        return true;
    }

    return false;
}

/**
 * Shows a dialog to edit pattern / hatch attributes of the given entity.
 */
bool QG_DialogFactory::requestHatchDialog(RS_Hatch* hatch, LC_GraphicViewport* viewport) {
    if (hatch == nullptr) {
        return false;
    }

    RS_PATTERNLIST->init();

    QG_DlgHatch dlg(parent, viewport, hatch, true);
    if (dlg.exec() != 0) {
        dlg.updateEntity();
        dlg.saveSettings();
        return true;
    }
    return false;
}

/**
 * Shows dialog for drawing options.
 */
int QG_DialogFactory::requestOptionsDrawingDialog(RS_Graphic& graphic, const int tabIndex) {
    QG_DlgOptionsDrawing dlg(parent);
    dlg.setGraphic(&graphic);
    dlg.showInitialTab(tabIndex);
    const int result = dlg.exec();
    return result;
}

bool QG_DialogFactory::requestOptionsMakerCamDialog() {
    QG_DlgOptionsMakerCam dlg(parent);
    return (dlg.exec() == QDialog::Accepted);
}

QString QG_DialogFactory::requestFileSaveAsDialog(const QString& caption /* = QString() */, const QString& dir /* = QString() */,
                                                  const QString& filter /* = QString() */, QString* selectedFilter /* = 0 */) {
    return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter);
}

void QG_DialogFactory::displayBlockName(const QString& blockName, const bool display) {
    if (m_selectionWidget != nullptr) {
        m_selectionWidget->flashAuxData(QString("Block Name"), blockName, QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT,
                                        display);
    }
}

/**
 * Called when an action needs to communicate 'message' to the user.
 */
void QG_DialogFactory::commandMessage(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::commandMessage");
    if (m_commandWidget != nullptr) {
        m_commandWidget->appendHistory(message);
    }
    RS_DEBUG->print("QG_DialogFactory::commandMessage: OK");
}

void QG_DialogFactory::command(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::command");
    if (m_commandWidget != nullptr) {
        m_commandWidget->setInput(message);
    }
    RS_DEBUG->print("QG_DialogFactory::command: OK");
}

void QG_DialogFactory::commandPrompt(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::command");
    if (m_commandWidget != nullptr) {
        m_commandWidget->setCommand(message);
    }
    RS_DEBUG->print("QG_DialogFactory::command: OK");
}

/**
 * Converts an extension to a format description.
 * e.g. "PNG" to "Portable Network Graphic"
 *
 * @param ext
 * @return Format description
 */
QString QG_DialogFactory::extToFormat(const QString& ext) {
    const QString e = ext.toLower();

    if (e == "bmp") {
        return QObject::tr("Windows Bitmap");
    }
    if (e == "jpeg" || e == "jpg") {
        return QObject::tr("Joint Photographic Experts Group");
    }
    if (e == "gif") {
        return QObject::tr("Graphics Interchange Format");
    }
    if (e == "mng") {
        return QObject::tr("Multiple-image Network Graphics");
    }
    if (e == "pbm") {
        return QObject::tr("Portable Bit Map");
    }
    if (e == "pgm") {
        return QObject::tr("Portable Grey Map");
    }
    if (e == "png") {
        return QObject::tr("Portable Network Graphic");
    }
    if (e == "ppm") {
        return QObject::tr("Portable Pixel Map");
    }
    if (e == "xbm") {
        return QObject::tr("X Bitmap Format");
    }
    if (e == "xpm") {
        return QObject::tr("X Pixel Map");
    }
    if (e == "svg") {
        return QObject::tr("Scalable Vector Graphics");
    }
    if (e == "bw") {
        return QObject::tr("SGI Black & White");
    }
    if (e == "eps") {
        return QObject::tr("Encapsulated PostScript");
    }
    if (e == "epsf") {
        return QObject::tr("Encapsulated PostScript Format");
    }
    if (e == "epsi") {
        return QObject::tr("Encapsulated PostScript Interchange");
    }
    if (e == "ico") {
        return QObject::tr("Windows Icon");
    }
    if (e == "jp2") {
        return QObject::tr("JPEG 2000");
    }
    if (e == "pcx") {
        return QObject::tr("ZSoft Paintbrush");
    }
    if (e == "pic") {
        return QObject::tr("PC Paint");
    }
    if (e == "rgb" || e == "rgba" || e == "sgi") {
        return QObject::tr("SGI-Bilddatei");
    }
    if (e == "tga") {
        return QObject::tr("Targa Image File");
    }
    if (e == "tif" || e == "tiff") {
        return QObject::tr("Tagged Image File Format");
    }
    return ext.toUpper();
}
