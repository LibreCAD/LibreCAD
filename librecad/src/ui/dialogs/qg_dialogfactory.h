/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024
 Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
 Copyright (C) 2001-2003 RibbonSoft. All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef QG_DIALOGFACTORY_H
#define QG_DIALOGFACTORY_H
#include "rs_dialogfactoryinterface.h"

class LC_RelZeroCoordinatesWidget;
class QG_SnapToolBar;
class LC_SnapOptionsWidgetsHolder;
class QG_SnapMiddleOptions;
class QG_SnapDistOptions;
class QWidget;
class QToolBar;
class QG_CoordinateWidget;
class QG_SelectionWidget;
class QG_MouseWidget;
class QG_CommandWidget;
class RS_Document;
class RS_Vector;

#define QG_DIALOGFACTORY (RS_DialogFactory::instance()->getFactoryObject())

/**
 * This is the Qt implementation of a widget which can create and
 * show dialogs.
 */
class QG_DialogFactory: public RS_DialogFactoryInterface {
public:
    QG_DialogFactory(QWidget* parent, QToolBar* ow, LC_SnapOptionsWidgetsHolder *);
    ~QG_DialogFactory() override;


public:

/**
 * @return command widget or nullptr.
 */
    QG_CommandWidget* getCommandWidget() const{
        return m_commandWidget;
    }

/**
 * Links the dialog factory to a main app window.
 */

    void requestWarningDialog(const QString& warning) override;

    RS_Layer* requestNewLayerDialog(
        RS_LayerList* layerList = nullptr) override;
    RS_Layer* requestLayerRemovalDialog(
        RS_LayerList* layerList = nullptr) override;
    QStringList requestSelectedLayersRemovalDialog(
        RS_LayerList* layerList = nullptr) override;
    RS_Layer* requestEditLayerDialog(
        RS_LayerList* layerList = nullptr) override;

    RS_BlockData requestNewBlockDialog(RS_BlockList* blockList) override;
    RS_Block* requestBlockRemovalDialog(
        RS_BlockList* blockList) override;
    QList<RS_Block*> requestSelectedBlocksRemovalDialog(
        RS_BlockList* blockList = nullptr) override;
    RS_BlockData requestBlockAttributesDialog(
        RS_BlockList* blockList) override;
   void closeEditBlockWindow(RS_Block* /*blockList*/) override{}
//QString requestFileSaveAsDialog() override;
//QString requestFileOpenDialog() override;

    QString requestImageOpenDialog() override;

protected:

public:
    bool requestAttributesDialog(RS_AttributesData& data,
                                 RS_LayerList& layerList) override;
    bool requestMoveDialog(RS_MoveData& data) override;
    bool requestRotateDialog(RS_RotateData& data) override;
    bool requestScaleDialog(RS_ScaleData& data) override;
    bool requestMirrorDialog(RS_MirrorData& data) override;
    bool requestMoveRotateDialog(RS_MoveRotateData& data) override;
    bool requestRotate2Dialog(RS_Rotate2Data& data) override;

    bool requestModifyEntityDialog(RS_Entity *entity, LC_GraphicViewport *viewport) override;
    bool requestMTextDialog(RS_MText *text, LC_GraphicViewport *viewport) override;
    bool requestTextDialog(RS_Text *text, LC_GraphicViewport *viewport) override;
    bool requestHatchDialog(RS_Hatch *hatch, LC_GraphicViewport *viewport) override;

    int requestOptionsDrawingDialog(RS_Graphic& graphic, int tabIndex) override;
    bool requestOptionsMakerCamDialog() override;

    QString requestFileSaveAsDialog(const QString& caption = QString(),
                                    const QString& dir = QString(),
                                    const QString& filter = QString(),
                                    QString* selectedFilter = 0) override;


    void updateSelectionWidget(int num, double length) override;//updated for total number of selected, and total length of selected
    void commandMessage(const QString& message) override;
    void command(const QString& message) override;
    void commandPrompt(const QString& message) override;

    static QString extToFormat(const QString& ext);
    void displayBlockName(const QString& blockName, const bool& display) override;
    // void setCurrentQAction(QAction *action) override;

    /// fixme - sand - temporary, remove accessort later with further refactorings
    void set_selection_widget(QG_SelectionWidget* selection_widget) {
        m_selectionWidget = selection_widget;
    }

    void set_command_widget(QG_CommandWidget* command_widget) {
        m_commandWidget = command_widget;
    }

    void set_rel_zero_coordinates_widget(LC_RelZeroCoordinatesWidget* rel_zero_coordinates_widget) {
        m_relZeroCoordinatesWidget = rel_zero_coordinates_widget;
    }


protected:
//! Pointer to the widget which can host dialogs
    QWidget* parent = nullptr;

//! Pointer to the selection widget.
    QG_SelectionWidget* m_selectionWidget = nullptr;
//! Pointer to the command line widget
    QG_CommandWidget* m_commandWidget = nullptr;

    LC_RelZeroCoordinatesWidget *m_relZeroCoordinatesWidget;

};

#endif
