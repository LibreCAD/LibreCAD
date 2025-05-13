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

#include "LC_DlgParabola.h"
#include "lc_dimordinate.h"
#include "lc_dlgdimordinate.h"
#include "lc_dlgsplinepoints.h"
#include "lc_parabola.h"
#include "qc_applicationwindow.h"
#include "qg_blockdialog.h"
#include "qg_commandwidget.h"
#include "qg_dlgarc.h"
#include "qg_dlgattributes.h"
#include "qg_dlgcircle.h"
#include "qg_dlgdimension.h"
#include "qg_dlgdimlinear.h"
#include "qg_dlgellipse.h"
#include "qg_dlghatch.h"
#include "qg_dlgimage.h"
#include "qg_dlginsert.h"
#include "qg_dlgline.h"
#include "qg_dlgmirror.h"
#include "qg_dlgmove.h"
#include "qg_dlgmoverotate.h"
#include "qg_dlgmtext.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_dlgoptionsmakercam.h"
#include "qg_dlgpoint.h"
#include "qg_dlgpolyline.h"
#include "qg_dlgrotate.h"
#include "qg_dlgrotate2.h"
#include "qg_dlgscale.h"
#include "qg_dlgspline.h"
#include "qg_dlgtext.h"
#include "qg_layerdialog.h"
#include "qg_selectionwidget.h"
#include "rs_arc.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_dimlinear.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"
#include "rs_patternlist.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_spline.h"
#include "rs_system.h"
#include "rs_text.h"


//QG_DialogFactory* QG_DialogFactory::uniqueInstance = nullptr;

class LC_EntityPropertiesDlg;
/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param ow Pointer to widget that can host option widgets.
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, [[maybe_unused]]QToolBar* optionsToolbar, [[maybe_unused]] LC_SnapOptionsWidgetsHolder* snapOptionsHolder)
    : parent(parent)
{
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
    QMessageBox::information(parent, QMessageBox::tr("Warning"),
                             warning,
                             QMessageBox::Ok);
}

/**
 * Shows a dialog for adding a layer. Doesn't add the layer.
 * This is up to the caller.
 *
 * @return a pointer to the newly created layer that
 * should be added.
 */
