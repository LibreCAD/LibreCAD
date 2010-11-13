/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "qg_dialogfactory.h"

#include <qmessagebox.h>
#include <q3filedialog.h>
//Added by qt3to4:
#include <Q3StrList>
#include <QImageReader>

#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_actioninterface.h"
#include "rs_document.h"

#include "rs_actiondimlinear.h"

#include "qg_arcoptions.h"
#include "qg_arctangentialoptions.h"
#include "qg_beveloptions.h"
#include "qg_blockdialog.h"
#include "qg_cadtoolbar.h"
#include "qg_circleoptions.h"
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
#include "qg_dlginsert.h"
#include "qg_dlgline.h"
#include "qg_dlgmirror.h"
#include "qg_dlgmove.h"
#include "qg_dlgmoverotate.h"
#include "qg_dlgoptionsdrawing.h"
#include "qg_dlgoptionsgeneral.h"
#include "qg_dlgpoint.h"
#include "qg_dlgrotate.h"
#include "qg_dlgrotate2.h"
#include "qg_dlgscale.h"
#include "qg_dlgspline.h"
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
#include "qg_mousewidget.h"
#include "qg_moverotateoptions.h"
#include "qg_printpreviewoptions.h"
#include "qg_roundoptions.h"
#include "qg_selectionwidget.h"
#include "qg_snapdistoptions.h"
#include "qg_splineoptions.h"
#include "qg_textoptions.h"
#include "qg_trimamountoptions.h"

#include "qg_polylineoptions.h"

#ifdef RS_CAM
#include "rs_camdialog.h"
#endif

#ifdef RVT_CAM
#include "rvt_cammakeprofiledialog.h"
#endif

#include "qg_layerwidget.h"
#include "qg_mainwindowinterface.h"

//QG_DialogFactory* QG_DialogFactory::uniqueInstance = NULL;


/**
 * Constructor.
 *
 * @param parent Pointer to parent widget which can host dialogs.
 * @param ow Pointer to widget that can host option widgets.
 */
