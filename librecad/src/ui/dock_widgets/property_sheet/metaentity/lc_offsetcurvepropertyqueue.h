/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVEPROPERTYQUEUE_H
#define LC_OFFSETCURVEPROPERTYQUEUE_H

#include <QObject>
#include <QPointer>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "lc_offsetcurve.h"

class LC_ActionContext;
class RS_Document;
class RS_GraphicView;

/** Native-only ownership. Ordinary property delegates and their queue are unchanged. */
class LC_OffsetCurvePropertyQueue final : public QObject {
    Q_OBJECT
public:
    static constexpr std::size_t MaximumSnapshotItems = 4096;
    static constexpr std::size_t MaximumSnapshotBytes = 1024*1024;
    using Epoch = std::uint64_t;
    using Edit = std::function<LC_OffsetCurveEditResult(LC_OffsetCurve&, const LC_OffsetRunLimits&)>;
    struct Attempt {
        const RS_Entity* original = nullptr; // identity only, not retained ownership
        bool editInvoked = false; // callback ran; stage metrics below come only from its actual result
        LC_OffsetCurveEditResult result;
        QString contextRefusal;
    };

    explicit LC_OffsetCurvePropertyQueue(QObject* parent, LC_ActionContext* context);
    ~LC_OffsetCurvePropertyQueue() override;
    void setContext(RS_Document* document, RS_GraphicView* view);
    void cancel();
    void invalidateGetters();
    Epoch epoch() const { return m_epoch; }
    const LC_OffsetCurve* value(Epoch epoch, const RS_Entity* original) const;
    void apply(Epoch epoch, const QList<RS_Entity*>& originals, const Edit& edit);
    std::size_t pendingCount() const { return m_pending.size(); }
    const std::vector<Attempt>& lastAttempts() const { return m_lastAttempts; }
    void setRunLimits(const LC_OffsetRunLimits& limits) { m_limits = limits; }
    const LC_OffsetRunLimits& runLimits() const { return m_limits; }

signals:
    void finishEditors();
    void pendingCancelled();

private:
    struct SourceSnapshot;
    struct Pending {
        std::unique_ptr<LC_OffsetCurve> candidate;
        std::unique_ptr<SourceSnapshot> source;
    };
    bool currentContext() const;
    bool isLive(const RS_Entity* original) const;
    void scheduleCommit();
    void commit(Epoch contextSerial);
    void report() const;

    LC_ActionContext* m_context;
    RS_Document* m_document = nullptr; // sheet's destruction listeners invalidate this first
    QPointer<RS_GraphicView> m_view;
    Epoch m_epoch = 1; // provider/getter lifetime; refills do not own accepted edits
    Epoch m_contextSerial = 1; // pending ownership and scheduled commit lifetime
    bool m_scheduled = false;
    LC_OffsetRunLimits m_limits;
    std::map<const RS_Entity*, Pending> m_pending;
    std::vector<Attempt> m_lastAttempts;
};
#endif
