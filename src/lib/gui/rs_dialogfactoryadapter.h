/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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


#ifndef RS_DIALOGFACTORYADAPTER_H
#define RS_DIALOGFACTORYADAPTER_H

#include "rs_dialogfactoryinterface.h"
#include "rs_block.h"

class RS_BlockList;

/**
 * Adapter for dialog factory interface.
 * Used when no factory object was set.
 */
class RS_DialogFactoryAdapter : public RS_DialogFactoryInterface {
public:
    RS_DialogFactoryAdapter() {}
    virtual ~RS_DialogFactoryAdapter() {}
    virtual void requestPreviousMenu() {}
    virtual void requestWarningDialog(const QString& ) {}
    virtual RS_GraphicView* requestNewDocument(const QString& = QString::null,
                        RS_Document* =NULL) { return NULL; }
    virtual RS_Layer* requestNewLayerDialog(
        RS_LayerList* = NULL) { return NULL; }
    virtual RS_Layer* requestLayerRemovalDialog(
        RS_LayerList* = NULL) { return NULL; }
    virtual RS_Layer* requestEditLayerDialog(
        RS_LayerList* = NULL) { return NULL; }
    virtual RS_BlockData requestNewBlockDialog(RS_BlockList* )
                { return RS_BlockData(); }
    virtual RS_Block* requestBlockRemovalDialog(
        RS_BlockList* ) { return NULL; }
    virtual RS_BlockData requestBlockAttributesDialog(
        RS_BlockList* ) { return RS_BlockData(); }
    virtual void requestEditBlockWindow(
        RS_BlockList* ) {}
        virtual void closeEditBlockWindow(RS_Block* ) {}
    virtual QString requestImageOpenDialog() { return ""; }
    virtual void requestOptions(RS_ActionInterface* ,
                bool , bool = false) {}
    virtual void requestSnapDistOptions(double& , bool ) {}
    virtual void requestSnapMiddleOptions(int& , bool ) {}
    virtual void requestModifyOffsetOptions(double& , bool ) {}
    virtual bool requestAttributesDialog(RS_AttributesData& ,
                RS_LayerList& ) { return false; }
    virtual bool requestMoveDialog(RS_MoveData& ) { return false; }
    virtual bool requestRotateDialog(RS_RotateData& ) { return false; }
    virtual bool requestScaleDialog(RS_ScaleData& ) { return false; }
    virtual bool requestMirrorDialog(RS_MirrorData& ) { return false; }
    virtual bool requestMoveRotateDialog(RS_MoveRotateData& ) { return false; }
    virtual bool requestRotate2Dialog(RS_Rotate2Data& ) { return false; }
    virtual void requestToolBar(RS2::ToolBarId ) {}
    virtual void resetToolBar() {}
    virtual void requestToolBarSelect(RS_ActionInterface* ,
                                      RS2::ActionType ) {}
    virtual bool requestModifyEntityDialog(RS_Entity* ) { return false; }
    virtual bool requestTextDialog(RS_Text* ) { return false; }
    virtual bool requestHatchDialog(RS_Hatch* ) { return false; }
    virtual void requestOptionsGeneralDialog() {}
    virtual void requestOptionsDrawingDialog(RS_Graphic& ) {}
    virtual void updateCoordinateWidget(const RS_Vector& , const RS_Vector& , bool =false) {}
    virtual void updateMouseWidget(const QString& ,
                                   const QString& ,
                                   bool) {}
    virtual void restoreMouseWidget(void) {}
    virtual void updateSelectionWidget(int /*c*/, double /*l*/ ) {}
    virtual void updateArcTangentialOptions(const double& , bool ){}
    virtual void commandMessage(const QString& ) {}
        virtual bool isAdapter() { return true; }
};

#endif
