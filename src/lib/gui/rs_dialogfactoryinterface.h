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


#ifndef RS_DIALOGFACTORYINTERFACE_H
#define RS_DIALOGFACTORYINTERFACE_H

#include "rs.h"
#include "rs_string.h"

class RS_ActionInterface;
class RS_ArcData;
class RS_AttributesData;
class RS_BevelData;
class RS_Block;
class RS_BlockData;
class RS_BlockList;
class RS_CircleData;
class RS_DimLinearData;
class RS_DimensionData;
class RS_Document;
class RS_Entity;
class RS_EventHandler;
class RS_Graphic;
class RS_GraphicView;
class RS_Grid;
class RS_Hatch;
class RS_Insert;
class RS_Layer;
class RS_LayerList;
class RS_MirrorData;
class RS_MoveData;
class RS_MoveRotateData;
class RS_Painter;
class RS_Rotate2Data;
class RS_RotateData;
class RS_RoundData;
class RS_ScaleData;
class RS_Solid;
class RS_Text;
class RS_Vector;
#ifdef RVT_CAM
class RVT_CAMProfileData;
#endif

/**
 * Interface for objects that can create and show dialogs.
 */
class RS_DialogFactoryInterface {
public:
    RS_DialogFactoryInterface() {}
    ;
    virtual ~RS_DialogFactoryInterface() {}
    ;

    /**
     * This virtual method must be overwritten and must show the previously
     * shown menu in the cad toolbar.
     */
    virtual void requestPreviousMenu() = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a message dialog.
     */
    virtual void requestWarningDialog(const RS_String& warning) = 0;

	/**
	 * This virtual method must be overwritten and must create a new
	 * window for the given document or for a new document isf no document
	 * is given.
	 */
    virtual RS_GraphicView* requestNewDocument(const RS_String& fileName = RS_String::null, 
			RS_Document* doc=NULL) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog for choosing the properties of a new layer to be 
     * created. The method must create the new layer but not add 
     * it to the layer list. The latter is up to the caller.
     *
     * @return The implementation is expected to return a pointer 
     *         to the newly created layer or NULL if the user 
     *         cancels the dialog.
     */
    virtual RS_Layer* requestNewLayerDialog(
        RS_LayerList* layerList = NULL) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that asks for permission for removing the selected
     * layer from the layer list. The method must not actually
     * remove the layer. This is up to the caller.
     *
     * @return The implementation is expected to return a pointer
     *         to the layer which can ne removed or NULL if the user
     *         cancels the dialog.
     */
    virtual RS_Layer* requestLayerRemovalDialog(
        RS_LayerList* layerList = NULL) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to edit the layers attributes. The method must
     * not actually edit the layer. This is up to the caller.
     *
     * @return The implementation is expected to return a pointer 
     *         to the modified layer or NULL if the user 
     *         cancels the dialog.
     */
    virtual RS_Layer* requestEditLayerDialog(
        RS_LayerList* layerList = NULL) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog for choosing the properties of a new block to be 
     * created. The method must create the new block but not add 
     * it to the block list. The latter is up to the caller.
    *
    * @param block Pointer to the newly created block with default
    *              attributes.
     *
     * @return The implementation is expected to return a pointer 
     *         to the newly created block or NULL if the user 
     *         cancels the dialog.
     */
    virtual RS_BlockData requestNewBlockDialog(RS_BlockList* blockList) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that asks for permission for removing the selected 
     * block from the block list. The method must not actually 
     * remove the block. This is up to the caller.
     *
     * @return The implementation is expected to return a pointer 
     *         to the block which can be removed or NULL if the user 
     *         cancels the dialog.
     */
    virtual RS_Block* requestBlockRemovalDialog(
        RS_BlockList* blockList) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that allows to change blocks attributes of the  
     * currently active block. 
     *
     * @return The implementation is expected to return a pointer 
     *         to the block which was changed or NULL if the user 
     *         cancels the dialog.
     */
    virtual RS_BlockData requestBlockAttributesDialog(
        RS_BlockList* blockList) = 0;

    /**
     * This virtual method must be overwritten and should provide
     * a way to edit a block. 
     */
    virtual void requestEditBlockWindow(
        RS_BlockList* blockList) = 0;

	virtual void closeEditBlockWindow(RS_Block* block) = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for saving a file. The method must
     * not actually save the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    //virtual RS_String requestFileSaveAsDialog() = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for opening a file. The method must
     * not actually open the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    //virtual RS_String requestFileOpenDialog() = 0;
	
    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for opening an image file. The method must
     * not actually open the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    virtual RS_String requestImageOpenDialog() = 0;
	
