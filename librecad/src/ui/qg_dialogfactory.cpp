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

#include <QMessageBox>
#include <QFileDialog>
#include <QImageReader>
#include <QString>
#include <QFileDialog>
#include <QToolBar>
#include <QRegularExpression>

#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_actioninterface.h"
#include "rs_document.h"
#include "rs_hatch.h"
#include "lc_splinepoints.h"
#include "rs_dimlinear.h"

#include "rs_actiondimlinear.h"

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
#include "lc_dlgsplinepoints.h"
#include "qg_dlgtext.h"
#include "qg_imageoptions.h"
#include "qg_insertoptions.h"
#include "qg_layerdialog.h"
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
#include "qg_printpreviewoptions.h"
#include "qg_roundoptions.h"
#include "qg_selectionwidget.h"
#include "qg_snapdistoptions.h"
#include "qg_snapmiddleoptions.h"
#include "qg_splineoptions.h"
#include "qg_textoptions.h"
#include "qg_trimamountoptions.h"
#include "qg_polylineoptions.h"
#include "qg_polylineequidistantoptions.h"
#include "qg_layerwidget.h"
#include "rs_actionprintpreview.h"
#include "rs_blocklist.h"
#include "qg_polylineequidistantoptions.h"
#include "qg_snapmiddleoptions.h"
#include "qg_snapdistoptions.h"
#include "rs_vector.h"
#include "rs_debug.h"

//QG_DialogFactory* QG_DialogFactory::uniqueInstance = nullptr;

/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param ow Pointer to widget that can host option widgets.
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, QToolBar* ow)
        : RS_DialogFactoryInterface()
		,parent(parent)
		,m_pLineAngleOptions(nullptr)
{
        RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory");

    setOptionWidget(ow);

	coordinateWidget = nullptr;
	mouseWidget = nullptr;
	selectionWidget = nullptr;
	commandWidget = nullptr;
	polylineEquidistantOptions=nullptr;
	snapMiddleOptions=nullptr;
	snapDistOptions=nullptr;
	modifyOffsetOptions=nullptr;
	printPreviewOptions=nullptr;
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
            delete printPreviewOptions;
			printPreviewOptions = nullptr;
        }
        return;
    }
	if (optionWidget ) {
		if (!printPreviewOptions) {
            printPreviewOptions = new QG_PrintPreviewOptions();
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

	if (optionWidget) {
		static QG_LineOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LineOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action);
            toolWidget->show();
        }
    }
        RS_DEBUG->print("QG_DialogFactory::requestLineOptions: OK");
}


/**
 * Shows a widget for options for the action: "draw polyline"
 */
void QG_DialogFactory::requestPolylineOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_PolylineOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_PolylineOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}

/**
 * Shows a widget for options for the action: "draw equidistant polyline"
 */
