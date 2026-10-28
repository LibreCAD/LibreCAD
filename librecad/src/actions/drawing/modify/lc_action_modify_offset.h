/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

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

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef RS_ACTIONMODIFYOFFSET_H
#define RS_ACTIONMODIFYOFFSET_H


#include <cstddef>
#include <memory>

#include "lc_action_modify_base.h"
#include "lc_selectedsetlistener.h"
#include <QPointer>

struct RS_OffsetData;
struct LC_OffsetBatchOutcome;
struct LC_OffsetSourceOutcome;
struct LC_OffsetPlainOutcome;
class LC_OffsetPreviewCache;
class LC_OffsetWorkerClient;
class LC_OffsetCreationSources;
class QTimer;
class RS_GraphicView;

/**
 * This action class create entity by offset
 *
 * @author Dongxu Li
 */
class LC_ActionModifyOffset : public LC_ActionModifyBase, private LC_SelectedSetListener {
    Q_OBJECT
public:
    explicit LC_ActionModifyOffset(LC_ActionContext *actionContext);
    ~LC_ActionModifyOffset() override;
    void init(int status) override;
    void trigger() override;
    void suspend() override;
    void keyPressEvent(QKeyEvent*) override;
    bool isPreparing() const noexcept;
    double getDistance() const;
    void setDistance(double distance) const;
    bool isFixedDistance() const {return m_distanceIsFixed;}
    void setDistanceFixed(bool value);
    bool isEditableOffsetCurve() const;
    void setEditableOffsetCurve(bool enabled) const;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint,
        SetPosition       /**< Setting the direction of offset*/
    };

    bool m_distanceIsFixed = true;
    RS_Vector m_referencePoint = RS_Vector(false);
    std::unique_ptr<RS_OffsetData> m_offsetData;
    /** Keep committed source diagnostics through selection and completion messages. */
    std::unique_ptr<LC_OffsetBatchOutcome> m_pendingOutcome;
    /** Offsets the preview made, for the sources it made them for. */
    std::unique_ptr<LC_OffsetPreviewCache> m_previewCache;
    QList<RS_Entity*> m_previewSources;
    std::unique_ptr<LC_OffsetBatchOutcome> m_workerReport;

    void previewOffset();
    /** Why a source was not offset, for the command line and the info cursor. */
    static QString failureReason(const LC_OffsetSourceOutcome& source, bool preview);
    /** Why a source that was offset got fewer copies than asked for. */
    static QString stopReason(const LC_OffsetSourceOutcome& source);
    /** Why an offset, or the copy that stopped a series, could not be made. */
    static QString offsetFailureReason(const LC_OffsetSourceOutcome& source);
    /** Committed native-request fallback only; never dereferences source identity pointers. */
    static QString keptPlainReason(const LC_OffsetSourceOutcome& source, const LC_OffsetPlainOutcome& plain);
    static QString keptPlainMessage(const LC_OffsetBatchOutcome& outcome);
    /** Appearance/MaxPreview, read signed: a non-positive value means none in detail. */
    static std::size_t maxPreviewDetail();
    void finish() override;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void updateActionPromptForSelection() override;
    void updateActionPromptForSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    /** A typed distance fixes it, as clicking the options widget's field would. */
    bool doProcessCommand(int status, const QString& command) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
private:
    friend class LC_OffsetCreationActionTestAccess;
    struct Work;
    bool currentContext() const;
    bool selectionNeedsWorker() const;
    bool currentWork(const std::shared_ptr<Work>&) const;
    void beginPreparation(bool preview);
    void advance(const std::shared_ptr<Work>&);
    void completePreparation(const std::shared_ptr<Work>&);
    void cancelPreparation(const QString& reason = {});
    void detachContext();
    void selectionChanged() override {}
    void selectedSetDestroyed() override;
    QPointer<RS_GraphicView> m_liveView;
    std::unique_ptr<LC_OffsetWorkerClient> m_worker;
    std::unique_ptr<QTimer> m_guardTimer;
    std::shared_ptr<Work> m_work;
    std::shared_ptr<LC_OffsetCreationSources> m_sourceInputs;
    qint64 m_serial = 0, m_requestId = 0;
};
#endif
