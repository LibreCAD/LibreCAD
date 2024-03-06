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
#include "qg_arcoptions.h"
#include "qg_arctangentialoptions.h"
#include "qg_beveloptions.h"
#include "qg_blockdialog.h"
#include "qg_circleoptions.h"
#include "qg_circletan2options.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_dimlinearoptions.h"
#include "qg_dimoptions.h"
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
#include "qg_imageoptions.h"
#include "qg_insertoptions.h"
#include "qg_layerdialog.h"
#include "qg_layerwidget.h"
#include "qg_libraryinsertoptions.h"
#include "qg_lineangleoptions.h"
#include "qg_linebisectoroptions.h"
#include "qg_lineoptions.h"
#include "qg_lineparalleloptions.h"
#include "qg_lineparallelthroughoptions.h"
#include "qg_linepolygon2options.h"
#include "qg_linepolygonoptions.h"
#include "qg_linerelangleoptions.h"
#include "qg_modifyoffsetoptions.h"
#include "qg_mousewidget.h"
#include "qg_moverotateoptions.h"
#include "qg_mtextoptions.h"
#include "qg_polylineequidistantoptions.h"
#include "qg_polylineequidistantoptions.h"
#include "qg_polylineoptions.h"
#include "qg_printpreviewoptions.h"
#include "qg_roundoptions.h"
#include "qg_selectionwidget.h"
#include "qg_snapdistoptions.h"
#include "qg_snapdistoptions.h"
#include "qg_snapmiddleoptions.h"
#include "qg_snapmiddleoptions.h"
#include "qg_splineoptions.h"
#include "qg_textoptions.h"
#include "qg_trimamountoptions.h"
#include "rs_actiondimlinear.h"
#include "rs_actioninterface.h"
#include "rs_actionprintpreview.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dimlinear.h"
#include "rs_document.h"
#include "rs_hatch.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_vector.h"
#include "lc_crossoptions.h"
#include "lc_lineoptions.h"
#include "lc_lineanglereloptions.h"
#include "lc_slicedivideoptions.h"
#include "lc_rectangle1pointoptions.h"

namespace {

template<typename T>
void addOptionWidget(QToolBar* toolbar, RS_ActionInterface* action, bool on)
{
    LC_LOG<<__func__<<"(): begin";
    if (toolbar) {
        static std::unique_ptr<T> option;
        if (option != nullptr) {
            option->hide();
            option->deleteLater();
            option.release();
        }
        if (on) {
            option = std::make_unique<T>(toolbar);
            toolbar->addWidget(option.get());
// looks like it's a bug - why action is set twice?
//            option->setAction(action);
            option->setAction(action);
            option->show();
        }
    }
    LC_LOG<<__func__<<"(): end";
}

template<typename T>
T* addOptionWidget(QToolBar* toolbar, RS_ActionInterface* action, bool on, bool update)
{
    LC_LOG<<__func__<<"(): begin";
    if (toolbar) {
        static std::unique_ptr<T> option;
        if (option != nullptr) {
            option->hide();
            option->deleteLater();
            option.release();
        }
        if (on) {
            option = std::make_unique<T>(toolbar);
            toolbar->addWidget(option.get());
            option->setAction(action, update);
            option->show();
        }
        return option.get();
    }
    LC_LOG<<__func__<<"(): end";
    return nullptr;
}
}

//QG_DialogFactory* QG_DialogFactory::uniqueInstance = nullptr;

/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param ow Pointer to widget that can host option widgets.
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, QToolBar* ow)
    : parent(parent)
{
    RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory");

    setOptionWidget(ow);
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
    RS_DEBUG->print("QG_DialogFactory::setOptionWidget: OK");
}

