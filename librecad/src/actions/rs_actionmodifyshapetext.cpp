/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Doug Geiger (noneyabiz@mail.wasent.cz)
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

#include "rs_actionmodifyshapetext.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_information.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"

RS_ActionModifyShapeText::RS_ActionModifyShapeText(RS_EntityContainer& container,
	RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Shape Text",
		container, graphicView)
	, textEntity{ nullptr }
	, shapeEntity{ nullptr }
	, shapePreview{ nullptr }
{
	offset = 0.5;
	actionType=RS2::ActionModifyShapeText;
}

RS_ActionModifyShapeText::~RS_ActionModifyShapeText() {
	if (graphicView != nullptr && graphicView->isCleanUp() == false) {
		setShapeTextPreviewEntity(nullptr, RS_Vector());
	}
}

void RS_ActionModifyShapeText::init(int status) {

	snapMode.clear();
	snapMode.restriction = RS2::RestrictNothing;
	RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyShapeText::trigger() {

	RS_DEBUG->print("RS_ActionModifyShapeText::trigger()");

	if (textEntity)
	{
		RS_Modification m(*container, graphicView);
		RS_Entity* parent = catchEntity(insertionPoint, RS2::ResolveNone);
		if (parent && !parent->isAtomic()) {
			parent->setSelected(true);
			m.explode();
			shapeEntity = catchEntity(insertionPoint, RS2::ResolveNone);
		}
		if (shapeEntity->isAtomic())
			m.shapeText(insertionPoint, reinterpret_cast<RS_AtomicEntity*>(shapeEntity), textEntity, offset);		

		textEntity = nullptr;
		shapeEntity = nullptr;
		setShapeTextPreviewEntity(nullptr, RS_Vector());		
		setStatus(ChooseTextEntity);
		RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(), container->totalSelectedLength());
	}
}

void RS_ActionModifyShapeText::mouseMoveEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_ActionModifyShapeText::mouseMoveEvent begin");

	RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
	RS_Entity* se = catchEntity(e, RS2::ResolveAll);
	RS_Entity* parent = catchEntity(e, RS2::ResolveNone);


	switch (getStatus()) {
	case ChooseShapeEntity:
		if (se != textEntity && parent != textEntity && (parent && parent->rtti() != RS2::EntityText && parent->rtti() != RS2::EntityMText && parent->rtti() != RS2::EntityAlignedText))
		{
			setShapeTextPreviewEntity(se, mouse);
			insertionPoint = mouse;
			shapeEntity = se;
		}
		break;

	default:
		break;
	}

	RS_DEBUG->print("RS_ActionModifyShapeText::mouseMoveEvent end");
}

void RS_ActionModifyShapeText::mouseReleaseEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {

		RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
		RS_Entity* se = catchEntity(e, RS2::ResolveAll);
		RS_Entity* parent = catchEntity(e, RS2::ResolveNone);

		switch (getStatus()) {
		case ChooseTextEntity:
			if (parent && (parent->rtti() == RS2::EntityText || parent->rtti() == RS2::EntityMText || parent->rtti() == RS2::EntityAlignedText))
			{
				textEntity = parent;
				textEntity->setHighlighted(true);
				setStatus(ChooseShapeEntity);
			}
			break;
		case ChooseShapeEntity:
			if (se != textEntity && parent != textEntity && (parent && parent->rtti() != RS2::EntityText && parent->rtti() != RS2::EntityMText && parent->rtti() != RS2::EntityAlignedText))
			{
				textEntity->setHighlighted(false);
				insertionPoint = mouse;
				shapeEntity = parent;
				if (shapeEntity /*&& trimEntity->isAtomic()*/)
					trigger();
			}
			break;
		default:
			break;
		}
	}
	else if (e->button() == Qt::RightButton) {
		setShapeTextPreviewEntity(nullptr, RS_Vector());
		init(getStatus() - 1);
	}
}

void RS_ActionModifyShapeText::updateMouseButtonHints() {
	switch (getStatus()) {
	case ChooseTextEntity:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select text entity"),
			tr("Back"));
		break;
	case ChooseShapeEntity:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select entity to shape text to"),
			tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionModifyShapeText::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::SelectCursor);
}

void RS_ActionModifyShapeText::setShapeTextPreviewEntity(RS_Entity * entity, const RS_Vector & point)
{
	if (shapePreview) {
		deletePreview();
		delete shapePreview;
		shapePreview = nullptr;
	}

	if (entity && entity->isAtomic()) {
		RS_Modification modify(*container, graphicView);
		modify.shapeText(point, reinterpret_cast<RS_AtomicEntity*>(entity), textEntity, offset, &shapePreview);		
	}
	drawShapeTextPreview();
}

void RS_ActionModifyShapeText::drawShapeTextPreview()
{
	deletePreview();
	if (shapePreview) {
		RS_Entity* e = shapePreview->clone();
		e->setHighlighted(true);
		preview->setHighlighted(true);
		preview->addEntity(e);
	}
	drawPreview();
}

void RS_ActionModifyShapeText::showOptions() {
    RS_ActionInterface::showOptions();
	
    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyShapeText::hideOptions() {
    RS_ActionInterface::hideOptions();
	
	RS_DIALOGFACTORY->requestOptions(this, false);
}

double RS_ActionModifyShapeText::getOffset() const
{
	return (offset);
}

void RS_ActionModifyShapeText::setOffset(double _offset)
{
	offset = _offset;
}