QG_DialogFactory::QG_DialogFactory(QWidget* parent, QToolBar* ow)
        : RS_DialogFactoryInterface() {
	RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory");

    this->parent = parent;
    setOptionWidget(ow);

    coordinateWidget = NULL;
    mouseWidget = NULL;
    selectionWidget = NULL;
    cadToolBar = NULL;
    commandWidget = NULL;
    mainWindow = NULL;
	RS_DEBUG->print("QG_DialogFactory::QG_DialogFactory: OK");
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
void QG_DialogFactory::requestWarningDialog(const RS_String& warning) {
    QMessageBox::information(parent, QMessageBox::tr("Warning"),
                             warning,
                             QMessageBox::Ok);
}



/**
 * Requests a new document from the main window.
 */
RS_GraphicView* QG_DialogFactory::requestNewDocument(const RS_String& fileName, RS_Document* doc) {
	if (mainWindow!=NULL) {
		mainWindow->createNewDocument(fileName, doc);
		return mainWindow->getGraphicView();
	}
	return NULL;
}


/**
 * Shows a dialog for adding a layer. Doesn't add the layer.
 * This is up to the caller.
 *
 * @return a pointer to the newly created layer that
 * should be added.
 */
RS_Layer* QG_DialogFactory::requestNewLayerDialog(RS_LayerList* layerList) {

    RS_Layer* layer = NULL;

    QString layer_name = "noname";
    int i = 2;

    if (layerList!=NULL) {
        while (layerList->find(layer_name) > 0)
            layer_name.sprintf("noname%d", i++);
    }

    // Layer for parameter livery
    layer = new RS_Layer(layer_name);

    QG_LayerDialog dlg(parent, "Layer Dialog");
    dlg.setLayer(layer);
    dlg.setLayerList(layerList);
    if (dlg.exec()) {
        dlg.updateLayer();
    } else {
        delete layer;
        layer = NULL;
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

    RS_Layer* layer = NULL;
    if (layerList==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestLayerRemovalDialog(): "
        	"layerList is NULL");
        return NULL;
    }
    /*
       if (layerList==NULL) {
           if (container!=NULL && container->rtti()==RS2::EntityGraphic) {
               layerList = (RS_LayerList*)container;
           } else {
               return NULL;
           }
       }*/

    // Layer for parameter livery
    layer = layerList->getActive();

    if (layer!=NULL) {
        if (layer->getName()!="0") {
            QMessageBox msgBox(
                    QMessageBox::Warning,
                    QMessageBox::tr("Remove Layer"),
                    QMessageBox::tr("Layer \"%1\" and all "
                                    "entities on it will be removed.")
                    .arg(layer->getName()),
                    QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec()==QMessageBox::Ok) {}
            else {
                layer = NULL;
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
 *         or NULL if the dialog was cancelled.
 */
RS_Layer* QG_DialogFactory::requestEditLayerDialog(RS_LayerList* layerList) {

    RS_DEBUG->print("QG_DialogFactory::requestEditLayerDialog");

    RS_Layer* layer = NULL;
    /*
       if (layerList==NULL) {
           if (container!=NULL && container->rtti()==RS2::EntityGraphic) {
               layerList = (RS_LayerList*)container;
           } else {
               return NULL;
           }
       }
    */

    if (layerList==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestEditLayerDialog(): "
        	"layerList is NULL");
        return NULL;
    }

    // Layer for parameter livery
    if (layerList->getActive()!=NULL) {
        layer = new RS_Layer(*layerList->getActive());

        QG_LayerDialog dlg(parent, QMessageBox::tr("Layer Dialog"));
        dlg.setLayer(layer);
        dlg.setLayerList(layerList);
        dlg.setEditLayer(TRUE);
        if (dlg.exec()) {
            dlg.updateLayer();
        } else {
            delete layer;
            layer = NULL;
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
    //RS_Block* block = NULL;
    RS_BlockData ret;
    ret = RS_BlockData("", RS_Vector(false), false);

    if (blockList==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestNewBlockDialog(): "
        	"blockList is NULL");
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
 * @return a pointer to the modified block or NULL on cancellation.
 */
RS_BlockData QG_DialogFactory::requestBlockAttributesDialog(RS_BlockList* blockList) {
    //RS_Block* block = NULL;
    RS_BlockData ret;
    ret = RS_BlockData("", RS_Vector(false), false);

    if (blockList==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestBlockAttributesDialog(): "
        	"blockList is NULL");
        return ret;
    }
    /*
       if (blockList==NULL) {
           if (container!=NULL && container->rtti()==RS2::EntityGraphic) {
               blockList = (RS_BlockList*)container;
           } else {
               return NULL;
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
    RS_Block* block = NULL;

    if (blockList==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestBlockRemovalDialog(): "
        	"blockList is NULL");
        return NULL;
    }

    // Block for parameter livery
    block = blockList->getActive();

    if (block!=NULL) {
        int remove =
            QMessageBox::warning(parent,
                                     QMessageBox::tr("Remove Block"),
                                     QMessageBox::tr("Block \"%1\" and all "
                                                     "its entities will be removed.")
                                     .arg(block->getName()),
                                     QMessageBox::Ok,
                                     QMessageBox::Cancel);
        if (remove==QMessageBox::Ok) {}
        else {
            block = NULL;
        }
    }

    return block;
}



/**
 * Shows a dialog for choosing a file name. Saving the file is up to
 * the caller.
 *
 * @return File name with path and extension to determine the file type
 *         or an empty string if the dialog was cancelled.
 */
/*
RS_String QG_DialogFactory::requestFileSaveAsDialog() {
    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    RS_String defDir = RS_SETTINGS->readEntry("/Save",
                       RS_SYSTEM->getHomeDir());
    RS_String defFilter = RS_SETTINGS->readEntry("/SaveFilter",
                          "Drawing Exchange (*.dxf)");
    RS_SETTINGS->endGroup();
 
    // prepare file save as dialog:
    QFileDialog fileDlg(this,0,true);
    QStringList filters;
    bool done = false;
    bool cancel = false;
    RS_String fn = "";
 
    filters.append("Drawing Exchange (*.dxf)");
    filters.append("Font (*.cxf)");
    fileDlg.setFilters(filters);
    fileDlg.setMode(QFileDialog::AnyFile);
    fileDlg.setCaption(tr("Save Drawing As"));
    fileDlg.setDir(defDir);
    fileDlg.setSelectedFilter(defFilter);
 
    // run dialog:
    do {
        // accepted:
        if (fileDlg.exec()==QDialog::Accepted) {
        	fn = fileDlg.selectedFile();
			cancel = false;
			
            // append default extension:
            if (fn.find('.')==-1) {
                if (fileDlg.selectedFilter()=="Font (*.cxf)") {
                    fn+=".cxf";
                } else {
                    fn+=".dxf";
                }
            }
 
            // overwrite warning:
            if(QFileInfo(fn).exists()) {
                int choice =
                    QMessageBox::warning(this, tr("Save Drawing As"),
                                         tr("%1 already exists.\n"
                                            "Do you want to replace it?")
                                         .arg(fn),
                                         tr("Yes"), tr("No"),
                                         tr("Cancel"), 0, 1 );
 
                switch (choice) {
                case 0:
                    done = true;
                    break;
                case 1:
                case 2:
                default:
                    done = false;
                    break;
                }
            } else {
                done = true;
            }
        }
		else {
            done = true;
            cancel = true;
			fn = "";
        }
    } while(!done);
 
    // store new default settings:
    if (!cancel) {
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/Save", QFileInfo(fn).dirPath(true));
        RS_SETTINGS->writeEntry("/SaveFilter", fileDlg.selectedFilter());
        RS_SETTINGS->endGroup();
    }
 
    return fn;
}
*/



/**
 * Shows a dialog for choosing a file name. Opening the file is up to
 * the caller.
 *
 * @return File name with path and extension to determine the file type
 *         or an empty string if the dialog was cancelled.
 */
RS_String QG_DialogFactory::requestImageOpenDialog() {
    RS_String fn = "";

    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    RS_String defDir = RS_SETTINGS->readEntry("/OpenImage",
                       RS_SYSTEM->getHomeDir());
    RS_String defFilter = RS_SETTINGS->readEntry("/ImageFilter",
                          "Portable Network Graphic (*.png)");
    RS_SETTINGS->endGroup();

    bool cancel = false;

    Q3FileDialog fileDlg(NULL, "", true);

    // RVT_PORT
    //Q3StrList f = QImageReader::supportedImageFormats();
    //QStringList formats = QStringList::fromStrList(f);
    QStringList filters;
    QString all = "";
    //filters = QStringList::fromStrList(formats);

    foreach (QByteArray format, QImageReader::supportedImageFormats()) {
		filters.append(format);
		/* RVT_PORT
		 QString ext = (*it);
        QString st;
        if (ext=="JPEG") {
            st = QString("%1 (*.%2 *.jpg)").arg(extToFormat(*it))
                 .arg(QString(*it).lower());
        } else {
            st = QString("%1 (*.%2)").arg(extToFormat(*it))
                 .arg(QString(*it).lower());
        }
        filters.append(st);

        if (!all.isEmpty()) {
            all += " ";
        }

        if (ext=="JPEG") {
            all += QString("*.%1 *.jpg").arg(QString(*it).lower());
        } else {
            all += QString("*.%1").arg(QString(*it).lower());
        } */
    }
    filters.append(QObject::tr("All Image Files (%1)").arg(all));
    filters.append(QObject::tr("All Files (*.*)"));

    //filters.append("Drawing Exchange (*.)");
    //filters.append("Font (*.cxf)");

    fileDlg.setFilters(filters);
    fileDlg.setMode(Q3FileDialog::ExistingFile);
    fileDlg.setCaption(QObject::tr("Open Image"));
    fileDlg.setDir(defDir);
    fileDlg.setSelectedFilter(defFilter);

    if (fileDlg.exec()==QDialog::Accepted) {
        fn = fileDlg.selectedFile();
        cancel = false;
    } else {
        cancel = true;
    }

    // store new default settings:
    if (!cancel) {
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/OpenImage", QFileInfo(fn).dirPath(true));
        RS_SETTINGS->writeEntry("/ImageFilter", fileDlg.selectedFilter());
        RS_SETTINGS->endGroup();
    }

    return fn;
}



void QG_DialogFactory::requestOptions(RS_ActionInterface* action,
                                      bool on, bool update) {
	RS_DEBUG->print("QG_DialogFactory::requestOptions");

    if (action==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_DialogFactory::requestOptions: action is NULL");
        return;
    }

    switch (action->rtti()) {
    case RS2::ActionPrintPreview:
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

    case RS2::ActionDrawLineRelAngle:
        requestLineRelAngleOptions(action, on, update);
        break;

    case RS2::ActionDrawLinePolygon:
        requestLinePolygonOptions(action, on, update);
        break;

    case RS2::ActionDrawLinePolygon2:
        requestLinePolygon2Options(action, on, update);
        break;

    case RS2::ActionDrawArc:
        requestArcOptions(action, on, update);
        break;
    
	case RS2::ActionDrawArcTangential:
        requestArcTangentialOptions(action, on, update);
        break;

    case RS2::ActionDrawCircleCR:
        requestCircleOptions(action, on, update);
        break;
		
    case RS2::ActionDrawSpline:
        requestSplineOptions(action, on, update);
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
        requestDimensionOptions(action, on, update);
        if (((RS_ActionDimLinear*)action)->hasFixedAngle()==false) {
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

    static QG_PrintPreviewOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_PrintPreviewOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
			toolWidget->show();
        }
    }
}


/**
 * Shows a widget for options for the action: "draw line"
 */
void QG_DialogFactory::requestLineOptions(RS_ActionInterface* action,
        bool on) {
    static QG_LineOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LineOptions();
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
		
    static QG_PolylineOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_PolylineOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "draw line parallel"
 */
void QG_DialogFactory::requestLineParallelOptions(RS_ActionInterface* action,
        bool on, bool update) {
    static QG_LineParallelOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
           toolWidget = new QG_LineParallelOptions();
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
    static QG_LineParallelThroughOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LineParallelThroughOptions();
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

    static QG_LineAngleOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LineAngleOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            //toolWidget->setData(&angle, &length, fixedAngle, update);
			toolWidget->show();
        }
    }
}



/**
 * Shows a widget for options for the action: "line relative angle"
 */
void QG_DialogFactory::requestLineRelAngleOptions(RS_ActionInterface* action,
        bool on, bool update) {

    static QG_LineRelAngleOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LineRelAngleOptions();
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

    static QG_LineBisectorOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LineBisectorOptions();
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
    static QG_LinePolygonOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LinePolygonOptions();
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
    static QG_LinePolygon2Options* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LinePolygon2Options();
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

    static QG_ArcOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_ArcOptions();
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
        bool on, bool update) {

    static QG_ArcTangentialOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_ArcTangentialOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            //toolWidget->setData(&data);
			toolWidget->show();
        }
    }
}



/**
 * Shows a widget for arc options.
 */
void QG_DialogFactory::requestCircleOptions(RS_ActionInterface* action,
        bool on, bool update) {
    static QG_CircleOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_CircleOptions();
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
    static QG_SplineOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_SplineOptions();
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

    static QG_TextOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_TextOptions();
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

    static QG_InsertOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_InsertOptions();
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

    static QG_ImageOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_ImageOptions();
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
    //static QLabel* l = NULL;
    static QG_DimOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_DimOptions();
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
    //static QLabel* l = NULL;
    static QG_DimLinearOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_DimLinearOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
			toolWidget->show();
        }
    }
}



/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void QG_DialogFactory::requestSnapDistOptions(double& dist, bool on) {
    static QG_SnapDistOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_SnapDistOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setDist(&dist);
			toolWidget->show();
        }
    }
}