void QG_DialogFactory::addOptionsWidget(QWidget * options){
    optionWidget->addWidget(options);
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

    QG_BlockDialog dlg(parent, "Rename Block");
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

    QString title(
                QMessageBox::tr("Remove %n block(s)", "", blocks.size())
                );

    QString text(
                QMessageBox::tr("Listed blocks and all their entities will be removed.")
                );

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
QString QG_DialogFactory::requestImageOpenDialog()
{
    QString strFileName = "";

    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    QString defDir = RS_SETTINGS->readEntry("/OpenImage", RS_SYSTEM->getHomeDir());
    QString defFilter = RS_SETTINGS->readEntry( "/ImageFilter", "");
    RS_SETTINGS->endGroup();

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
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/OpenImage", QFileInfo(strFileName).absolutePath());
        RS_SETTINGS->writeEntry("/ImageFilter", fileDlg.selectedNameFilter());
        RS_SETTINGS->endGroup();
    }

    return strFileName;
}



void QG_DialogFactory::requestOptions(RS_ActionInterface* action,
                                      bool on, bool update) {
    RS_DEBUG->print("QG_DialogFactory::requestOptions");

    if (!action) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_DialogFactory::requestOptions: action is nullptr");
        return;
    }

    //	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DialogFactory::requestOptions, action %s, on %s, update %s",qPrintable(action->getName()),on?"TRUE":"FALSE",update?"TRUE":"FALSE");

    switch (action->rtti()) {
    case RS2::ActionFilePrintPreview:
        requestPrintPreviewOptions(action, on, update);
        break;

    case RS2::ActionDrawLine:
        RS_DEBUG->print("QG_DialogFactory::requestOptions: line");
        requestLineOptions(action, on);
        RS_DEBUG->print("QG_DialogFactory::requestOptions: line: OK");
        break;

    case RS2::ActionDrawPolyline:
        requestPolylineOptions(action, on, update);
        break;

    case RS2::ActionDrawLineAngle:
    case RS2::ActionDrawLineHorizontal:
    case RS2::ActionDrawLineVertical:
        requestLineAngleOptions(action, on, update);
        break;

    case RS2::ActionDrawLineParallel:
        requestLineParallelOptions(action, on, update);
        break;

    case RS2::ActionDrawLineParallelThrough:
        requestLineParallelThroughOptions(action, on, update);
        break;

    case RS2::ActionDrawLineBisector:
        requestLineBisectorOptions(action, on, update);
        break;

    case RS2::ActionDrawLineOrthogonal:
    case RS2::ActionDrawLineRelAngle:
        requestLineRelAngleOptions(action, on, update);
        break;

    case RS2::ActionDrawLinePolygonCenCor:
        requestLinePolygonOptions(action, on, update);
        break;

    case RS2::ActionDrawLinePolygonCorCor:
        requestLinePolygon2Options(action, on, update);
        break;

    case RS2::ActionDrawArc:
        requestArcOptions(action, on, update);
        break;

    case RS2::ActionDrawArcTangential:
        requestArcTangentialOptions(action, on, update);
        break;

    case RS2::ActionDrawCircleCR:
    case RS2::ActionDrawCircle2PR:
        requestCircleOptions(action, on, update);
        break;

    case RS2::ActionDrawCircleTan2:
        requestCircleTan2Options(action, on, update);
        break;

    case RS2::ActionDrawSpline:
    case RS2::ActionDrawSplinePoints:
        requestSplineOptions(action, on, update);
        break;

    case RS2::ActionDrawMText:
        requestMTextOptions(action, on, update);
        break;

    case RS2::ActionDrawText:
        requestTextOptions(action, on, update);
        break;

    case RS2::ActionBlocksInsert:
        requestInsertOptions(action, on, update);
        break;

    case RS2::ActionDrawImage:
        requestImageOptions(action, on, update);
        break;

    case RS2::ActionDimAligned:
        requestDimensionOptions(action, on, update);
        break;

    case RS2::ActionDimLinear:
    case RS2::ActionDimLinearVer:
    case RS2::ActionDimLinearHor:
        requestDimensionOptions(action, on, update);
        if (!((RS_ActionDimLinear*)action)->hasFixedAngle()) {
            requestDimLinearOptions(action, on, update);
        }
        break;

    case RS2::ActionDimRadial:
        requestDimensionOptions(action, on, update);
        break;

    case RS2::ActionDimDiametric:
        requestDimensionOptions(action, on, update);
        break;

    case RS2::ActionDimAngular:
        requestDimensionOptions(action, on, update);
        break;

    case RS2::ActionDimArc:
        requestDimensionOptions(action, on, update);
        break;

    case RS2::ActionModifyTrimAmount:
        requestTrimAmountOptions(action, on, update);
        break;

    case RS2::ActionModifyMoveRotate:
        requestMoveRotateOptions(action, on, update);
        break;

    case RS2::ActionModifyBevel:
        requestBevelOptions(action, on, update);
        break;

    case RS2::ActionModifyRound:
        requestRoundOptions(action, on, update);
        break;

    case RS2::ActionLibraryInsert:
        requestLibraryInsertOptions(action, on, update);
        break;
    case RS2::ActionPolylineEquidistant:
        requestPolylineEquidistantOptions(action, on, update);
        break;

    case RS2::ActionDrawCross:
        requestCrossOptions(action, on, update);
        break;

     case RS2::ActionDrawLineAngleRel:
     case RS2::ActionDrawLineOrthogonalRel:
         requestLineAngleRelOptions(action, on, update);
         break;

     case RS2::ActionDrawRectangle1Point:
         requestLineRectangleFixedOptions(action, on, update);
        break;

     case RS2::ActionDrawLineRel:
        requestLineRelOptions(action, on, update);
        break;

     case RS2::ActionDrawSliceDivide:
         requestSliceDivideOptions(action, on, update);
         break;


    default:
        break;
    }
    RS_DEBUG->print("QG_DialogFactory::requestOptions: OK");
}