RS_Layer* QG_DialogFactory::requestNewLayerDialog(RS_LayerList* layerList)
{
    RS_Layer* layer {nullptr};

    QString layer_name;
    QString newLayerName;
    if (nullptr != layerList) {
        layer_name = layerList->getActive()->getName();
        if (layer_name.isEmpty() || !layer_name.compare("0") ) {
            layer_name = QObject::tr( "noname", "default layer name");
        }
        newLayerName = layer_name;

        QString sBaseLayerName( layer_name);
        QString sNumLayerName;
        int nlen {1};
        int i {0};
        QRegularExpression re("^(.*\\D+|)(\\d*)$");
        auto match( re.match(layer_name));
        if (match.hasMatch()) {
            sBaseLayerName = match.captured(1);
            if( 1 < match.lastCapturedIndex()) {
                sNumLayerName = match.captured(2);
                nlen = sNumLayerName.length();
                i = sNumLayerName.toInt();
            }
        }

        while (layerList->find( newLayerName)) {
            newLayerName = QString("%1%2").arg( sBaseLayerName).arg( ++i, nlen, 10, QChar('0'));
        }
    }

    // Layer for parameter livery
    layer = new RS_Layer(newLayerName);

    QG_LayerDialog dlg(parent, "Layer Dialog");
    dlg.setLayer(layer);
    dlg.setLayerList(layerList);
    dlg.getQLineEdit()->selectAll();
    if (dlg.exec()) {
        dlg.updateLayer();
    } else {
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
    if (!layerList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestLayerRemovalDialog(): "
                        "layerList is nullptr");
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

    if (layer) {
        if (layer->getName()!="0") {
            QMessageBox msgBox(
                        QMessageBox::Warning,
                        QMessageBox::tr("Remove Layer"),
                        QMessageBox::tr("Layer \"%1\" and all "
                                        "entities on it will be removed.\n"
                                        "This action can NOT be undone.")
                        .arg(layer->getName()),
                        QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec()==QMessageBox::Ok) {}
            else {
                layer = nullptr;
            }
        } else {
            QMessageBox::information(parent,
                                     QMessageBox::tr("Remove Layer"),
                                     QMessageBox::tr("Layer \"%1\" can never "
                                                     "be removed.")
                                     .arg(layer->getName()),
                                     QMessageBox::Ok);
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
QStringList QG_DialogFactory::requestSelectedLayersRemovalDialog(
        RS_LayerList* layerList)
{

    if (!layerList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestSelectedLayersRemovalDialog(): "
                        "layerList is nullptr");
        return QStringList();
    }

    QStringList names;
    bool layer_0_selected {false};

    // first, try to add selected layers
    for (auto layer: *layerList) {
        if (!layer) continue;
        if (!layer->isVisibleInLayerList()) continue;
        if (!layer->isSelectedInLayerList()) continue;
        if (layer->getName() == "0") {
            layer_0_selected = true;
            continue;
        }
        names << layer->getName();
    }

    // then, if there're no selected layers,
    // try to add active layer instead
    if (names.isEmpty()) {
        RS_Layer* layer = layerList->getActive();
        if (layer && layer->isVisibleInLayerList()) {
            if (layer->getName() == "0") {
                layer_0_selected = true;
            } else {
                names << layer->getName();
            }
        }
    }

    // still there're no layers, so return
    if (names.isEmpty()) {
        if (layer_0_selected) {
            QMessageBox::information(
                        parent,
                        QMessageBox::tr("Remove Layer"),
                        QMessageBox::tr("Layer \"0\" can never be removed."),
                        QMessageBox::Ok
                        );
        }
        return names; // empty, nothing to remove
    }

    // layers added, show confirmation dialog

    QString title(
                QMessageBox::tr("Remove %n layer(s)", "", names.size())
                );

    QStringList text_lines = {
        QMessageBox::tr("Listed layers and all entities on them will be removed."),
        "",
        QMessageBox::tr("Warning: this action can NOT be undone!"),
    };

    if (layer_0_selected) {
        text_lines << "" <<
                      QMessageBox::tr("Warning: layer \"0\" can never be removed.");
    }

    QStringList detail_lines = {
        QMessageBox::tr("Layers for removal:"),
        "",
    };
    detail_lines << names;

    QMessageBox msgBox(
                QMessageBox::Warning,
                title,
                text_lines.join("\n"),
                QMessageBox::Ok | QMessageBox::Cancel
                );

    msgBox.setDetailedText(detail_lines.join("\n"));

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

    if (!layerList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestEditLayerDialog(): "
                        "layerList is nullptr");
        return nullptr;
    }

    // Layer for parameter livery
    if (layerList->getActive()) {
        layer = new RS_Layer(*layerList->getActive());

        QG_LayerDialog dlg(parent, QMessageBox::tr("Layer Dialog"));
        dlg.setLayer(layer);
        dlg.setLayerList(layerList);
        dlg.setEditLayer(true);
        if (dlg.exec()) {
            dlg.updateLayer();
        } else {
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
    RS_BlockData ret;
    ret = RS_BlockData("", RS_Vector(false), false);

    if (!blockList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestNewBlockDialog(): "
                        "blockList is nullptr");
        return ret;
    }

    // Block for parameter livery
    //block = new RS_Block(container, "noname", RS_Vector(0.0,0.0));

    QG_BlockDialog dlg(parent);
    dlg.setBlockList(blockList);
    if (dlg.exec()) {
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
    RS_BlockData ret;
    ret = RS_BlockData("", RS_Vector(false), false);

    if (!blockList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestBlockAttributesDialog(): "
                        "blockList is nullptr");
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
    if (dlg.exec()) {
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

    if (!blockList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestBlockRemovalDialog(): "
                        "blockList is nullptr");
        return nullptr;
    }

    // Block for parameter livery
    block = blockList->getActive();

    if (block) {
        int remove =
                QMessageBox::warning(parent,
                                     QMessageBox::tr("Remove Block"),
                                     QMessageBox::tr("Block \"%1\" and all "
                                                     "its entities will be removed.")
                                     .arg(block->getName()),
                                     QMessageBox::Ok | QMessageBox::Cancel,
                                     QMessageBox::Cancel);
        if (remove==QMessageBox::Ok) {}
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
QList<RS_Block*> QG_DialogFactory::requestSelectedBlocksRemovalDialog(
        RS_BlockList* blockList)
{

    if (!blockList) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestSelectedBlocksRemovalDialog(): "
                        "blockList is nullptr");
        return QList<RS_Block*>();
    }

    QList<RS_Block*> blocks;

    // first, try to add selected blocks
    for (auto block: *blockList) {
        if (!block) continue;
        if (!block->isVisibleInBlockList()) continue;
        if (!block->isSelectedInBlockList()) continue;
        blocks << block;
    }

    // then, if there're no selected blocks,
    // try to add active block instead
    if (blocks.isEmpty()) {
        RS_Block* block = blockList->getActive();
        if (block && block->isVisibleInBlockList()) {
            blocks << block;
        }
    }

    // still nothing was added, so return
    if (blocks.isEmpty()) {
        return blocks; // empty, nothing to remove
    }

    // blocks added, show confirmation dialog

    QString title(QMessageBox::tr("Remove %n block(s)", "", blocks.size()));

    QString text(QMessageBox::tr("Listed blocks and all their entities will be removed."));

    QStringList detail_lines = {
        QMessageBox::tr("Blocks for removal:"),
        "",
    };
    for (auto block: blocks) {
        detail_lines << block->getName();
    }

    QMessageBox msgBox(
                QMessageBox::Warning,
                title,
                text,
                QMessageBox::Ok | QMessageBox::Cancel
                );

    msgBox.setDetailedText(detail_lines.join("\n"));

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
QString QG_DialogFactory::requestImageOpenDialog(){
    QString strFileName = "";

    // read default settings:
    LC_GROUP("Paths");
    QString defDir = LC_GET_STR("OpenImage", RS_SYSTEM->getHomeDir());
    QString defFilter = LC_GET_STR("ImageFilter", "");
    LC_GROUP_END();

    QStringList filters;
    QString all = "";
    bool haveJpeg= false;
    for(const QByteArray& format: QImageReader::supportedImageFormats()) {
        if (format.toUpper() == "JPG" || format.toUpper() == "JPEG" ){
            if (!haveJpeg) {
                haveJpeg = true;
                filters.append("jpeg (*.jpeg *.jpg)");
                all += " *.jpeg *.jpg";
            }
        } else {
            filters.append(QString("%1 (*.%1)").arg(QString(format)));
            all += QString(" *.%1").arg(QString(format));
        }
    }
    QString strAllImageFiles = QObject::tr("All Image Files (%1)").arg(all);
    filters.append(strAllImageFiles);
    filters.append(QObject::tr("All Files (*.*)"));

    QFileDialog fileDlg(nullptr, "");
    fileDlg.setModal(true);
    fileDlg.setFileMode(QFileDialog::ExistingFile);
    fileDlg.setWindowTitle(QObject::tr("Open Image"));
    fileDlg.setDirectory(defDir);
    fileDlg.setNameFilters(filters);
    if (defFilter.isEmpty())
        defFilter = strAllImageFiles;
    fileDlg.selectNameFilter(defFilter);

    if (QDialog::Accepted == fileDlg.exec()) {
        QStringList strSelectedFiles = fileDlg.selectedFiles();
        if (!strSelectedFiles.isEmpty())
            strFileName = strSelectedFiles.first();

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
bool QG_DialogFactory::requestAttributesDialog(RS_AttributesData& data,
                                               RS_LayerList& layerList) {

    QG_DlgAttributes dlg(parent);
    dlg.setData(&data, layerList);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows move options dialog presenting the given data.
 */
bool QG_DialogFactory::requestMoveDialog(RS_MoveData& data) {
    QG_DlgMove dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows rotate options dialog presenting the given data.
 */
bool QG_DialogFactory::requestRotateDialog(RS_RotateData& data) {
    QG_DlgRotate dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows scale options dialog presenting the given data.
 */
bool QG_DialogFactory::requestScaleDialog(RS_ScaleData& data) {
    QG_DlgScale dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows mirror options dialog presenting the given data.
 */
bool QG_DialogFactory::requestMirrorDialog(RS_MirrorData& data) {
    QG_DlgMirror dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows move/rotate options dialog presenting the given data.
 */
bool QG_DialogFactory::requestMoveRotateDialog(RS_MoveRotateData& data) {
    QG_DlgMoveRotate dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows rotate around two centers options dialog presenting the given data.
 */
bool QG_DialogFactory::requestRotate2Dialog(RS_Rotate2Data& data) {
    QG_DlgRotate2 dlg(parent);
    dlg.setData(&data);
    if (dlg.exec()) {
        dlg.updateData();
        return true;
    }
    return false;
}

/**
 * Shows a dialog to edit the given entity.
 */

// fixme - sand - files - remove from there, move to action or so (introduces additional dependencies)
bool QG_DialogFactory::requestModifyEntityDialog(RS_Entity *entity, LC_GraphicViewport *viewport) {
    if (!entity) return false;

    bool ret = false;
    LC_EntityPropertiesDlg* editDialog;
    bool hasDialog = true;

    switch (entity->rtti()) {
    case RS2::EntityPoint: {
        editDialog = new QG_DlgPoint(parent, viewport, dynamic_cast<RS_Point *>(entity));
        break;
    }
    case RS2::EntityLine: {
        editDialog = new QG_DlgLine(parent, viewport, dynamic_cast<RS_Line *>(entity));
        break;
    }
    case RS2::EntityArc: {
        editDialog = new QG_DlgArc(parent, viewport, dynamic_cast<RS_Arc *>(entity));
        break;
    }
    case RS2::EntityCircle: {
        editDialog = new QG_DlgCircle(parent, viewport, dynamic_cast<RS_Circle *>(entity));
        break;
    }
    case RS2::EntityEllipse: {
        editDialog = new QG_DlgEllipse(parent, viewport,dynamic_cast<RS_Ellipse *>(entity));
        break;
    }
    case RS2::EntityParabola: {
        editDialog = new LC_DlgParabola(parent, viewport,dynamic_cast<LC_Parabola *>(entity));
        break;
    }
    case RS2::EntitySpline: {
        editDialog = new QG_DlgSpline(parent, viewport,dynamic_cast<RS_Spline *>(entity));
        break;
    }
    case RS2::EntitySplinePoints: {
        editDialog = new LC_DlgSplinePoints(parent, viewport,dynamic_cast<LC_SplinePoints *>(entity));
        break;
    }
    case RS2::EntityInsert: {
        editDialog = new QG_DlgInsert(parent, viewport,dynamic_cast<RS_Insert *>(entity));
        break;
    }
    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimDiametric:
    case RS2::EntityDimRadial:
    case RS2::EntityDimArc: {
        editDialog = new QG_DlgDimension(parent, viewport,dynamic_cast<RS_Dimension *>(entity));
        break;
    }
    case RS2::EntityDimLinear: {
        editDialog = new QG_DlgDimLinear(parent, viewport, dynamic_cast<RS_DimLinear *>(entity));
        break;
    }
    case RS2::EntityDimOrdinate: {
        editDialog = new LC_DlgDimOrdinate(parent, viewport, dynamic_cast<LC_DimOrdinate *>(entity));
        break;
    }
    case RS2::EntityMText: {
        editDialog = new QG_DlgMText(parent, viewport,dynamic_cast<RS_MText *>(entity), false);
        break;
    }
    case RS2::EntityText: {
        editDialog = new QG_DlgText(parent, viewport,dynamic_cast<RS_Text *>(entity), false);
        break;
    }
    case RS2::EntityHatch: {
        editDialog = new QG_DlgHatch(parent, viewport,dynamic_cast<RS_Hatch *>(entity), false);
        break;
    }
    case RS2::EntityPolyline: {
        editDialog = new QG_DlgPolyline(parent, viewport,dynamic_cast<RS_Polyline *>(entity));
        break;
    }
    case RS2::EntityImage: {
        editDialog = new QG_DlgImage(parent, viewport, dynamic_cast<RS_Image *>(entity));
        break;
    }
    default:
        hasDialog = false;
        break;
    }

    if (hasDialog){
        if (editDialog->exec()) {
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
bool QG_DialogFactory::requestMTextDialog(RS_MText *text, LC_GraphicViewport *viewport) {
    if (!text) return false;

    QG_DlgMText dlg(parent, viewport, text, true);
    if (dlg.exec()) {
        dlg.updateEntity();
        return true;
    }

    return false;
}

/**
 * Shows a dialog to edit the attributes of the given text entity.
 */
bool QG_DialogFactory::requestTextDialog(RS_Text *text, LC_GraphicViewport *viewport) {
    if (!text) return false;

    QG_DlgText dlg(parent, viewport, text, true);
    if (dlg.exec()) {
        dlg.updateEntity();
        return true;
    }

    return false;
}

/**
 * Shows a dialog to edit pattern / hatch attributes of the given entity.
 */
bool QG_DialogFactory::requestHatchDialog(RS_Hatch *hatch, LC_GraphicViewport *viewport) {
    if (!hatch) return false;

    RS_PATTERNLIST->init();

    QG_DlgHatch dlg(parent, viewport, hatch, true);
    if (dlg.exec()) {
        dlg.updateEntity();
        dlg.saveSettings();
        return true;
    }
    return false;
}


/**
 * Shows dialog for drawing options.
 */
int QG_DialogFactory::requestOptionsDrawingDialog(RS_Graphic& graphic, int tabIndex) {
    QG_DlgOptionsDrawing dlg(parent);
    dlg.setGraphic(&graphic);
    dlg.showInitialTab(tabIndex);
    int result = dlg.exec();
    return result;
}

bool QG_DialogFactory::requestOptionsMakerCamDialog() {
    QG_DlgOptionsMakerCam dlg(parent);
    return (dlg.exec() == QDialog::Accepted);
}

QString QG_DialogFactory::requestFileSaveAsDialog(const QString& caption /* = QString() */,
                                                  const QString& dir /* = QString() */,
                                                  const QString& filter /* = QString() */,
                                                  QString* selectedFilter /* = 0 */) {

    return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter);
}

/**
 * Called whenever the selection changed.
 */
void QG_DialogFactory::updateSelectionWidget(int num, double length) {
    if (m_selectionWidget != nullptr) {
        m_selectionWidget->setNumber(num);
        m_selectionWidget->setTotalLength(length);
    }
}

void QG_DialogFactory::displayBlockName(const QString& blockName, const bool& display){
    if (m_selectionWidget != nullptr)    {
        m_selectionWidget->flashAuxData( QString("Block Name"),
                                       blockName,
                                       QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT,
                                       display);
    }
}

/**
 * Called when an action needs to communicate 'message' to the user.
 */
void QG_DialogFactory::commandMessage(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::commandMessage");
    if (m_commandWidget) {
        m_commandWidget->appendHistory(message);
    }
    RS_DEBUG->print("QG_DialogFactory::commandMessage: OK");

}
void QG_DialogFactory::command(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::command");
    if (m_commandWidget) {
        m_commandWidget->setInput(message);
    }
    RS_DEBUG->print("QG_DialogFactory::command: OK");
}

void QG_DialogFactory::commandPrompt(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::command");
    if (m_commandWidget) {
        m_commandWidget->setCommand(message);
    }
    RS_DEBUG->print("QG_DialogFactory::command: OK");
}


/**
 * Converts an extension to a format description.
 * e.g. "PNG" to "Portable Network Graphic"
 *
 * @param Extension
 * @return Format description
 */
QString QG_DialogFactory::extToFormat(const QString& ext) {
    QString e = ext.toLower();

    if (e=="bmp") {
        return QObject::tr("Windows Bitmap");
    } else if (e=="jpeg" || e=="jpg") {
        return QObject::tr("Joint Photographic Experts Group");
    } else if (e=="gif") {
        return QObject::tr("Graphics Interchange Format");
    } else if (e=="mng") {
        return QObject::tr("Multiple-image Network Graphics");
    } else if (e=="pbm") {
        return QObject::tr("Portable Bit Map");
    } else if (e=="pgm") {
        return QObject::tr("Portable Grey Map");
    } else if (e=="png") {
        return QObject::tr("Portable Network Graphic");
    } else if (e=="ppm") {
        return QObject::tr("Portable Pixel Map");
    } else if (e=="xbm") {
        return QObject::tr("X Bitmap Format");
    } else if (e=="xpm") {
        return QObject::tr("X Pixel Map");
    } else if (e=="svg") {
        return QObject::tr("Scalable Vector Graphics");
    } else if (e=="bw") {
        return QObject::tr("SGI Black & White");
    } else if (e=="eps") {
        return QObject::tr("Encapsulated PostScript");
    } else if (e=="epsf") {
        return QObject::tr("Encapsulated PostScript Format");
    } else if (e=="epsi") {
        return QObject::tr("Encapsulated PostScript Interchange");
    } else if (e=="ico") {
        return QObject::tr("Windows Icon");
    } else if (e=="jp2") {
        return QObject::tr("JPEG 2000");
    } else if (e=="pcx") {
        return QObject::tr("ZSoft Paintbrush");
    } else if (e=="pic") {
        return QObject::tr("PC Paint");
    } else if (e=="rgb" || e=="rgba" || e=="sgi") {
        return QObject::tr("SGI-Bilddatei");
    } else if (e=="tga") {
        return QObject::tr("Targa Image File");
    } else if (e=="tif" || e=="tiff") {
        return QObject::tr("Tagged Image File Format");
    }
    else {
        return ext.toUpper();
    }
}
