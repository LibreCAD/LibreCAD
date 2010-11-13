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

#ifndef QG_DIALOGFACTORY_H
#define QG_DIALOGFACTORY_H

#include <qwidget.h>
#include <QToolBar>

#include "rs_dialogfactoryinterface.h"
#include "rs_vector.h"
#include "rs_debug.h"

/*
#include "qg_cadtoolbar.h"
#include "qg_coordinatewidget.h"
#include "qg_selectionwidget.h"
#include "qg_mousewidget.h"
#include "qg_printpreviewoptions.h"
*/

class QG_CadToolBar;
class QToolBar;
class QG_CoordinateWidget;
class QG_SelectionWidget;
class QG_MouseWidget;
//class PrintPreviewOptions;
class QG_CommandWidget;
class QG_MainWindowInterface;
class RS_Document;

#define QG_DIALOGFACTORY (RS_DialogFactory::instance()->getFactoryObject()->isAdapter()==false ? ((QG_DialogFactory*)RS_DialogFactory::instance()->getFactoryObject()) : NULL)

/**
 * This is the Qt implementation of a widget which can create and
 * show dialogs.
 */
class QG_DialogFactory: public RS_DialogFactoryInterface {
public:
    QG_DialogFactory(QWidget* parent, QToolBar* ow);
    virtual ~QG_DialogFactory();

protected:
    /**
     * Links factory to a widget that can host tool options.
     */
    virtual void setOptionWidget(QToolBar* ow) {
		RS_DEBUG->print("QG_DialogFactory::setOptionWidget");
        optionWidget = ow;
		RS_DEBUG->print("QG_DialogFactory::setOptionWidget: OK");
    }

public:
    /**
     * Links this dialog factory to a coordinate widget.
     */
    virtual void setCoordinateWidget(QG_CoordinateWidget* cw) {
        coordinateWidget = cw;
    }

    /**
     * Links this dialog factory to a mouse widget.
     */
    virtual void setMouseWidget(QG_MouseWidget* mw) {
        mouseWidget = mw;
    }
	
    /**
     * Links this dialog factory to a selection widget.
     */
    virtual void setSelectionWidget(QG_SelectionWidget* sw) {
        selectionWidget = sw;
    }

    /**
     * Links this dialog factory to the cad tool bar.
     */
    virtual void setCadToolBar(QG_CadToolBar* ctb) {
        cadToolBar = ctb;
    }

    /**
     * @return cad tool bar or NULL.
     */
    QG_CadToolBar* getCadToolBar() {
        return cadToolBar;
    }

    /**
     * Links this dialog factory to a command widget.
     */
    virtual void setCommandWidget(QG_CommandWidget* cw) {
        commandWidget = cw;
    }

    /**
     * @return command widget or NULL.
     */
    QG_CommandWidget* getCommandWidget() {
        return commandWidget;
    }
	
    /**
     * Links the dialog factory to a main app window.
     */
    virtual void setMainWindow(QG_MainWindowInterface* mw) {
        mainWindow = mw;
    }


    virtual void requestWarningDialog(const RS_String& warning);
	
    virtual RS_GraphicView* requestNewDocument(const RS_String& fileName = RS_String::null, 
			RS_Document* doc=NULL);

    virtual RS_Layer* requestNewLayerDialog(
        RS_LayerList* layerList = NULL);
    virtual RS_Layer* requestLayerRemovalDialog(
        RS_LayerList* layerList = NULL);
    virtual RS_Layer* requestEditLayerDialog(
        RS_LayerList* layerList = NULL);

    virtual RS_BlockData requestNewBlockDialog(RS_BlockList* blockList);
    virtual RS_Block* requestBlockRemovalDialog(
        RS_BlockList* blockList);
    virtual RS_BlockData requestBlockAttributesDialog(
        RS_BlockList* blockList);
    virtual void requestEditBlockWindow(
        RS_BlockList* /*blockList*/) {}
    virtual void closeEditBlockWindow(
        RS_Block* /*blockList*/) {}
    //virtual RS_String requestFileSaveAsDialog();
    //virtual RS_String requestFileOpenDialog();
	
    virtual RS_String requestImageOpenDialog();


