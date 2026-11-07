/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEEDITSNAPSHOT_H
#define LC_OFFSETCURVEEDITSNAPSHOT_H

#include <cstddef>
#include <memory>
#include <QList>

class LC_OffsetCurve;
class RS_Document;
class RS_Entity;
class RS_Layer;

/** Bounded source-value baseline, shared by native sheet and dialog edits.
 * Capture before cloning. Callers must prove live membership/owner/context
 * before capture or matches; this value object does not own the source entity.
 */
class LC_OffsetCurveEditSnapshot final {
public:
    static constexpr std::size_t MaximumSnapshotItems = 4096;
    static constexpr std::size_t MaximumSnapshotBytes = 1024*1024;
    static std::unique_ptr<LC_OffsetCurveEditSnapshot> capture(const LC_OffsetCurve& curve);
    ~LC_OffsetCurveEditSnapshot();
    bool matches(const LC_OffsetCurve& curve) const;
    /** No virtual display reads. Use after every callbackful batch read and
     * renewed live membership checks, before handing off prepared replacements.
     * Bounded value copies may still throw; this does not own the source.
     */
    bool matchesStoredState(const LC_OffsetCurve& curve) const;
    LC_OffsetCurveEditSnapshot(const LC_OffsetCurveEditSnapshot&) = delete;
    LC_OffsetCurveEditSnapshot& operator=(const LC_OffsetCurveEditSnapshot&) = delete;
private:
    struct Data;
    explicit LC_OffsetCurveEditSnapshot(std::unique_ptr<Data> data) noexcept;
    std::unique_ptr<Data> m_data;
};

/** Offset creation's owned numeric inputs. Live membership is proved before
 * every retained address is read; no live entity/layer is owned by this value.
 * Keep this object alive for the whole ordered preparation, including preview
 * cache entries bound to its frozen sources. Never publish those source copies.
 */
class LC_OffsetCreationSources final {
public:
    static constexpr std::size_t MaximumNodes = 16384;
    static constexpr std::size_t MaximumBytes = 16*1024*1024;
    static std::shared_ptr<LC_OffsetCreationSources> capture(RS_Document&, const QList<RS_Entity*>&);
    /** Capture a direct document target and a detached, action-owned limit.
     * The caller must keep the exact detached object alive until the snapshot is
     * destroyed, and prove that ownership before calling matchesDetached().
     */
    static std::shared_ptr<LC_OffsetCreationSources> captureDetachedLimit(
        RS_Document&, RS_Entity& target, RS_Entity& detachedLimit);
    /** The caller must prove the nested limit is a live owned document child
     * before capture and before every matchesNested() call. */
    static std::shared_ptr<LC_OffsetCreationSources> captureNestedLimit(
        RS_Document&, RS_Entity& target, RS_Entity& nestedLimit);
    ~LC_OffsetCreationSources();
    bool matches(const RS_Document&) const;
    bool matchesDetached(const RS_Document&, const RS_Entity* ownedLimit) const;
    bool matchesNested(const RS_Document&, const RS_Entity* liveLimit) const;
    /** Return an independently owned root for a worker, detached from the
     * snapshot's private document. Index follows sources() order. */
    std::unique_ptr<RS_Entity> cloneWorkerSource(std::size_t index) const;
    const QList<RS_Entity*>& sources() const noexcept;
    const QList<RS_Entity*>& originals() const noexcept;
    const QList<RS_Entity*>& roots() const noexcept;
    RS_Entity* original(const RS_Entity* frozen) const noexcept;
    bool needsWorker() const noexcept;
    /** Rebind only private output layers, not source copies/cache identities. */
    bool localize(const QList<RS_Entity*>& outputs, RS_Layer* activeLayer, bool useActiveLayer) const;
private:
    struct Data;
    bool appendLimit(RS_Entity&, bool nested);
    bool matchesImpl(const RS_Document&, const RS_Entity* ownedLimit) const;
    explicit LC_OffsetCreationSources(std::unique_ptr<Data>);
    std::unique_ptr<Data> m_data;
};
#endif
