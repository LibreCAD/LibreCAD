/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETIMPORTSESSION_H
#define LC_OFFSETIMPORTSESSION_H

#include <QObject>
#include <QTimer>
#include <functional>
#include <memory>
#include <vector>

#include "lc_offsetcurvexdata.h"
#include "rs_graphic.h"
#include "rs_spline.h"
#include "../offset_worker/lc_offsetworkerclient.h"

struct LC_OffsetImportDiagnostic {
    // Reason codes are from the named parser/worker enum; Validation detail is
    // the optional cache-check reason (-1 when absent). Negative reasons are
    // local admission/materialization failures.
    enum class Kind { XData, Carrier, Budget, Validation, Worker, Timeout };
    Kind kind = Kind::Budget;
    int reason = 0;
    int detail = 0;
    std::size_t item = 0;
};

struct LC_OffsetImportBudget {
    static constexpr std::size_t MaximumCarriers = 1024;
    static constexpr std::size_t MaximumBytes = 64u * 1024u * 1024u;
    std::size_t carriers = 0, bytes = 0, refused = 0;
    std::size_t suppressedDiagnostics = 0;
    std::vector<LC_OffsetImportDiagnostic> diagnostics;
    void report(LC_OffsetImportDiagnostic::Kind, int reason = 0, std::size_t item = 0, int detail = 0);
    bool admit(const RS_SplineData&, const LC_OffsetCurveXData::Definition&, std::size_t metadataItems);
};

/** Private carrier used only inside an unpublished import drawing. It keeps no
 * parser pointer and survives the existing XREF clone/redirect path. */
class LC_OffsetImportPlaceholder final : public RS_Spline {
public:
    struct Carrier {
        RS_SplineData cache;
        LC_OffsetCurveXData::Definition definition;
        double tolerance = 0;
    };
    LC_OffsetImportPlaceholder(const RS_Spline& fallback, std::shared_ptr<const Carrier> carrier)
        : RS_Spline(fallback), m_carrier(std::move(carrier)) {}
    RS_Entity* clone() const override { return new LC_OffsetImportPlaceholder(*this); }
    const Carrier& carrier() const noexcept { return *m_carrier; }
private:
    std::shared_ptr<const Carrier> m_carrier;
};

/** Owns an unpublished drawing until every staged offset carrier is resolved.
 * Parsing is still synchronous on the owning thread; native cache validation is
 * asynchronous. Cancellation suppresses delivery and destroys the private graph.
 */
class LC_OffsetImportSession final : public QObject {
public:
    enum class Status { Complete, ReadFailed, ResourceLimit };
    struct Outcome {
        Status status = Status::ReadFailed;
        std::unique_ptr<RS_Graphic> graphic;
        QString error;
        bool partial = false;
        std::size_t admitted = 0, downgraded = 0;
        std::size_t suppressedDiagnostics = 0;
        std::vector<LC_OffsetImportDiagnostic> diagnostics;
    };
    using Callback = std::function<void(Outcome)>;
    explicit LC_OffsetImportSession(QObject* parent = nullptr);
    ~LC_OffsetImportSession() override;
    bool start(const QString& file, RS2::FormatType type, RS2::DxfTextEncoding encoding,
        bool acceptPartial, Callback callback);
    void cancel();
    bool busy() const noexcept;
private:
    friend class LC_OffsetImportSessionTestAccess;
    struct Work;
    void parse(const std::shared_ptr<Work>&);
    void advance(const std::shared_ptr<Work>&);
    void consume(const std::shared_ptr<Work>&, std::unique_ptr<LC_OffsetWorkerCompletion>);
    void finish(const std::shared_ptr<Work>&, Status);
    LC_OffsetWorkerClient m_worker;
    QTimer m_deadline;
    std::shared_ptr<Work> m_work;
    qint64 m_generation = 0, m_request = 0;
};
#endif