    virtual void requestOptions(RS_ActionInterface* action,
                                bool on, bool update = false);

protected:
    virtual void requestPrintPreviewOptions(RS_ActionInterface* action,
                                    bool on, bool update);
    virtual void requestLineOptions(RS_ActionInterface* action,
                                    bool on);
    virtual void requestPolylineOptions(RS_ActionInterface* action,
                                    bool on, bool update);
    virtual void requestLineAngleOptions(RS_ActionInterface* action,
                                         bool on, bool update);
    virtual void requestLineRelAngleOptions(RS_ActionInterface* action,
                                            bool on, bool update);
    virtual void requestLineParallelOptions(RS_ActionInterface* action,
                                            bool on, bool update);
    virtual void requestLineParallelThroughOptions(RS_ActionInterface* action,
                                            bool on, bool update);
    virtual void requestLineBisectorOptions(RS_ActionInterface* action,
                                            bool on, bool update);
    virtual void requestLinePolygonOptions(RS_ActionInterface* action,
                                            bool on, bool update);
    virtual void requestLinePolygon2Options(RS_ActionInterface* action,
                                            bool on, bool update);

    virtual void requestArcOptions(RS_ActionInterface* action,
                                   bool on, bool update);
    
	virtual void requestArcTangentialOptions(RS_ActionInterface* action,
                                   bool on, bool update);

    virtual void requestCircleOptions(RS_ActionInterface* action,
                                      bool on, bool update);
									  
    virtual void requestSplineOptions(RS_ActionInterface* action,
                                      bool on, bool update);
									  
    virtual void requestTextOptions(RS_ActionInterface* action,
                                   bool on, bool update);

    virtual void requestDimensionOptions(RS_ActionInterface* action,
                                         bool on, bool update);
    virtual void requestDimLinearOptions(RS_ActionInterface* action,
                                         bool on, bool update);
										 
    virtual void requestInsertOptions(RS_ActionInterface* action,
                                   bool on, bool update);
    virtual void requestImageOptions(RS_ActionInterface* action,
                                   bool on, bool update);
										 
    virtual void requestTrimAmountOptions(RS_ActionInterface* action,
                                          bool on, bool update);
    virtual void requestMoveRotateOptions(RS_ActionInterface* action,
                                          bool on, bool update);
    virtual void requestBevelOptions(RS_ActionInterface* action,
                                     bool on, bool update);
    virtual void requestRoundOptions(RS_ActionInterface* action,
                                     bool on, bool update);
    virtual void requestLibraryInsertOptions(RS_ActionInterface* action,
                                     bool on, bool update);

public:
    virtual void requestSnapDistOptions(double& dist, bool on);

public:
    virtual void requestToolBar(RS2::ToolBarId id);
    virtual void requestToolBarSelect(RS_ActionInterface* selectAction,
                                      RS2::ActionType nextAction);

    virtual bool requestAttributesDialog(RS_AttributesData& data, 
				RS_LayerList& layerList);
    virtual bool requestMoveDialog(RS_MoveData& data);
    virtual bool requestRotateDialog(RS_RotateData& data);
    virtual bool requestScaleDialog(RS_ScaleData& data);
    virtual bool requestMirrorDialog(RS_MirrorData& data);
    virtual bool requestMoveRotateDialog(RS_MoveRotateData& data);
    virtual bool requestRotate2Dialog(RS_Rotate2Data& data);
	
    virtual bool requestModifyEntityDialog(RS_Entity* entity);
    virtual bool requestTextDialog(RS_Text* text);
    virtual bool requestHatchDialog(RS_Hatch* hatch);
	
#ifdef RS_CAM
    virtual bool requestCamOptionsDialog(RS_Graphic& graphic);
#endif

#ifdef RVT_CAM
    virtual bool requestCamProfileDialog(RVT_CAMProfileData& data);
#endif


    virtual void requestOptionsGeneralDialog();
    virtual void requestOptionsDrawingDialog(RS_Graphic& graphic);

    virtual void requestPreviousMenu();

    virtual void updateCoordinateWidget(const RS_Vector& abs,
                                        const RS_Vector& rel,
										bool updateFormat=false);
    virtual void updateMouseWidget(const RS_String& left,
                                   const RS_String& right);
    virtual void updateSelectionWidget(int num);
    virtual void commandMessage(const RS_String& message);
	virtual bool isAdapter() { return false; }

	static QString extToFormat(const QString& ext);


protected:
    //! Pointer to the widget which can host dialogs
    QWidget* parent;
    //! Pointer to the widget which can host individual tool options
    QToolBar* optionWidget;
    //! Pointer to the coordinate widget.
    QG_CoordinateWidget* coordinateWidget;
    //! Pointer to the mouse widget.
    QG_MouseWidget* mouseWidget;
    //! Pointer to the selection widget.
    QG_SelectionWidget* selectionWidget;
    //! Pointer to the CAD tool bar
    QG_CadToolBar* cadToolBar;
    //! Pointer to the command line widget
    QG_CommandWidget* commandWidget;
    //! Pointer to the main app window
    QG_MainWindowInterface* mainWindow;
};

#endif