void QG_DialogFactory::requestPolylineEquidistantOptions(RS_ActionInterface* action,
                                                         bool on, bool /*update*/ ) {

    if(!on) {
		if (polylineEquidistantOptions) {
            delete polylineEquidistantOptions;
			polylineEquidistantOptions = nullptr;
        }
        return;
    }
	if (optionWidget ) {
		if (!polylineEquidistantOptions) {
            polylineEquidistantOptions = new QG_PolylineEquidistantOptions();

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

	if (optionWidget) {
		static QG_LineParallelOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
		   toolWidget = new QG_LineParallelOptions(optionWidget);
           optionWidget->addWidget(toolWidget);
           toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "draw line parallel through"
 */
void QG_DialogFactory::requestLineParallelThroughOptions(
    RS_ActionInterface* action,
    bool on, bool update) {

	if (optionWidget) {
		static QG_LineParallelThroughOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LineParallelThroughOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}


/**
 * Shows a widget for options for the action: "line angle"
 */
void QG_DialogFactory::requestLineAngleOptions(RS_ActionInterface* action,
                                               bool on, bool update) {

	if (optionWidget) {
		if (on) {
			if(!m_pLineAngleOptions)
                m_pLineAngleOptions = new QG_LineAngleOptions();
            optionWidget->addWidget(m_pLineAngleOptions);
            m_pLineAngleOptions->setAction(action, update);
            //toolWidget->setData(&angle, &length, fixedAngle, update);
            m_pLineAngleOptions->show();
        }else{
			if (!m_pLineAngleOptions) return;
            delete m_pLineAngleOptions;
			m_pLineAngleOptions = nullptr;
        }
    }
}



/**
 * Shows a widget for options for the action: "line relative angle"
 */
void QG_DialogFactory::requestLineRelAngleOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_LineRelAngleOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LineRelAngleOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            //toolWidget->setData(&angle, &length, fixedAngle, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "line angle"
 */
void QG_DialogFactory::requestLineBisectorOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_LineBisectorOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LineBisectorOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "draw polygon"
 */
void QG_DialogFactory::requestLinePolygonOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_LinePolygonOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LinePolygonOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "draw polygon2"
 */
void QG_DialogFactory::requestLinePolygon2Options(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_LinePolygon2Options* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LinePolygon2Options(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestArcOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_ArcOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_ArcOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            //toolWidget->setData(&data);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for tangential arc options.
 */
void QG_DialogFactory::requestArcTangentialOptions(RS_ActionInterface* action,
		bool on, bool /*update*/) {


	if (optionWidget) {
		static QG_ArcTangentialOptions* toolWidget = nullptr;
		if (toolWidget && !on) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			bool useUpdate = toolWidget;
			if (!toolWidget) {
				toolWidget = new QG_ArcTangentialOptions(optionWidget);
				optionWidget->addWidget(toolWidget);
			}
			toolWidget->setAction(action, useUpdate);
			toolWidget->show();
        }
		arcTangentialOptions=toolWidget;
    }
}

void QG_DialogFactory::updateArcTangentialOptions(const double& d, bool byRadius)
{
	if (!arcTangentialOptions) return;
	if (byRadius){
        arcTangentialOptions->updateAngle(QString::number(d,'g',5));
    }else{
        arcTangentialOptions->updateRadius(QString::number(d,'g',5));
    }
}



/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestCircleOptions(RS_ActionInterface* action,
                                            bool on, bool update) {

	if (optionWidget) {
		static QG_CircleOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_CircleOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}


/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestCircleTan2Options(RS_ActionInterface* action,
                                            bool on, bool update) {

	if (optionWidget) {
		static QG_CircleTan2Options* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_CircleTan2Options(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}


/**
 * Shows a widget for spline options.
 */
void QG_DialogFactory::requestSplineOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_SplineOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_SplineOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for multi-line text options.
 */
void QG_DialogFactory::requestMTextOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_MTextOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_MTextOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}


/**
 * Shows a widget for text options.
 */
void QG_DialogFactory::requestTextOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_TextOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_TextOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}


/**
 * Shows a widget for insert options.
 */
void QG_DialogFactory::requestInsertOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_InsertOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_InsertOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for image options.
 */
void QG_DialogFactory::requestImageOptions(RS_ActionInterface* action,
        bool on, bool update) {


	if (optionWidget) {
		static QG_ImageOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_ImageOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for dimension options.
 */
void QG_DialogFactory::requestDimensionOptions(RS_ActionInterface* action,
        bool on, bool update) {
	//static QLabel* l = nullptr;

	if (optionWidget) {
		static QG_DimOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_DimOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for linear dimension options.
 */
void QG_DialogFactory::requestDimLinearOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_DimLinearOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_DimLinearOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}

/**
 * Shows a widget for 'snap to equidistant middle points ' options.
 */
void QG_DialogFactory::requestSnapMiddleOptions(int& middlePoints, bool on) {

    if(!on) {
		if (snapMiddleOptions) {
            delete snapMiddleOptions;
			snapMiddleOptions = nullptr;
        }
        return;
    }
	if (optionWidget ) {
		if (!snapMiddleOptions) {
            snapMiddleOptions = new QG_SnapMiddleOptions(middlePoints);
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
            delete snapDistOptions;
			snapDistOptions = nullptr;
        }
        return;
    }
	if (optionWidget ) {
		if (!snapDistOptions) {
            snapDistOptions = new QG_SnapDistOptions();
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

	if (optionWidget) {
		static QG_MoveRotateOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_MoveRotateOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for 'trim amount' options.
 */
void QG_DialogFactory::requestTrimAmountOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_TrimAmountOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_TrimAmountOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
}



/**
 * Shows a widget for beveling options.
 */
void QG_DialogFactory::requestBevelOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_BevelOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_BevelOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}



/**
 * Shows a widget for rounding options.
 */
void QG_DialogFactory::requestRoundOptions(RS_ActionInterface* action,
        bool on, bool update) {

	if (optionWidget) {
		static QG_RoundOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_RoundOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}


/**
 * Shows a widget for offset options.
 */
void QG_DialogFactory::requestModifyOffsetOptions(double& dist, bool on) {
    if(!on) {
		if (modifyOffsetOptions) {
            delete modifyOffsetOptions;
			modifyOffsetOptions = nullptr;
        }
        return;
    }
	if (optionWidget ) {
		if (!modifyOffsetOptions) {
            modifyOffsetOptions = new QG_ModifyOffsetOptions();
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

	if (optionWidget) {
		static QG_LibraryInsertOptions* toolWidget = nullptr;
		if (toolWidget) {
            delete toolWidget;
			toolWidget = nullptr;
        }
		if (on) {
			toolWidget = new QG_LibraryInsertOptions(optionWidget);
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
                        toolWidget->show();
        }
    }
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
    case RS2::EntityDimRadial: {
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


