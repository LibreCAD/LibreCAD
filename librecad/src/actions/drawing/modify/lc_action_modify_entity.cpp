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

#include "lc_action_modify_entity.h"

#include <QPointer>
#include <utility>
#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "rs_document.h"
#include "rs_preview.h"
#include "lc_selectedset.h"
#include "lc_selectedsetlistener.h"

#include "lc_dlg_dimension.h"
#include "lc_entitypropertieseditor.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "qg_dlg_hatch.h"
#include "qg_dlg_mtext.h"
#include "qg_dlg_text.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_entity.h"
#include "rs_graphicview.h"
#include "rs_text.h"

namespace {
// Compare the candidate address only; never inspect a possibly removed context entity.
bool ownedDialogMember(const RS_EntityContainer& owner, const RS_Entity* candidate,
    std::size_t& remaining, unsigned depth = 0) {
    if (!owner.isOwner() || depth > 32) return false;
    for (const auto* child : owner) {
        if (!remaining) return false;
        --remaining;
        if (!child) continue;
        if (child == candidate) return true;
        if (const auto* nested = dynamic_cast<const RS_EntityContainer*>(child))
            if (ownedDialogMember(*nested, candidate, remaining, depth + 1)) return true;
    }
    return false;
}
}

struct LC_ActionModifyEntity::NativeDocumentLifetime final : LC_SelectedSetListener {
    LC_ActionModifyEntity& action;
    RS_Document* document;
    QPointer<RS_GraphicView> view;
    NativeDocumentLifetime(LC_ActionModifyEntity& owner, RS_Document* doc, RS_GraphicView* graphicView)
        : action(owner), document(doc), view(graphicView) {
        if (document) document->getSelection()->addListener(this);
    }
    void selectionChanged() override {}
    void selectedSetDestroyed() override {
        // The tracked registration has already been removed. The document is partly destroyed.
        document = nullptr;
        action.nativeDialogDocumentDestroyed();
    }
};

bool LC_ActionModifyEntity::nativeDialogDocumentIsCurrent() const {
    if (!m_nativeDocumentLifetime || !m_nativeDocumentLifetime->document || !m_actionContext) return false;
    const auto& lifetime = *m_nativeDocumentLifetime;
    return lifetime.document == m_document && lifetime.view && !lifetime.view->isClosing() &&
        lifetime.view.data() == m_graphicView && m_actionContext->getGraphicView() == lifetime.view.data() &&
        m_actionContext->getDocument() == lifetime.document && lifetime.view->getDocument() == lifetime.document &&
        lifetime.view->getViewPort() == m_viewport;
}

bool LC_ActionModifyEntity::dialogSourceIsMember(const RS_Entity* source) const {
    if (!source || !nativeDialogDocumentIsCurrent()) return false;
    std::size_t remaining = 65536;
    return m_document->findEntityIndex(source) >= 0 || ownedDialogMember(*m_document, source, remaining);
}

void LC_ActionModifyEntity::abandonDialogEdit() {
    discardNativeDialogClone(); // Also owns ordinary delayed-dialog sessions.
    m_allowExternalTermination = true;
    m_finished = true;
    setFinished(); // No traversal/redraw through a replaced document or viewport.
}

void LC_ActionModifyEntity::nativeDialogDocumentDestroyed() {
    // No reads of the dying document, source, layers, or viewport here.
    m_document = nullptr;
    m_graphic = nullptr;
    m_keyEntity = nullptr;
    if (m_preview) m_preview->setParent(nullptr);
    if (m_propertiesEditor && m_actionContext->getInteractiveInputInfo()->requestor == m_propertiesEditor.data())
        m_actionContext->interactiveInputRequestCancel();
    abandonDialogEdit();
}

struct LC_ActionModifyEntity::NativeDialogSource {
    RS_Document* document; // Identity only until the live view/context checks pass.
    QPointer<RS_GraphicView> view;
    LC_GraphicViewport* viewport;
    const RS_Entity* original; // Membership must precede every later dereference.
    unsigned long long documentId, sourceId;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;

    NativeDialogSource(RS_Document* doc, RS_GraphicView* graphicView, const LC_OffsetCurve& curve,
        std::unique_ptr<LC_OffsetCurveEditSnapshot> captured)
        : document(doc), view(graphicView), viewport(graphicView->getViewPort()), original(&curve),
          documentId(doc->getId()), sourceId(curve.getId()), snapshot(std::move(captured)) {}
};