/**
 * Shows a widget for 'snap to a point with a given distance' options.
 */
void QG_DialogFactory::requestMoveRotateOptions(RS_ActionInterface* action,
        bool on, bool update) {
    static QG_MoveRotateOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_MoveRotateOptions();
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
    static QG_TrimAmountOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_TrimAmountOptions();
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
    static QG_BevelOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_BevelOptions();
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
    static QG_RoundOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_RoundOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
            toolWidget->show();
        }
    }
}



/**
 * Shows a widget for library insert options.
 */
void QG_DialogFactory::requestLibraryInsertOptions(RS_ActionInterface* action,
        bool on, bool update) {
    static QG_LibraryInsertOptions* toolWidget = NULL;

    if (optionWidget!=NULL) {
        if (toolWidget!=NULL) {
            delete toolWidget;
            toolWidget = NULL;
        }
        if (on==true) {
            toolWidget = new QG_LibraryInsertOptions();
            optionWidget->addWidget(toolWidget);
            toolWidget->setAction(action, update);
			toolWidget->show();
        }
    }
}



/**
 * Shows the given toolbar.
 */
void QG_DialogFactory::requestToolBar(RS2::ToolBarId id) {
    if (cadToolBar!=NULL) {
        cadToolBar->showToolBar(id);
    }
}



