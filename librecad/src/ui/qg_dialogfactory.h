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

#ifndef QG_DIALOGFACTORY_H
#define QG_DIALOGFACTORY_H

#include "rs_dialogfactoryinterface.h"

class QG_PolylineEquidistantOptions;
class QG_SnapMiddleOptions;
class QG_SnapDistOptions;
class QG_ModifyOffsetOptions;
class QWidget;

class QToolBar;
class QG_CoordinateWidget;
class QG_SelectionWidget;
class QG_MouseWidget;
class QG_ArcTangentialOptions;
class QG_PrintPreviewOptions;
//class PrintPreviewOptions;
class QG_CommandWidget;
class RS_Document;
class QG_LineAngleOptions;
class RS_Vector;

#define QG_DIALOGFACTORY (RS_DialogFactory::instance()->getFactoryObject())

/**
 * This is the Qt implementation of a widget which can create and
 * show dialogs.
 */
class QG_DialogFactory: public RS_DialogFactoryInterface {
public:
	QG_DialogFactory(QWidget* parent, QToolBar* ow);
	~QG_DialogFactory() override;

protected:
	/**
	 * Links factory to a widget that can host tool options.
	 */
	void setOptionWidget(QToolBar* ow);
public:
	/**
	 * Links this dialog factory to a coordinate widget.
	 */
	void setCoordinateWidget(QG_CoordinateWidget* cw) override{
		coordinateWidget = cw;
	}

	/**
	 * Links this dialog factory to a mouse widget.
	 */
	void setMouseWidget(QG_MouseWidget* mw) override{
		mouseWidget = mw;
	}

	/**
	 * Links this dialog factory to a selection widget.
	 */
	void setSelectionWidget(QG_SelectionWidget* sw) override{
		selectionWidget = sw;
	}

	/**
	 * Links this dialog factory to a command widget.
	 */
	void setCommandWidget(QG_CommandWidget* cw) override{
		commandWidget = cw;
	}

	/**
	 * @return command widget or nullptr.
	 */
	QG_CommandWidget* getCommandWidget() const{
		return commandWidget;
	}

	/**
	 * Links the dialog factory to a main app window.
	 */

	void requestWarningDialog(const QString& warning) override;

	RS_Layer* requestNewLayerDialog(
			RS_LayerList* layerList = nullptr) override;
	RS_Layer* requestLayerRemovalDialog(
			RS_LayerList* layerList = nullptr) override;
	RS_Layer* requestEditLayerDialog(
			RS_LayerList* layerList = nullptr) override;

	RS_BlockData requestNewBlockDialog(RS_BlockList* blockList) override;
	RS_Block* requestBlockRemovalDialog(
			RS_BlockList* blockList) override;
	RS_BlockData requestBlockAttributesDialog(
			RS_BlockList* blockList) override;
	void requestEditBlockWindow(RS_BlockList* /*blockList*/) override{}
	void closeEditBlockWindow(RS_Block* /*blockList*/) override{}
	//QString requestFileSaveAsDialog() override;
	//QString requestFileOpenDialog() override;

	QString requestImageOpenDialog() override;


	void requestOptions(RS_ActionInterface* action,
								bool on, bool update = false) override;

protected:
	void requestPrintPreviewOptions(RS_ActionInterface* action,
											bool on, bool update);
	void requestLineOptions(RS_ActionInterface* action,
									bool on);
	void requestPolylineOptions(RS_ActionInterface* action,
										bool on, bool update);
	void requestPolylineEquidistantOptions(RS_ActionInterface* action, bool on, bool update=false);
	void requestLineAngleOptions(RS_ActionInterface* action,
										 bool on, bool update);
	void requestLineRelAngleOptions(RS_ActionInterface* action,
											bool on, bool update);
	void requestLineParallelOptions(RS_ActionInterface* action,
											bool on, bool update);
	void requestLineParallelThroughOptions(RS_ActionInterface* action,
												   bool on, bool update);
	void requestLineBisectorOptions(RS_ActionInterface* action,
											bool on, bool update);
	void requestLinePolygonOptions(RS_ActionInterface* action,
										   bool on, bool update);
	void requestLinePolygon2Options(RS_ActionInterface* action,
											bool on, bool update);