bool LC_ActionModifyEntity::captureNativeDialogSource() {
    if (!nativeDialogDocumentIsCurrent() ||
        m_actionContext->getDocument() != m_document || m_actionContext->getGraphicView() != m_graphicView ||
        m_graphicView->isClosing() || m_graphicView->getDocument() != m_document ||
        m_graphicView->getViewPort() != m_viewport || !m_document->isOwner() || m_document->isLocked() ||
        m_document->findEntityIndex(m_entity) < 0) return false;
    const auto* curve = dynamic_cast<const LC_OffsetCurve*>(m_entity);
    if (!curve || curve->getParent() != m_document || !curve->isOwner() || curve->isDeleted() ||
        !curve->isVisible() || curve->isLocked()) return false;
    auto snapshot = LC_OffsetCurveEditSnapshot::capture(*curve);
    if (!snapshot) return false; // Full bounded baseline BEFORE the dialog's native clone.
    auto source = std::make_unique<NativeDialogSource>(m_document, m_graphicView, *curve, std::move(snapshot));
    m_nativeDialogSource = std::move(source);
    return true;
}

bool LC_ActionModifyEntity::nativeDialogSourceIsCurrent() const {
    if (!m_nativeDialogSource || !nativeDialogDocumentIsCurrent()) return false;
    const auto& source = *m_nativeDialogSource;
    if (!source.view || source.view->isClosing() || source.view.data() != m_graphicView ||
        m_actionContext->getGraphicView() != source.view.data() ||
        m_actionContext->getDocument() != source.document || source.view->getDocument() != source.document ||
        source.view->getViewPort() != source.viewport || m_viewport != source.viewport ||
        m_document != source.document || m_entity != source.original) return false;
    // The tracked selection lifetime, not a borrowed view pointer, vouches for the document.
    auto* document = source.view->getDocument();
    if (!document || document->getId() != source.documentId || !document->isOwner() || document->isLocked() ||
        document->findEntityIndex(source.original) < 0) return false;
    const auto* curve = dynamic_cast<const LC_OffsetCurve*>(source.original);
    return curve && curve->getId() == source.sourceId && curve->getParent() == document && curve->isOwner() &&
        !curve->isDeleted() && curve->isVisible() && !curve->isLocked() && source.snapshot->matches(*curve);
}

void LC_ActionModifyEntity::discardNativeDialogClone() {
    if (m_propertiesEditor) m_propertiesEditor->cancelGuardedEdit();
    if (m_pendingNativeClone) {
        if (m_pendingNativeClone->entity) m_pendingNativeClone->entity->setParent(nullptr);
        m_pendingNativeClone.reset(); // An executing modal retains its own session until unwind.
    }
    m_clonedEntity = nullptr;
    m_entity = nullptr;
    m_nativeDialogSource.reset();
}

void LC_ActionModifyEntity::finishNativeDialog(bool refused) {
    const bool liveContext = nativeDialogDocumentIsCurrent();
    const auto view = m_nativeDialogSource->view;
    auto* document = m_nativeDialogSource->document;
    if (refused) commandMessage(tr("Offset Curve: the source or editing context changed; the dialog edit was not applied."));
    const auto input = m_actionContext->getInteractiveInputInfo();
    if (input->requestor == m_propertiesEditor.data()) m_actionContext->interactiveInputRequestCancel();
    discardNativeDialogClone();
    m_allowExternalTermination = true;
    if (liveContext && view && !view->isClosing() && view.data() == m_graphicView &&
        m_actionContext->getGraphicView() == view.data() && m_actionContext->getDocument() == document &&
        view->getDocument() == document && view->getViewPort() == m_viewport) {
        if (m_invokedForSingleEntity) finish();
        else setStatus(ShowDialog);
        view->redraw(RS2::RedrawDrawing);
    } else {
        // Do not route finish()/redraw through a replaced or destroyed borrowed viewport.
        m_finished = true;
        setFinished();
    }
}

LC_ActionModifyEntity::LC_ActionModifyEntity(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionModifyEntity", actionContext, RS2::ActionModifyEntity),
      m_nativeDocumentLifetime(std::make_unique<NativeDocumentLifetime>(*this, m_document, m_graphicView)) {
}

LC_ActionModifyEntity::~LC_ActionModifyEntity() {
    if (m_nativeDialogSource || m_pendingNativeClone) discardNativeDialogClone();
    delete m_propertiesEditor.data();
}

void LC_ActionModifyEntity::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    m_entity = contextEntity;
    m_invokedForSingleEntity = true;
    m_modifyCursor = false;
}

