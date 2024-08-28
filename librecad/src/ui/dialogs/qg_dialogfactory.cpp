/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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
#include <QString>
#include <QRegularExpression>
#include <QToolBar>

#include "LC_DlgParabola.h"
#include "lc_dlgsplinepoints.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "qc_applicationwindow.h"
#include "qg_blockdialog.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
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
#include "qg_dlgoptionsgeneral.h"
#include "qg_dlgoptionsmakercam.h"
#include "qg_dlgpoint.h"
#include "qg_dlgpolyline.h"
#include "qg_dlgrotate.h"
#include "qg_dlgrotate2.h"
#include "qg_dlgscale.h"
#include "qg_dlgspline.h"
#include "qg_dlgtext.h"
#include "qg_layerdialog.h"
#include "qg_layerwidget.h"
#include "qg_mousewidget.h"
#include "qg_selectionwidget.h"
#include "qg_snapmiddleoptions.h"
#include "rs_actioninterface.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dimlinear.h"
#include "rs_hatch.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_vector.h"
#include "lc_crossoptions.h"
#include "lc_lineanglereloptions.h"
#include "lc_rectangle1pointoptions.h"
#include "lc_optionswidgetsholder.h"
#include "lc_actionsshortcutsdialog.h"

//QG_DialogFactory* QG_DialogFactory::uniqueInstance = nullptr;

/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param ow Pointer to widget that can host option widgets.
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, QToolBar* optionsToolbar, LC_SnapOptionsWidgetsHolder* snapOptionsHolder)
    : parent(parent)
{
    RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory");
    snapOptionsWidgetHolderSnapToolbar  = snapOptionsHolder;
    setOptionWidget(optionsToolbar);
    RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory: OK");
}


/**
 * Destructor
 */
QG_DialogFactory::~QG_DialogFactory() {
    RS_DEBUG->print("QG_DialogFactory::~QG_DialogFactory");
    RS_DEBUG->print("QG_DialogFactory::~QG_DialogFactory: OK");
}

void QG_DialogFactory::setOptionWidget(QToolBar* ow) {
    RS_DEBUG->print("QG_DialogFactory::setOptionWidget");
    optionWidget = ow;
    optionWidgetHolder = new LC_OptionsWidgetsHolder(ow);
    optionWidget->addWidget(optionWidgetHolder);
    snapOptionsWidgetHolderOptionsToolbar = optionWidgetHolder->getSnapOptionsHolder();
    RS_DEBUG->print("QG_DialogFactory::setOptionWidget: OK");
}

void QG_DialogFactory::addOptionsWidget(QWidget * options){
    optionWidgetHolder->addOptionsWidget(options);
    optionWidget->update();
}

void QG_DialogFactory::removeOptionsWidget(QWidget * options){
    optionWidgetHolder->removeOptionsWidget(options);

}
void QG_DialogFactory::hideSnapOptions(){
    getSnapOptionsHolder()->hideSnapOptions();
}

LC_SnapOptionsWidgetsHolder* QG_DialogFactory::getSnapOptionsHolder(){
    LC_SnapOptionsWidgetsHolder* result = nullptr;
    bool useSnapToolbar = LC_GET_ONE_BOOL("Appearance", "showSnapOptionsInSnapToolbar", false);
    if (useSnapToolbar){
        result = snapOptionsWidgetHolderSnapToolbar;
    }
    else{
        result = snapOptionsWidgetHolderOptionsToolbar;
    }
    if (lastUsedSnapOptionsWidgetHolder != nullptr && lastUsedSnapOptionsWidgetHolder != result){
        result->updateBy(lastUsedSnapOptionsWidgetHolder);
    }
    lastUsedSnapOptionsWidgetHolder = result;

    return result;
}

/**
 * Shows a widget for 'snap to equidistant middle points ' options.
 */
void QG_DialogFactory::requestSnapMiddleOptions(int* middlePoints, bool on) {
    getSnapOptionsHolder()->showSnapMiddleOptions(middlePoints, on);
}