	void requestArcOptions(RS_ActionInterface* action,
								   bool on, bool update);

	void requestArcTangentialOptions(RS_ActionInterface* action,
											 bool on, bool update);

	void requestCircleOptions(RS_ActionInterface* action,
									  bool on, bool update);

	void requestCircleTan2Options(RS_ActionInterface* action,
										  bool on, bool update);

	void requestSplineOptions(RS_ActionInterface* action,
									  bool on, bool update);

	void requestMTextOptions(RS_ActionInterface* action,
									 bool on, bool update);

	void requestTextOptions(RS_ActionInterface* action,
									bool on, bool update);

	void requestDimensionOptions(RS_ActionInterface* action,
										 bool on, bool update);
	void requestDimLinearOptions(RS_ActionInterface* action,
										 bool on, bool update);

	void requestInsertOptions(RS_ActionInterface* action,
									  bool on, bool update);
	void requestImageOptions(RS_ActionInterface* action,
									 bool on, bool update);

	void requestTrimAmountOptions(RS_ActionInterface* action,
										  bool on, bool update);
	void requestMoveRotateOptions(RS_ActionInterface* action,
										  bool on, bool update);
	void requestBevelOptions(RS_ActionInterface* action,
									 bool on, bool update);
	void requestRoundOptions(RS_ActionInterface* action,
									 bool on, bool update);
	void requestLibraryInsertOptions(RS_ActionInterface* action,
											 bool on, bool update);

public:
	void requestSnapDistOptions(double& dist, bool on) override;
	void requestSnapMiddleOptions(int& middlePoints, bool on) override;

public:

	bool requestAttributesDialog(RS_AttributesData& data,
										 RS_LayerList& layerList) override;
	bool requestMoveDialog(RS_MoveData& data) override;
	bool requestRotateDialog(RS_RotateData& data) override;
	bool requestScaleDialog(RS_ScaleData& data) override;
	bool requestMirrorDialog(RS_MirrorData& data) override;
	bool requestMoveRotateDialog(RS_MoveRotateData& data) override;
	bool requestRotate2Dialog(RS_Rotate2Data& data) override;

	bool requestModifyEntityDialog(RS_Entity* entity) override;
	void requestModifyOffsetOptions(double& dist, bool on) override;
	bool requestMTextDialog(RS_MText* text) override;
	bool requestTextDialog(RS_Text* text) override;
	bool requestHatchDialog(RS_Hatch* hatch) override;
	void requestOptionsGeneralDialog() override;
	void requestOptionsDrawingDialog(RS_Graphic& graphic) override;
	bool requestOptionsMakerCamDialog() override;

	QString requestFileSaveAsDialog(const QString& caption = QString(),
											const QString& dir = QString(),
											const QString& filter = QString(),
											QString* selectedFilter = 0) override;

	void updateCoordinateWidget(const RS_Vector& abs, const RS_Vector& rel, bool updateFormat=false) override;
	/**
	 * \brief updateMouseWidget Called when an action has a mouse hint.
	 * \param left mouse hint for left button
	 * \param right mouse hint for right button
	 */
	void updateMouseWidget(const QString& left=QString::null,
								   const QString& right=QString::null) override;
	void updateSelectionWidget(int num, double length) override;//updated for total number of selected, and total length of selected
	void commandMessage(const QString& message) override;

	static QString extToFormat(const QString& ext);
	void updateArcTangentialOptions(const double& d, bool byRadius) override;



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
	//! Pointer to the command line widget
	QG_CommandWidget* commandWidget;
	//! Pointer to arcTangential Option widge
	QG_ArcTangentialOptions* arcTangentialOptions;
	QG_PolylineEquidistantOptions* polylineEquidistantOptions;
private:
	// pointers to snap option widgets
	QG_SnapMiddleOptions* snapMiddleOptions;
	QG_SnapDistOptions* snapDistOptions;
	QG_ModifyOffsetOptions* modifyOffsetOptions;
	QG_PrintPreviewOptions* printPreviewOptions;
	QG_LineAngleOptions* m_pLineAngleOptions;
};

#endif