void LC_ActionModifyEntity::init(const int status) {
    if (!nativeDialogDocumentIsCurrent()) return;
    if (mayInitWithContextEntity(status)) {
        auto* saved = m_actionContext->getContextMenuActionContextEntity();
        if (saved && !dialogSourceIsMember(saved)) {
            // Base init() clears/unselects the saved context. Drop an invalid address BEFORE it runs.
            m_actionContext->saveContextMenuActionContext(nullptr, RS_Vector(false), false);
            m_entity = nullptr;
            commandMessage(tr("The properties source is no longer in this document."));
        }
    }
    RS_PreviewActionInterface::init(status);
    if (m_entity != nullptr) {
        trigger();
    }
}

void LC_ActionModifyEntity::trigger() {
    if (!nativeDialogDocumentIsCurrent()) return;
    RS_PreviewActionInterface::trigger();
}

void LC_ActionModifyEntity::doTrigger() {
    if (!nativeDialogDocumentIsCurrent()) return;
    if (getStatus() == ShowDialog && m_entity && !dialogSourceIsMember(m_entity)) {
        m_entity = nullptr;
        commandMessage(tr("The properties source is no longer in this document."));
        return;
    }
    LC_UndoableDocumentModificationAction::doTrigger();
}

void LC_ActionModifyEntity::notifyFinished() const {
    if (nativeDialogDocumentIsCurrent()) m_graphicView->notifyLastActionFinished();
}

void LC_ActionModifyEntity::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (!nativeDialogDocumentIsCurrent() ||
        (m_pendingNativeClone && !m_nativeDialogSource && !dialogSourceIsMember(m_entity))) {
        if (m_nativeDialogSource) finishNativeDialog(true);
        else abandonDialogEdit();
        return;
    }
    if (m_nativeDialogSource) {
        if (shouldBeSkipped || !nativeDialogSourceIsCurrent()) {
            finishNativeDialog(!shouldBeSkipped);
            return;
        }
        trigger(); // No document transaction is started until the native guard passes.
        finishNativeDialog(false);
        return;
    }
    if (shouldBeSkipped) {
        if (m_pendingNativeClone) {
            m_pendingNativeClone.reset();
            m_clonedEntity = nullptr;
        }
        if (m_invokedForSingleEntity) {
            m_entity = nullptr;
            delete m_clonedEntity;
            m_clonedEntity = nullptr;
            finish();
        }
        else {
            setStatus(ShowDialog);
        }
    }
    else {
        trigger();
        if (m_invokedForSingleEntity) {
            finish();
        }
        else {
            setStatus(ShowDialog);
        }
    }
    if (m_propertiesEditor) m_propertiesEditor->cancelGuardedEdit();
    m_graphicView->redraw(RS2::RedrawDrawing);
    m_allowExternalTermination = true;
}

