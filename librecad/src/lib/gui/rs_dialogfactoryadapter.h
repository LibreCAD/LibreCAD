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
	void requestWarningDialog(const QString&) override {}
	RS_GraphicView* requestNewDocument(const QString&, RS_Document*) {return nullptr;}
	RS_Layer* requestNewLayerDialog(RS_LayerList*) override{return nullptr;}
	RS_Layer* requestLayerRemovalDialog(RS_LayerList*) override{return nullptr;}
	QStringList requestSelectedLayersRemovalDialog(RS_LayerList*) override{return {};}
	RS_Layer* requestEditLayerDialog(RS_LayerList*) override{return nullptr;}
	RS_BlockData requestNewBlockDialog(RS_BlockList*) override {return {};}
	RS_Block* requestBlockRemovalDialog(RS_BlockList*) override {return nullptr;}
	QList<RS_Block*> requestSelectedBlocksRemovalDialog(RS_BlockList*) override{return {};}
	RS_BlockData requestBlockAttributesDialog(RS_BlockList*) override{return {};}
	void requestEditBlockWindow(RS_BlockList*) override{}
	void closeEditBlockWindow(RS_Block*) override {}
	QString requestImageOpenDialog() override {return {};}
	void requestSnapDistOptions(double* , bool) override {}
	void requestSnapMiddleOptions(int* , bool) override {}
	bool requestAttributesDialog(RS_AttributesData&, RS_LayerList&) override{return false;}
	bool requestMoveDialog(RS_MoveData&) override {return false;}
	bool requestRotateDialog(RS_RotateData&) override {return false;}
	bool requestScaleDialog(RS_ScaleData&) override {return false;}
	bool requestMirrorDialog(RS_MirrorData&) override {return false;}
	bool requestMoveRotateDialog(RS_MoveRotateData&) override {return false;}
	bool requestRotate2Dialog(RS_Rotate2Data&) override {return false;}
	bool requestModifyEntityDialog(RS_Entity*) override {return false;}
	bool requestMTextDialog(RS_MText*) override {return false;}
	bool requestTextDialog(RS_Text*) override {return false;}
	bool requestHatchDialog(RS_Hatch*) override {return false;}
	int requestOptionsGeneralDialog() override {return -1;}
    void requestKeyboardShortcutsDialog([[maybe_unused]]LC_ActionGroupManager *pManager) override{}
	int requestOptionsDrawingDialog(RS_Graphic&, int) override {return -1;}
	bool requestOptionsMakerCamDialog() override {return false;}
	QString requestFileSaveAsDialog(const QString&, const QString&, const QString&, QString*) override {return {};}
	void updateCoordinateWidget(const RS_Vector& , const RS_Vector& , bool =false) override {}
 void updateMouseWidget(const QString&, const QString&,[[maybe_unused]] const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE()) override{}
 void clearMouseWidgetIcon() override  {}
 void setStatusBarManager([[maybe_unused]]LC_QTStatusbarManager *statusBarManager) override {}
	void updateSelectionWidget(int, double) override {}
//    void updateArcTangentialOptions(double, bool) override{}
	void commandMessage(const QString&) override {}
 void command([[maybe_unused]]const QString& message) override{};
	void setMouseWidget(QG_MouseWidget*) override {}
	void setCoordinateWidget(QG_CoordinateWidget* ) override {}
 void setRelativeZeroCoordinatesWidget([[maybe_unused]]LC_RelZeroCoordinatesWidget *widget) override {}
 void setSelectionWidget(QG_SelectionWidget* ) override {}
	void setCommandWidget(QG_CommandWidget* ) override {}
	void displayBlockName(const QString&, const bool&) override {}
 void hideSnapOptions() override {};
 void removeOptionsWidget([[maybe_unused]]QWidget *options) override {}
 void addOptionsWidget([[maybe_unused]]QWidget * options) override {}
 void setCurrentQAction(QAction *action) override{};
};

#endif