    /**
     * This virtual method must be overwritten and must present
     * a widget for options for the given action.
     *
	 * @param action Pointer to the action which needs the options.
     * @param on true: switch widget on, false: off
	 * @param update true: widget gets data from the action, false: 
	 *   widget gets data from config file.
     */
    virtual void requestOptions(RS_ActionInterface* action, 
		bool on, bool update = false) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for snap point with distance options.
     *
     * @param dist Distance which can be directly changed 
     *             by the presented widget.
     * @param on true: switch widget on, false: off
     */
    virtual void requestSnapDistOptions(double& dist, bool on) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for entity attributes.
     *
     * @param data Attribute data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestAttributesDialog(RS_AttributesData& data,
		RS_LayerList& layerList) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for move options (number of copies).
     *
     * @param data Move data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestMoveDialog(RS_MoveData& data) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for rotate options (number of copies, angle).
     *
     * @param data Rotation data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestRotateDialog(RS_RotateData& data) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for rotate options (number of copies, angle).
     *
     * @param data Scaling data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestScaleDialog(RS_ScaleData& data) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for mirror options (number of copies).
     *
     * @param data Mirror data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestMirrorDialog(RS_MirrorData& data) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for move/rotate options (number of copies, angle).
     *
     * @param data Move/rotate data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestMoveRotateDialog(RS_MoveRotateData& data) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a widget for rotate around two centers options (number of 
    * copies, angles).
     *
     * @param data Rotate data which can be directly changed 
     *             by the presented widget.
     */
    virtual bool requestRotate2Dialog(RS_Rotate2Data& data) = 0;

    /**
     * This virtual method must be overwritten and must show
     * the given toolbar.
     *
     * @param id Tool bar ID.
     */
    virtual void requestToolBar(RS2::ToolBarId id) = 0;

    /**
     * This virtual method must be overwritten and must show
     * the tag toolbar with a button for launching the given
     * action.
     *
     * @param nextAction ID of next action to create after selecting was done.
     */
    virtual void requestToolBarSelect(RS_ActionInterface* selectAction,
                                      RS2::ActionType nextAction) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a dialog to edit the given entity.
     *
     * @param entity Pointer to the entity.
     */
    virtual bool requestModifyEntityDialog(RS_Entity* entity) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a dialog to edit text entity attributes.
     *
     * @param entity Pointer to the text entity.
     */
    virtual bool requestTextDialog(RS_Text* text) = 0;

    /**
     * This virtual method must be overwritten and must present
     * a dialog to select pattern attributes.
     *
     * @param entity Pointer to the hatch entity.
     */
    virtual bool requestHatchDialog(RS_Hatch* hatch) = 0;
	
    /**
     * This virtual method must be overwritten and must present
     * a dialog for general application options.
     */
    virtual void requestOptionsGeneralDialog() = 0;
	
    /**
     * This virtual method must be overwritten and must present
     * a dialog for drawing options.
	 *
	 * @param graphic Graphic document.
     */
    virtual void requestOptionsDrawingDialog(RS_Graphic& graphic) = 0;
	
#ifdef RS_CAM
    virtual bool requestCamOptionsDialog(RS_Graphic& graphic) = 0;
#endif

#ifdef RVT_CAM
    virtual bool requestCamProfileDialog(RVT_CAMProfileData& data) = 0;
#endif

    /**
     * This virtual method must be overwritten if the graphic view has
     * a component that is interested in the current mouse position.
     * The implementation will be called every time the mouse position
     * changes.
     *
     * @param abs Absolute coordiante of the mouse cursor or the 
     *            point it snaps to.
     * @param rel Relative coordiante.
     */
    virtual void updateCoordinateWidget(const RS_Vector& abs,
										const RS_Vector& rel,
										bool updateFormat=false) = 0;

    /**
     * This virtual method must be overwritten if the graphic view has
     * a component that is interested in the current mouse button hints.
     * The implementation will be called typically by actions to inform
     * the user about the current functionalty of the mouse buttons.
     *
     * @param left Help text for the left mouse button.
     * @param right Help text for the right mouse button.
     */
    virtual void updateMouseWidget(const RS_String& left,
                                   const RS_String& right) = 0;
								   
    /**
     * This virtual method must be overwritten if the graphic view has
     * a component that is interested in the current number of selected
	 * entities.
     * The implementation will be called every time the selection
     * changes.
     *
     * @param num Number of selected entities
     */
    virtual void updateSelectionWidget(int num) = 0;

    /**
     * This virtual method must be overwritten if the graphic view has
     * a component that is interested in command messages (such as a 
    * command line history).
     * The implementation will be called typically by actions to inform
     * the user about current events and errors.
     *
     * @param message The message for the user.
     */
    virtual void commandMessage(const RS_String& message) = 0;


	virtual bool isAdapter() = 0;
};

#endif