bool LC_ActionModifyEntity::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const int status = getStatus();
    if (status == ShowDialog) {
        if (m_entity != nullptr) {
            const bool selected = m_entity->isSelected();
            if (m_entity->rtti() == RS2::EntityOffsetCurve && !captureNativeDialogSource()) {
                commandMessage(tr("Offset Curve: a live, visible, unlocked document-owned source is required."));
                m_entity = nullptr;
                ctx.dontSetActiveLayerAndPen();
                return false;
            }
            // Native Cancel leaves even the original's selection/cache flags untouched.
            if (m_entity->rtti() != RS2::EntityOffsetCurve) select(m_entity);
            setStatus(InEditing);
            LC_EntityPropertiesDlg* editDialog{nullptr};
            QWidget* parent = QC_ApplicationWindow::getAppWindow().get();
            if (m_nativeDialogSource) {
                auto session = std::make_shared<LC_EntityPropertiesEditSession>();
                session->entity.reset(m_entity->clone());
                m_clonedEntity = session->entity.get();
                m_pendingNativeClone = std::move(session);
            } else {
                m_clonedEntity = m_entity->clone();
            }
            bool hasDialog = true;
            switch (m_entity->rtti()) {
                case RS2::EntityPoint:
                case RS2::EntityLine:
                case RS2::EntityArc:
                case RS2::EntityCircle:
                case RS2::EntityEllipse:
                case RS2::EntityOffsetCurve:
                case RS2::EntityHyperbola:
                case RS2::EntityParabola:
                case RS2::EntitySpline:
                case RS2::EntitySplinePoints:
                case RS2::EntityInsert:
                case RS2::EntityPolyline:
                case RS2::EntityImage:
                case RS2::EntityHatch:
                case RS2::EntityText:
                case RS2::EntityMText: {
                  // editing via delayed invocation in editor to support
                  // interactive input
                  if (!m_pendingNativeClone) {
                      // Only delayed editor cases use this owner; synchronous dimension dialogs stay unchanged.
                      std::unique_ptr<RS_Entity> clone(m_clonedEntity);
                      m_clonedEntity = nullptr;
                      auto session = std::make_shared<LC_EntityPropertiesEditSession>();
                      session->entity = std::move(clone);
                      m_clonedEntity = session->entity.get();
                      m_pendingNativeClone = std::move(session);
                  }
                  if (m_propertiesEditor == nullptr) {
                      m_propertiesEditor = new LC_EntityPropertiesEditor(m_actionContext, this);
                      m_propertiesEditor->setParent(this);
                      connect(m_propertiesEditor.data(), &QObject::destroyed, this, [this] {
                          m_propertiesEditor = nullptr;
                          if (m_nativeDialogSource) finishNativeDialog(false);
                          else if (m_pendingNativeClone) abandonDialogEdit();
                      });
                  }
                  m_allowExternalTermination = false;
                  if (m_nativeDialogSource) {
                      m_propertiesEditor->editEntity(parent, m_clonedEntity, m_viewport,
                          [this] { return nativeDialogSourceIsCurrent(); }, m_pendingNativeClone);
                  } else {
                      m_propertiesEditor->editEntity(parent, m_clonedEntity, m_viewport,
                          [this] { return dialogSourceIsMember(m_entity); }, m_pendingNativeClone);
                  }
                  hasDialog = false;
                  break;
                }
                case RS2::EntityDimAligned:
                case RS2::EntityDimAngular:
                case RS2::EntityDimDiametric:
                case RS2::EntityDimRadial:
                case RS2::EntityDimArc:
                case RS2::EntityDimOrdinate:
                case RS2::EntityDimLinear: {
                    editDialog = new LC_DlgDimension(parent, m_viewport, static_cast<RS_Dimension*>(m_clonedEntity));
                    break;
                }
                default:
                    hasDialog = false;
                    break;
            }

            if (hasDialog) {
                m_allowExternalTermination = false;
                if (editDialog->exec() != 0) {
                    editDialog->updateEntity();
                    ctx.replace(m_entity, m_clonedEntity);
                    if (!selected) {
                        m_clonedEntity->clearSelectionFlag();
                    }
                }
                else {
                    if (!selected) {
                        unselect(m_entity);
                    }
                    delete m_clonedEntity;
                }
                m_allowExternalTermination = true;
                m_graphicView->redraw(RS2::RedrawDrawing);
                setStatus(ShowDialog);
                delete editDialog;
            }
        }
        else {
            RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
        }
    }
    else if (status == InEditing) {
        if (m_nativeDialogSource && !nativeDialogSourceIsCurrent()) {
            ctx.dontSetActiveLayerAndPen();
            return false;
        }
        // Native per-field setters already publish a checked cache. Accept must
        // not regenerate it, including on attribute-only and same-value edits.
        if (m_clonedEntity->rtti() != RS2::EntityOffsetCurve) m_clonedEntity->update();
        m_clonedEntity->calculateBorders();
        m_clonedEntity->clearSelectionFlag();

        const unsigned long cloneEntityId = m_clonedEntity->getId();
        const unsigned long originalEntityId = m_entity->getId();

        ctx.replace(m_entity, m_clonedEntity);

        if (m_pendingNativeClone) {
            m_pendingNativeClone->entity.release(); // ownership passes to the established undo batch
            m_pendingNativeClone.reset();
            m_clonedEntity = nullptr;
        }

        // hm... probably there is a better way to notify (signal, broadcasting etc) without direct dependency?
        LC_QuickInfoWidget* entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
        if (entityInfoWidget != nullptr) {
            entityInfoWidget->onEntityPropertiesEdited(originalEntityId, cloneEntityId);
        }
        setStatus(EditComplete);
    }
    ctx.dontSetActiveLayerAndPen();
    return true;
}

void LC_ActionModifyEntity::doTriggerCompletion(const bool success) {
    LC_UndoableDocumentModificationAction::doTriggerCompletion(success);
}

void LC_ActionModifyEntity::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (status == ShowDialog) {
        const RS_Entity* entity = catchAndDescribe(e);
        if (entity != nullptr) {
            highlightHoverWithRefPoints(entity, true);
        }
    }
}

void LC_ActionModifyEntity::onMouseLeftButtonRelease([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (status == ShowDialog) {
        m_entity = catchEntityByEvent(e);
        if (m_entity != nullptr) {
            trigger();
        }
    }
}

void LC_ActionModifyEntity::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

RS2::CursorType LC_ActionModifyEntity::doGetMouseCursor([[maybe_unused]] int status) {
    if (m_modifyCursor) {
        return RS2::SelectCursor;
    }
    return RS2::NoCursorChange;
}

void LC_ActionModifyEntity::updateActionPrompt() {
    updatePromptTRCancel(tr("Click on entity to modify"));
}
