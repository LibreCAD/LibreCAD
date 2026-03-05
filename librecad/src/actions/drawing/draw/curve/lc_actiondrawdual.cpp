/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
//! File: lc_actiondrawdual.cpp
#include <QMouseEvent>

#include "lc_actiondrawdual.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "rs_entitycontainer.h"
#include "lc_quadraticutils.h"
#include "rs_pen.h"
#include "lc_undosection.h"

LC_ActionDrawDual::LC_ActionDrawDual(LC_ActionContext* context)
    : LC_ActionPreSelectionAwareBase("Draw Dual", context),
      m_center(false)
{
}

void LC_ActionDrawDual::init(int status) {
  setStatus(ChooseCenter);
  LC_ActionPreSelectionAwareBase::init(status);


  RS_DIALOGFACTORY->commandMessage(tr("Select Center"));

  updateMouseButtonHints();
}

void LC_ActionDrawDual::trigger() {
  if (!m_center.valid) {
    commandMessage(tr("No center specified."));
    m_center={0., 0.};
  }

  createDualForSelected();
  finish();
}

void LC_ActionDrawDual::mouseMoveEvent(QMouseEvent* e)
{
    drawSnapper();
}

void LC_ActionDrawDual::mouseReleaseEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    RS_Vector snap = snapPoint(e);

    if (getStatus() == ChooseCenter) {
      m_center = snap;
      trigger();  // Always proceed to creation after center is chosen
    }
  }
  else if (e->button() == Qt::RightButton) {
    finish();
  }
}

void LC_ActionDrawDual::coordinateEvent(RS_CoordinateEvent* e) {
  RS_Vector coord = e->getCoordinate();

  if (getStatus() == ChooseCenter) {
    m_center = coord;
    trigger();
  }
}

void LC_ActionDrawDual::createDualForSelected()
{
  // Collect all currently selected entities into a new temporary container
  //m_container->collectSelected(m_selectedEntities, false);  // true = deep/recursive selection

  if (m_selectedEntities.empty()) {
    commandMessage(tr("No entities selected. Dual creation cancelled."));
    finish();
    return;
  }

         // Create a group to hold all dual copies
  auto dualGroup = std::make_unique<RS_EntityContainer>(m_container, false);
  dualGroup->setLayer(m_container->getLayer());
  dualGroup->setPen(m_container->getPen());

  for (RS_Entity* e: m_selectedEntities) {

    if (!e->isVisible() || !e->isSelected()) continue;

           // Delegate dual creation to the utility function
    RS_Entity* dualCopy = LC_QuadraticUtils::createDualAroundCenter(e, m_center);

    if (dualCopy != nullptr) {
      if (dualCopy->isContainer()) {
        auto* dualContainer = static_cast<RS_EntityContainer*>(dualCopy);
        for (RS_Entity* dual: *dualContainer) {
          dualGroup->addEntity(dual);
        }
        dualContainer->setOwner(false);
        dualContainer->clear();
        delete dualCopy;
      } else {
          dualGroup->addEntity(dualCopy);
      }
      e->setSelected(false);
    }
  }

  if (dualGroup->count() == 0) {
    commandMessage(tr("No supported entities could be dualized."));
    return;
  }

         // Undo support (optional – already added in previous version)
  LC_UndoSection undo(m_document, m_viewport, true);
  for(RS_Entity* en: *dualGroup) {
    m_container->addEntity(en);
    en->reparent(m_container);
    undo.addUndoable(en);
  }

  m_container->calculateBorders();
  m_graphicView->redraw(RS2::RedrawDrawing);

  commandMessage(tr("Dual created around center (%1 entities)").arg(dualGroup->count()));
}

RS_Vector LC_ActionDrawDual::symmetricOf(const RS_Vector& p) const {
  return m_center * 2.0 - p;
}


void LC_ActionDrawDual::doTrigger(bool /*keepSelected*/) {
  createDualForSelected();
}

void LC_ActionDrawDual::updateMouseButtonHintsForSelection() {
  updateMouseWidgetTRCancel(tr("Select to create dual (Enter to complete)"), MOD_SHIFT_LC(tr("Select contour")));
}

void LC_ActionDrawDual::updateMouseButtonHints() {

}