/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void QG_DialogFactory::requestSnapDistOptions(double* dist, bool on) {
    getSnapOptionsHolder()->showSnapDistOptions(dist, on);
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
        QRegularExpressionMatch match( re.match(layer_name));
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
bool QG_DialogFactory::requestModifyEntityDialog(RS_Entity* entity) {
    if (!entity) return false;

    bool ret = false;

    switch (entity->rtti()) {
    case RS2::EntityPoint: {
        QG_DlgPoint dlg(parent);
        dlg.setPoint(*((RS_Point*)entity));
        if (dlg.exec()) {
            dlg.updatePoint();
            ret = true;
        }
    }
        break;

    case RS2::EntityLine: {
        QG_DlgLine dlg(parent);
        dlg.setLine(*((RS_Line*)entity));
        if (dlg.exec()) {
            dlg.updateLine();
            ret = true;
        }
    }
        break;

    case RS2::EntityArc: {
        QG_DlgArc dlg(parent);
        dlg.setArc(*((RS_Arc*)entity));
        if (dlg.exec()) {
            dlg.updateArc();
            ret = true;
        }
    }
        break;

    case RS2::EntityCircle: {
        QG_DlgCircle dlg(parent);
        dlg.setCircle(*((RS_Circle*)entity));
        if (dlg.exec()) {
            dlg.updateCircle();
            ret = true;
        }
    }
        break;

    case RS2::EntityEllipse: {
        QG_DlgEllipse dlg(parent);
        dlg.setEllipse(*((RS_Ellipse*)entity));
        if (dlg.exec()) {
            dlg.updateEllipse();
            ret = true;
        }
    }
        break;

    case RS2::EntityParabola: {
        LC_DlgParabola dlg;
        dlg.setParabola(*static_cast<LC_Parabola*>(entity));
        if (dlg.exec()) {
            dlg.updateParabola();
            ret = true;
        }
    }
        break;

    case RS2::EntitySpline: {
        QG_DlgSpline dlg;
        dlg.setSpline(*((RS_Spline*)entity));
        if (dlg.exec()) {
            dlg.updateSpline();
            ret = true;
        }
    }
        break;

    case RS2::EntitySplinePoints: {
        LC_DlgSplinePoints dlg;
        dlg.setSpline(*static_cast<LC_SplinePoints*>(entity));
        if (dlg.exec()) {
            dlg.updateSpline();
            ret = true;
        }
    }
        break;

    case RS2::EntityInsert: {
        QG_DlgInsert dlg;
        dlg.setInsert(*((RS_Insert*)entity));
        if (dlg.exec()) {
            dlg.updateInsert();
            ret = true;
            entity->update();
        }
    }
        break;

    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimDiametric:
    case RS2::EntityDimRadial:
    case RS2::EntityDimArc: {
        QG_DlgDimension dlg(parent);
        dlg.setDim(*((RS_Dimension*)entity));
        if (dlg.exec()) {
            dlg.updateDim();
            ret = true;
            ((RS_Dimension*)entity)->updateDim(true);
        }
    }
        break;

    case RS2::EntityDimLinear: {
        QG_DlgDimLinear dlg(parent);
        dlg.setDim(*((RS_DimLinear*)entity));
        if (dlg.exec()) {
            dlg.updateDim();
            ret = true;
            ((RS_DimLinear*)entity)->updateDim(true);
        }
    }
        break;

    case RS2::EntityMText: {
        QG_DlgMText dlg(parent);
        dlg.setText(*((RS_MText*)entity), false);
        if (dlg.exec()) {
            dlg.updateText();
            ret = true;
            ((RS_MText*)entity)->update();
        }
    }
        break;

    case RS2::EntityText: {
        QG_DlgText dlg(parent);
        dlg.setText(*((RS_Text*)entity), false);
        if (dlg.exec()) {
            dlg.updateText();
            ret = true;
            ((RS_Text*)entity)->update();
        }
    }
        break;

    case RS2::EntityHatch: {
        QG_DlgHatch dlg(parent);
        dlg.setHatch(*((RS_Hatch*)entity), false);
        if (dlg.exec()) {
            dlg.updateHatch();
            ret = true;
            ((RS_Hatch*)entity)->update();
        }
    }
        break;

    case RS2::EntityPolyline: {
        QG_DlgPolyline dlg(parent);
        dlg.setPolyline(*((RS_Polyline*)entity));
        if (dlg.exec()) {
            dlg.updatePolyline();
            ret = true;
        }
    }
        break;

    case RS2::EntityImage: {
        QG_DlgImage dlg(parent);
        dlg.setImage(*((RS_Image*)entity));
        if (dlg.exec()) {
            dlg.updateImage();
            ret = true;
        }
    }
        break;

    default:
        break;
    }

    return ret;
}

/**
 * Shows a dialog to edit the attributes of the given dimension entity.
 */
/*
bool QG_DialogFactory::requestDimAlignedDialog(RS_DimAligned* dim) {
    if (dim==nullptr) {
        return false;
    }

    QG_DlgDimAligned dlg(parent);
    dlg.setDim(*dim, true);
    if (dlg.exec()) {
        dlg.updateDim();
        return true;
    }

    return false;
}
*/



/**
 * Shows a dialog to edit the attributes of the given multi-line text entity.
 */
bool QG_DialogFactory::requestMTextDialog(RS_MText* text) {
    if (!text) return false;

    QG_DlgMText dlg(parent);
    dlg.setText(*text, true);
    if (dlg.exec()) {
        dlg.updateText();
        return true;
    }

    return false;
}


/**
 * Shows a dialog to edit the attributes of the given text entity.
 */
bool QG_DialogFactory::requestTextDialog(RS_Text* text) {
    if (!text) return false;

    QG_DlgText dlg(parent);
    dlg.setText(*text, true);
    if (dlg.exec()) {
        dlg.updateText();
        return true;
    }

    return false;
}


/**
 * Shows a dialog to edit pattern / hatch attributes of the given entity.
 */
bool QG_DialogFactory::requestHatchDialog(RS_Hatch* hatch) {
    if (!hatch) return false;

    RS_PATTERNLIST->init();

    QG_DlgHatch dlg(parent);
    dlg.setHatch(*hatch, true);
    if (dlg.exec()) {
        dlg.updateHatch();
        dlg.saveSettings();
        return true;
    }
    return false;
}

/**
 * Shows dialog for general application options.
 */
int QG_DialogFactory::requestOptionsGeneralDialog() {
    QG_DlgOptionsGeneral dlg(parent);
    int result = dlg.exec();
    getSnapOptionsHolder(); // as side effect, should update location of snap options
    return result;
}

void QG_DialogFactory::requestKeyboardShortcutsDialog(LC_ActionGroupManager *pManager) {
    LC_ActionsShortcutsDialog dlg(parent, pManager);
    dlg.exec();
}


/**
 * Shows dialog for drawing options.
 */
int QG_DialogFactory::requestOptionsDrawingDialog(RS_Graphic& graphic) {
    QG_DlgOptionsDrawing dlg(parent);
    dlg.setGraphic(&graphic);
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
 * Called whenever the mouse position changed.
 */
void QG_DialogFactory::updateCoordinateWidget(const RS_Vector& abs,
                                              const RS_Vector& rel, bool updateFormat) {
    if (coordinateWidget) {
        coordinateWidget->setCoordinates(abs, rel, updateFormat);
    }
}

void QG_DialogFactory::updateMouseWidget(const QString& left,
                                         const QString& right,
                                         const LC_ModifiersInfo& modifiers) {
    if (mouseWidget) {
        mouseWidget->setHelp(left, right, modifiers);
    }
    if (commandWidget) {
        commandWidget->setCommand(left);
    }
}

/**
 * Called whenever the selection changed.
 */
void QG_DialogFactory::updateSelectionWidget(int num, double length) {
    if (selectionWidget) {
        selectionWidget->setNumber(num);
        selectionWidget->setTotalLength(length);
    }
}



void QG_DialogFactory::displayBlockName(const QString& blockName, const bool& display)
{
    if (selectionWidget)
    {
        selectionWidget->flashAuxData( QString("Block Name"),
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
    if (commandWidget) {
        commandWidget->appendHistory(message);
    }
    RS_DEBUG->print("QG_DialogFactory::commandMessage: OK");

}
void QG_DialogFactory::command(const QString& message) {
    RS_DEBUG->print("QG_DialogFactory::command");
    if (commandWidget) {
        commandWidget->setInput(message);
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
