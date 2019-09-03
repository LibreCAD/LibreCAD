/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
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

#include "rs_actionmodifytrimexcess.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_information.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"

RS_ActionModifyTrimExcess::RS_ActionModifyTrimExcess(RS_EntityContainer& container,
	RS_GraphicView& graphicView, bool both)
	:RS_PreviewActionInterface("Trim Entity",
		container, graphicView)
	, trimEntity{ nullptr }
	, trimPreview{ nullptr }
{
}

RS_ActionModifyTrimExcess::~RS_ActionModifyTrimExcess() {
	if (graphicView != nullptr && graphicView->isCleanUp() == false) {
		setTrimPreviewEntity(nullptr, RS_Vector());
	}
}

void RS_ActionModifyTrimExcess::init(int status) {

	snapMode.clear();
	snapMode.restriction = RS2::RestrictNothing;
	RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyTrimExcess::trigger() {

	RS_DEBUG->print("RS_ActionModifyTrimExcess::trigger()");

	if (trimEntity && trimEntity->isAtomic()) {
		RS_Modification m(*container, graphicView);
		RS_Entity* parent = catchEntity(trimCoord, RS2::ResolveNone);
		if (parent && !parent->isAtomic()) {
			parent->setSelected(true);
			m.explode();
			trimEntity = catchEntity(trimCoord, RS2::ResolveNone);
		}
		if (trimEntity->isAtomic())
			m.trimExcess(trimCoord, (RS_AtomicEntity*)trimEntity);

		trimEntity = nullptr;
		setTrimPreviewEntity(nullptr, RS_Vector());		
		setStatus(ChooseTrimEntity);
		RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(), container->totalSelectedLength());
	}
}

void RS_ActionModifyTrimExcess::mouseMoveEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_ActionModifyTrimExcess::mouseMoveEvent begin");

	RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
	RS_Entity* se = catchEntity(e, RS2::ResolveAll);

	switch (getStatus()) {
	case ChooseTrimEntity:
		setTrimPreviewEntity(se, mouse);
		trimCoord = mouse;
		trimEntity = se;
		break;

	default:
		break;
	}

	RS_DEBUG->print("RS_ActionModifyTrimExcess::mouseMoveEvent end");
}

void RS_ActionModifyTrimExcess::mouseReleaseEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {

		RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
		RS_Entity* se = catchEntity(e, RS2::ResolveAll);

		switch (getStatus()) {
		case ChooseTrimEntity:
			trimCoord = mouse;
			trimEntity = se;
			if (trimEntity /*&& trimEntity->isAtomic()*/) {
				trigger();
			}
			break;

		default:
			break;
		}
	}
	else if (e->button() == Qt::RightButton) {
		setTrimPreviewEntity(nullptr, RS_Vector());
		init(getStatus() - 1);
	}
}

void RS_ActionModifyTrimExcess::updateMouseButtonHints() {
	switch (getStatus()) {
	case ChooseTrimEntity:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select entity to trim"),
			tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionModifyTrimExcess::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::SelectCursor);
}

void RS_ActionModifyTrimExcess::setTrimPreviewEntity(RS_Entity * entity, const RS_Vector & point)
{
	if (trimPreview) {
		deletePreview();
		delete trimPreview;
		trimPreview = nullptr;
	}

	if (entity && entity->isAtomic()) {
		RS_Modification modify(*container, graphicView);
		modify.trimExcess(point, reinterpret_cast<RS_AtomicEntity*>(entity), &trimPreview);		
	}
	drawTrimPreview();
}

void RS_ActionModifyTrimExcess::drawTrimPreview()
{
	deletePreview();
	if (trimPreview) {
		RS_Entity* e = trimPreview->clone();
		e->setHighlighted(true);
		preview->setHighlighted(true);
		preview->addEntity(e);
	}
	drawPreview();
}
