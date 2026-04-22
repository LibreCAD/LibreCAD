/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef RS_DIALOGFACTORYINTERFACE_H
#define RS_DIALOGFACTORYINTERFACE_H
#include "rs_block.h"

class QWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class QG_MouseWidget;
class QG_SelectionWidget;
class RS_ActionInterface;
class RS_Block;
class RS_BlockList;
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
class RS_MText;
class RS_Painter;
class RS_Solid;
class RS_Text;
class RS_Vector;
class LC_GraphicViewport;

struct RS_ArcData;
struct RS_AttributesData;
struct RS_BevelData;
struct RS_CircleData;
struct RS_DimLinearData;
struct RS_DimensionData;
struct RS_MirrorData;
struct RS_MoveData;
struct RS_MoveRotateData;
struct RS_Rotate2Data;
struct RS_RotateData;
struct RS_RoundData;
struct RS_ScaleData;

/**
 * Interface for objects that can create and show dialogs.
 */
class RS_DialogFactoryInterface {
public:
    virtual ~RS_DialogFactoryInterface() = default;

    /**
     * This virtual method must be overwritten and must provide
     * a message dialog.
     */
    virtual void requestWarningDialog([[maybe_unused]] const QString& warning) {
    }

    /**
     * This virtual method must be overwritten and must create a new
     * window for the given document or for a new document isf no document
     * is given.
     */
    //    virtual RS_GraphicView* requestNewDocument(const QString& fileName = QString(),
    //                        RS_Document* doc=NULL) = 0;

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
    virtual RS_Layer* requestNewLayerDialog([[maybe_unused]] RS_LayerList* layerList = nullptr) {
        return nullptr;
    }

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
    virtual RS_Layer* requestLayerRemovalDialog([[maybe_unused]] RS_LayerList* layerList = nullptr) {
        return nullptr;
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that asks for permission for removing the selected
     * layers from the layer list. The method must not actually
     * remove those layers. This is up to the caller.
     *
     * @return The implementation is expected to return a list
     *         of selected layers names to be removed, or empty
     *         list if the user cancels the dialog.
     */
    virtual QStringList requestSelectedLayersRemovalDialog([[maybe_unused]] RS_LayerList* layerList = nullptr) {
        return QStringList();
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to edit the layers attributes. The method must
     * not actually edit the layer. This is up to the caller.
     *
     * @return The implementation is expected to return a pointer
     *         to the modified layer or NULL if the user
     *         cancels the dialog.
     */
    virtual RS_Layer* requestEditLayerDialog([[maybe_unused]] RS_LayerList* layerList = nullptr) {
        return nullptr;
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog for choosing the properties of a new block to be
     * created. The method must create the new block but not add
     * it to the block list. The latter is up to the caller.
    *
    * @param blockList
     *
     * @return The implementation is expected to return a pointer
     *         to the newly created block or NULL if the user
     *         cancels the dialog.
     */
    virtual RS_BlockData requestNewBlockDialog([[maybe_unused]] RS_BlockList* blockList) {
        return RS_BlockData();
    }

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
    virtual RS_Block* requestBlockRemovalDialog([[maybe_unused]] RS_BlockList* blockList) {
        return nullptr;
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that asks for permission for removing the selected
     * blocks from the block list. The method must not actually
     * remove those blocks. This is up to the caller.
     *
     * @return The implementation is expected to return a list
     *         of selected blocks to be removed, or empty
     *         list if the user cancels the dialog.
     */
    virtual QList<RS_Block*> requestSelectedBlocksRemovalDialog([[maybe_unused]] RS_BlockList* blockList = nullptr) {
        return QList<RS_Block*>();
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog that allows to change blocks attributes of the
     * currently active block.
     *
     * @return The implementation is expected to return a pointer
     *         to the block which was changed or NULL if the user
     *         cancels the dialog.
     */
    virtual RS_BlockData requestBlockAttributesDialog([[maybe_unused]] RS_BlockList* blockList) {
        return RS_BlockData();
    }

    virtual void closeEditBlockWindow([[maybe_unused]] RS_Block* block) {
    }

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for saving a file. The method must
     * not actually save the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    //virtual QString requestFileSaveAsDialog() = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for opening a file. The method must
     * not actually open the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    //virtual QString requestFileOpenDialog() = 0;

    /**
     * This virtual method must be overwritten and must provide
     * a dialog to get a filename for opening an image file. The method must
     * not actually open the file. This is up to the caller.
     *
     * @return The implementation is expected to return a string
     *         which contains the file name or an empty string if
     *         the user cancels the dialog.
     */
    virtual QString requestImageOpenDialog() {
        return "";
    }

    /**
     * This virtual method must be overwritten and must present
     * a widget for entity attributes.
     *
     * @param data Attribute data which can be directly changed
     *             by the presented widget.
     * @param layerList
     */
    virtual bool requestAttributesDialog([[maybe_unused]] RS_AttributesData& data, [[maybe_unused]] RS_LayerList& layerList) {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog to edit the given entity.
     *
     * @param entity Pointer to the entity.
     * @param viewport
     */
    virtual bool requestModifyEntityDialog([[maybe_unused]] RS_Entity* entity, [[maybe_unused]] LC_GraphicViewport* viewport) {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog to edit multi-line text entity attributes.
     *
     * @param text  Pointer to the mtext entity.
     * @param viewport
     */
    virtual bool requestMTextDialog([[maybe_unused]] RS_MText* text, [[maybe_unused]] LC_GraphicViewport* viewport) {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog to edit text entity attributes.
     *
     * @param text Pointer to the text entity.
     * @param viewport
     */
    virtual bool requestTextDialog([[maybe_unused]] RS_Text* text, [[maybe_unused]] LC_GraphicViewport* viewport) {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog to select pattern attributes.
     *
     * @param hatch Pointer to the hatch entity.
     * @param viewport
     */
    virtual bool requestHatchDialog([[maybe_unused]] RS_Hatch* hatch, [[maybe_unused]] LC_GraphicViewport* viewport) {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog for drawing options.
         *
         * @param graphic Graphic document.
         * @param tabIndex
     */
    virtual int requestOptionsDrawingDialog([[maybe_unused]] RS_Graphic& graphic, [[maybe_unused]] int tabIndex = -1) {
        return 0;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog for options how to export as MakeCAM SVG.
     */
    virtual bool requestOptionsMakerCamDialog() {
        return false;
    }

    /**
     * This virtual method must be overwritten and must present
     * a dialog for saving a file.
     */
    virtual QString requestFileSaveAsDialog([[maybe_unused]] const QString& caption = QString(),
                                            [[maybe_unused]] const QString& dir = QString(),
                                            [[maybe_unused]] const QString& filter = QString(),
                                            [[maybe_unused]] QString* selectedFilter = nullptr) {
        return "";
    }

    [[deprecated]] virtual void displayBlockName([[maybe_unused]] const QString& blockName, [[maybe_unused]] const bool display) {
    }

    /**
     * This virtual method must be overwritten if the graphic view has
     * a component that is interested in command messages (such as a
    * command line history).
     * The implementation will be called typically by actions to inform
     * the user about current events and errors.
     *
     * @param message The message for the user.
     */
    virtual void commandMessage([[maybe_unused]] const QString& message) {
    }

    virtual void command([[maybe_unused]] const QString& message) {
    }

    virtual void commandPrompt([[maybe_unused]] const QString& message) {
    }
};

#endif