/**
 * Shows a widget for options for the action: "print preview"
 */
void QG_DialogFactory::requestPrintPreviewOptions(RS_ActionInterface* action,
                                                  bool on, bool update) {

    if(!on) {
        if (printPreviewOptions) {
            printPreviewOptions->hide();
            printPreviewOptions->deleteLater();
            printPreviewOptions=nullptr;
        }
        return;
    }
    if (optionWidget ) {
        if (!printPreviewOptions) {
            printPreviewOptions = new QG_PrintPreviewOptions(optionWidget);
            printPreviewOptions ->setAction(action, false);
            optionWidget->addWidget(printPreviewOptions);
        }
        if(update) printPreviewOptions ->setAction(action, update);
        printPreviewOptions->show();
    }

}


/**
 * Shows a widget for options for the action: "draw line"
 */
void QG_DialogFactory::requestLineOptions(RS_ActionInterface* action,
                                          bool on) {
    LC_LOG<<__func__<<"(): begin";
    addOptionWidget<QG_LineOptions>(optionWidget, action, on);
    LC_LOG<<__func__<<"(): end";
    RS_DEBUG->print("QG_DialogFactory::requestLineOptions: OK");
}


/**
 * Shows a widget for options for the action: "draw polyline"
 */
void QG_DialogFactory::requestPolylineOptions(RS_ActionInterface* action,
                                              bool on, bool update) {
    addOptionWidget<QG_PolylineOptions>(optionWidget, action, on, update);
}

/**
 * Shows a widget for options for the action: "draw equidistant polyline"
 */
void QG_DialogFactory::requestPolylineEquidistantOptions(RS_ActionInterface* action,
                                                         bool on, bool /*update*/ ) {

    if(!on) {
        if (polylineEquidistantOptions) {
            polylineEquidistantOptions->hide();
            polylineEquidistantOptions->deleteLater();
            polylineEquidistantOptions=nullptr;
        }
        return;
    }
    if (optionWidget ) {
        if (!polylineEquidistantOptions) {
            polylineEquidistantOptions = new QG_PolylineEquidistantOptions(optionWidget);
            optionWidget->addWidget(polylineEquidistantOptions);
            // settings from action
            polylineEquidistantOptions -> setAction(action, false);
        }else{
            // settings from widget using saved config
            polylineEquidistantOptions -> setAction(action, true);
        }
        polylineEquidistantOptions->show();
    }
}