/**
 * Shows the select toolbar with the given action to launch.
 */
void QG_DialogFactory::requestToolBarSelect(RS_ActionInterface* selectAction,
        RS2::ActionType nextAction) {
    if (cadToolBar!=NULL) {
        cadToolBar->showToolBarSelect(selectAction, nextAction);
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
    if (entity==NULL) {
        return false;
    }

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
            QG_DlgSpline dlg(parent);
            dlg.setSpline(*((RS_Spline*)entity));
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
                ((RS_Dimension*)entity)->update(true);
            }
        }
        break;
		
    case RS2::EntityDimLinear: {
            QG_DlgDimLinear dlg(parent);
            dlg.setDim(*((RS_DimLinear*)entity));
            if (dlg.exec()) {
                dlg.updateDim();
                ret = true;
                ((RS_DimLinear*)entity)->update(true);
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
    if (dim==NULL) {
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
 * Shows a dialog to edit the attributes of the given text entity.
 */
bool QG_DialogFactory::requestTextDialog(RS_Text* text) {
    if (text==NULL) {
        return false;
    }

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
    if (hatch==NULL) {
        return false;
    }

    RS_PATTERNLIST->init();

    QG_DlgHatch dlg(parent);
    dlg.setHatch(*hatch, true);
    if (dlg.exec()) {
        dlg.updateHatch();
        return true;
    }

    return false;
}



/**
 * Shows a dialog for CAM options.
 */
#ifdef RS_CAM
bool QG_DialogFactory::requestCamOptionsDialog(RS_Graphic& graphic) {
	RS_DEBUG->print("QG_DialogFactory::requestCamOptionsDialog");
	RS_CamDialog dlg(graphic, parent);
	RS_DEBUG->print("QG_DialogFactory::requestCamOptionsDialog: exec");
	if (dlg.exec()) {
		RS_DEBUG->print("QG_DialogFactory::requestCamOptionsDialog: OK");
		return true;
	}
    RS_DEBUG->print("QG_DialogFactory::requestCamOptionsDialog: Cancel");
	return false;
}
#endif

#ifdef RVT_CAM
bool QG_DialogFactory::requestCamProfileDialog(RVT_CAMProfileData& data) {
	RS_DEBUG->print("QG_DialogFactory::requestCamProfileDialog");
        RVT_CamMakeProfileDialog dlg(parent);
	dlg.setData(&data);
	RS_DEBUG->print("QG_DialogFactory::requestCamProfileDialog: exec");
	if (dlg.exec()) {
		dlg.updateData();
		RS_DEBUG->print("QG_DialogFactory::requestCamOptionsDialog: OK");
		return true;
	}
    RS_DEBUG->print("QG_DialogFactory::requestCamProfileDialog: Cancel");
	return false;
}
#endif

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


/**
 * Back to last menu in cad toolbar.
 */
void QG_DialogFactory::requestPreviousMenu() {
    if (cadToolBar!=NULL) {
        cadToolBar->showToolBarMain();
    }
}



/**
 * Called whenever the mouse position changed.
 */
void QG_DialogFactory::updateCoordinateWidget(const RS_Vector& abs,
        const RS_Vector& rel, bool updateFormat) {
    if (coordinateWidget!=NULL) {
        coordinateWidget->setCoordinates(abs, rel, updateFormat);
    }
}



/**
 * Called when an action has a mouse hint.
 */
void QG_DialogFactory::updateMouseWidget(const RS_String& left,
        const RS_String& right) {
    if (mouseWidget!=NULL) {
        mouseWidget->setHelp(left, right);
    }
    if (commandWidget!=NULL) {
        commandWidget->setCommand(left);
    }
}



/**
 * Called whenever the selection changed.
 */
void QG_DialogFactory::updateSelectionWidget(int num) {
    if (selectionWidget!=NULL) {
        selectionWidget->setNumber(num);
    }
}


/**
 * Called when an action needs to communicate 'message' to the user.
 */
void QG_DialogFactory::commandMessage(const RS_String& message) {
	RS_DEBUG->print("QG_DialogFactory::commandMessage");
    if (commandWidget!=NULL) {
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
    QString e = ext.lower();

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
    }
    else {
        return ext.upper();
    }
}