/**
 * Shows a widget for options for the action: "draw line parallel"
 */
void QG_DialogFactory::requestLineParallelOptions(RS_ActionInterface* action,
                                                  bool on, bool update) {
    addOptionWidget<QG_LineParallelOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for options for the action: "draw line parallel through"
 */
void QG_DialogFactory::requestLineParallelThroughOptions(
        RS_ActionInterface* action,
        bool on, bool update) {
    addOptionWidget<QG_LineParallelThroughOptions>(optionWidget, action, on, update);
}


/**
 * Shows a widget for options for the action: "line angle"
 */
void QG_DialogFactory::requestLineAngleOptions(RS_ActionInterface* action,
                                               bool on, bool update) {

    if (optionWidget) {
        if (on) {
            if(!m_pLineAngleOptions)
                m_pLineAngleOptions = new QG_LineAngleOptions(optionWidget);
            optionWidget->addWidget(m_pLineAngleOptions);
            m_pLineAngleOptions->setAction(action, update);
            //toolWidget->setData(&angle, &length, fixedAngle, update);
            m_pLineAngleOptions->show();
        }else{
            if (!m_pLineAngleOptions) return;
            m_pLineAngleOptions->hide();
            m_pLineAngleOptions->deleteLater();
            m_pLineAngleOptions=nullptr;
        }
    }
}



/**
 * Shows a widget for options for the action: "line relative angle"
 */
void QG_DialogFactory::requestLineRelAngleOptions(RS_ActionInterface* action,
                                                  bool on, bool update) {
    addOptionWidget<QG_LineRelAngleOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for options for the action: "line angle"
 */
void QG_DialogFactory::requestLineBisectorOptions(RS_ActionInterface* action,
                                                  bool on, bool update) {
    addOptionWidget<QG_LineBisectorOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for options for the action: "draw polygon"
 */
void QG_DialogFactory::requestLinePolygonOptions(RS_ActionInterface* action,
                                                 bool on, bool update) {
    addOptionWidget<QG_LinePolygonOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for options for the action: "draw polygon2"
 */
void QG_DialogFactory::requestLinePolygon2Options(RS_ActionInterface* action,
                                                  bool on, bool update) {

    addOptionWidget<QG_LinePolygon2Options>(optionWidget, action, on, update);
}



/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestArcOptions(RS_ActionInterface* action,
                                         bool on, bool update) {

    addOptionWidget<QG_ArcOptions>(optionWidget, action, on, update);
}


void QG_DialogFactory::requestCrossOptions(RS_ActionInterface* action,
                                           bool on, bool update){
    addOptionWidget<LC_CrossOptions>(optionWidget, action, on, update);
}

void QG_DialogFactory::requestLineAngleRelOptions(RS_ActionInterface* action,
                                                  bool on, bool update){
    addOptionWidget<LC_LineAngleRelOptions>(optionWidget, action, on, update);
}

void QG_DialogFactory::requestLineRectangleFixedOptions(RS_ActionInterface* action,
                                                        bool on, bool update){
    addOptionWidget<LC_Rectangle1PointOptions>(optionWidget, action, on, update);
}

void QG_DialogFactory::requestLineRelOptions(RS_ActionInterface* action,
                                           bool on, bool update){
    addOptionWidget<LC_LineOptions>(optionWidget, action, on, update);
}

void QG_DialogFactory::requestSliceDivideOptions(RS_ActionInterface* action,
                                           bool on, bool update){
    addOptionWidget<LC_SliceDivideOptions>(optionWidget, action, on, update);
}


/**
 * Shows a widget for tangential arc options.
 */
void QG_DialogFactory::requestArcTangentialOptions(RS_ActionInterface* action,
                                                   bool on, bool update) {
    LC_ERR<<__func__<<"(): update from action="<<update;
    arcTangentialOptions = addOptionWidget<QG_ArcTangentialOptions>(optionWidget, action, on, update);
}

void QG_DialogFactory::updateArcTangentialOptions(double d, bool byRadius)
{
    if (arcTangentialOptions == nullptr) return;
    if (byRadius){
        arcTangentialOptions->updateRadius(QString::number(d,'g',5));
    }else{
        arcTangentialOptions->updateAngle(QString::number(d,'g',5));
    }
}



/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestCircleOptions(RS_ActionInterface* action,
                                            bool on, bool update) {

    addOptionWidget<QG_CircleOptions>(optionWidget, action, on, update);
    //	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DialogFactory::requestCircleOptions, action %s, on %s, update %s",qPrintable(action->getName()),on?"TRUE":"FALSE",update?"TRUE":"FALSE");
}


/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestCircleTan2Options(RS_ActionInterface* action,
                                                bool on, bool update) {
    addOptionWidget<QG_CircleTan2Options>(optionWidget, action, on, update);

}


/**
 * Shows a widget for spline options.
 */
void QG_DialogFactory::requestSplineOptions(RS_ActionInterface* action,
                                            bool on, bool update) {

    addOptionWidget<QG_SplineOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for multi-line text options.
 */
void QG_DialogFactory::requestMTextOptions(RS_ActionInterface* action,
                                           bool on, bool update) {


    addOptionWidget<QG_MTextOptions>(optionWidget, action, on, update);
}


/**
 * Shows a widget for text options.
 */
void QG_DialogFactory::requestTextOptions(RS_ActionInterface* action,
                                          bool on, bool update) {

    addOptionWidget<QG_TextOptions>(optionWidget, action, on, update);

}


/**
 * Shows a widget for insert options.
 */
void QG_DialogFactory::requestInsertOptions(RS_ActionInterface* action,
                                            bool on, bool update) {

    addOptionWidget<QG_InsertOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for image options.
 */
void QG_DialogFactory::requestImageOptions(RS_ActionInterface* action,
                                           bool on, bool update) {


    addOptionWidget<QG_ImageOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for dimension options.
 */
void QG_DialogFactory::requestDimensionOptions(RS_ActionInterface* action,
                                               bool on, bool update) {
    addOptionWidget<QG_DimOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for linear dimension options.
 */
void QG_DialogFactory::requestDimLinearOptions(RS_ActionInterface* action,
                                               bool on, bool update) {

    addOptionWidget<QG_DimLinearOptions>(optionWidget, action, on, update);
}

/**
 * Shows a widget for 'snap to equidistant middle points ' options.
 */
void QG_DialogFactory::requestSnapMiddleOptions(int& middlePoints, bool on) {

    if(!on) {
        if (snapMiddleOptions) {
            snapMiddleOptions->hide();
            snapMiddleOptions->deleteLater();
            snapMiddleOptions = nullptr;
        }
        return;
    }
    if (optionWidget ) {
        if (!snapMiddleOptions) {
            snapMiddleOptions = new QG_SnapMiddleOptions(middlePoints, optionWidget);
            optionWidget->addWidget(snapMiddleOptions);
            snapMiddleOptions->setMiddlePoints(middlePoints);
        }else{
            snapMiddleOptions->setMiddlePoints(middlePoints,false);
        }
        snapMiddleOptions->show();
    }
    //std::cout<<"QG_DialogFactory::requestSnapMiddleOptions(): middlePoints="<<middlePoints<<std::endl;
}


/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void QG_DialogFactory::requestSnapDistOptions(double& dist, bool on) {
    if(!on) {
        if (snapDistOptions) {
            snapDistOptions->hide();
            snapDistOptions->deleteLater();
            snapDistOptions = nullptr;
        }
        return;
    }
    if (optionWidget ) {
        if (!snapDistOptions) {
            snapDistOptions = new QG_SnapDistOptions(optionWidget);
            optionWidget->addWidget(snapDistOptions);
            snapDistOptions->setDist(dist);
        }else {
            snapDistOptions->setDist(dist,false);
        }
        //std::cout<<"QG_DialogFactory::requestSnapDistOptions(): dist="<<dist<<std::endl;
        snapDistOptions->show();
    }
}



/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void QG_DialogFactory::requestMoveRotateOptions(RS_ActionInterface* action,
                                                bool on, bool update) {
    addOptionWidget<QG_MoveRotateOptions>(optionWidget, action, on, update);

}



/**
 * Shows a widget for 'trim amount' options.
 */
void QG_DialogFactory::requestTrimAmountOptions(RS_ActionInterface* action,
                                                bool on, bool update) {
    addOptionWidget<QG_TrimAmountOptions>(optionWidget, action, on, update);
}



/**
 * Shows a widget for beveling options.
 */
void QG_DialogFactory::requestBevelOptions(RS_ActionInterface* action,
                                           bool on, bool update) {
    addOptionWidget<QG_BevelOptions>(optionWidget, action, on, update);

}



/**
 * Shows a widget for rounding options.
 */
void QG_DialogFactory::requestRoundOptions(RS_ActionInterface* action,
                                           bool on, bool update) {
    addOptionWidget<QG_RoundOptions>(optionWidget, action, on, update);
}


/**
 * Shows a widget for offset options.
 */
void QG_DialogFactory::requestModifyOffsetOptions(double& dist, bool on) {
    if(!on) {
        if (modifyOffsetOptions) {
            modifyOffsetOptions->hide();
            modifyOffsetOptions->deleteLater();
            modifyOffsetOptions = nullptr;
        }
        return;
    }
    if (optionWidget ) {
        if (!modifyOffsetOptions) {
            modifyOffsetOptions = new QG_ModifyOffsetOptions(optionWidget);
            optionWidget->addWidget(modifyOffsetOptions);
            modifyOffsetOptions->setDist(dist);
        }else {
            modifyOffsetOptions->setDist(dist,false);
        }
        //std::cout<<"QG_DialogFactory::requestSnapDistOptions(): dist="<<dist<<std::endl;
        modifyOffsetOptions->show();
    }
}



/**
 * Shows a widget for library insert options.
 */
void QG_DialogFactory::requestLibraryInsertOptions(RS_ActionInterface* action,
                                                   bool on, bool update) {

    addOptionWidget<QG_LibraryInsertOptions>(optionWidget, action, on, update);
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
        LC_DlgParabola dlg(nullptr, false);
        dlg.setParabola(*static_cast<LC_Parabola*>(entity));
        if (dlg.exec()) {
            dlg.updateParabola();
            ret = true;
        }
    }
        break;

    case RS2::EntitySpline: {
        QG_DlgSpline dlg(nullptr, false);
        dlg.setSpline(*((RS_Spline*)entity));
        if (dlg.exec()) {
            dlg.updateSpline();
            ret = true;
        }
    }
        break;

    case RS2::EntitySplinePoints: {
        LC_DlgSplinePoints dlg(nullptr, false);
        dlg.setSpline(*static_cast<LC_SplinePoints*>(entity));
        if (dlg.exec()) {
            dlg.updateSpline();
            ret = true;
        }
    }
        break;

    case RS2::EntityInsert: {
        QG_DlgInsert dlg(parent);
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
void QG_DialogFactory::requestOptionsGeneralDialog() {
    QG_DlgOptionsGeneral dlg(parent);
    dlg.exec();
}



/**
 * Shows dialog for drawing options.
 */
void QG_DialogFactory::requestOptionsDrawingDialog(RS_Graphic& graphic) {
    QG_DlgOptionsDrawing dlg(parent);
    dlg.setGraphic(&graphic);
    dlg.exec();
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
                                         const QString& right) {
    if (mouseWidget) {
        mouseWidget->setHelp(left, right);
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


