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

#ifndef RS_MODIFICATION_H
#define RS_MODIFICATION_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include <QList>

#include "lc_copyutils.h"
#include "lc_ellipseoffsetgeometry.h"
#include "lc_offsetoutputbudget.h"
#include "lc_offsetcurveagain.h"
#include "lc_offsetcachetrim.h"
#include "rs_pen.h"
#include "rs_vector.h"

class RS_Arc;
class RS_AtomicEntity;
class RS_Entity;
enum class LC_OffsetCurveRefusal;
class RS_EntityContainer;
class RS_MText;
class RS_Text;
class RS_Line;
class RS_Insert;
class RS_Block;
class RS_Polyline;
class RS_Document;
class RS_Graphic;
class RS_GraphicView;
class LC_GraphicViewport;
class LC_OffsetWorkerCompletion;
class LC_OffsetBatchPreparation;
struct LC_OffsetPreviewSnapshot;
namespace LC_OffsetWorkerRequest { struct Request; }
class LC_SelectedSet;
class LC_UndoSection;
struct LC_DocumentModificationBatch;
struct LC_CurveOffsetMaterializationResult;
enum class LC_CurveOffsetSide;
enum class LC_CurveOffsetStatus;
namespace LC_OffsetCurveTransform { class Steps; }

struct LC_ModifyOperationFlags {
    int number = 0;
    bool useCurrentAttributes = false;
    bool useCurrentLayer = false;
    bool keepOriginals = false;
    bool multipleCopies = false;

    int obtainNumberOfCopies() const {
        int numberOfCopies = number;
        if (!multipleCopies) {
            numberOfCopies = 1;
        }
        else {
            if (numberOfCopies < 1) {
                numberOfCopies = 1;
            }
        }
        return numberOfCopies;
    }
};

/**
 * Holds the data needed for move modifications.
 */
struct RS_MoveData : LC_ModifyOperationFlags {
    RS_Vector offset;
};

struct LC_AlignRefData : LC_ModifyOperationFlags {
    bool scale = false;
    RS_Vector rotationCenter;
    RS_Vector offset;
    double rotationAngle = 0.0;
    double scaleFactor = 0.0;
};

struct RS_BoundData {
    RS_BoundData(const RS_Vector& left, const RS_Vector& top) : min{left}, max{top} {
    }

    RS_Vector min;
    RS_Vector max;

    RS_Vector getCenter() const {
        return (min + max) / 2;
    }
};

/**
 * Holds the data needed for offset modifications.
 */
struct RS_OffsetData : LC_ModifyOperationFlags {
    /**
     * An execution bound, not only a widget limit: saved settings and the
     * property sheet can ask for more copies than the options widget offers.
     */
    static constexpr int kMaximumOffsetCopies = 100;

    /**
     * The number of copies, clamped to [1, kMaximumOffsetCopies]. It hides the
     * shared helper, which Move, Rotate and Scale keep unbounded.
     */
    int obtainNumberOfCopies() const {
        return std::min(LC_ModifyOperationFlags::obtainNumberOfCopies(), kMaximumOffsetCopies);
    }

    RS_Vector coord;
    /**
     * Where coord lies on a curve, or on the segments it is drawn with, it
     * gives no side the user chose, and this point's side decides instead.
     * Modify > Offset with a distance taken from two clicks sets it to the
     * second one: its first click, the reference point, is where snapping
     * puts it, on the curve or on its drawing, and the second shows the side.
     */
    RS_Vector sideFallback{false};
    double distance = 0.;
    /** Programmatic requests opt in; Modify Offset loads its saved interactive default separately. */
    bool asOffsetCurve = false;
};

/**
 * Hard output limits of one offset request. They belong to the modification
 * layer; the geometry engine only sees the remaining budget of one source.
 */
struct LC_OffsetBatchLimits {
    LC_OffsetSourceBudget perSource = makeDefaultOffsetSourceBudget();
    std::size_t maxDeepEntitiesPerRequest = kDefaultOffsetDeepEntitiesPerRequest;
    /**
     * The offset engine's effort per copy of a spline: exact offset
     * evaluations and intersection box pairs. Zero keeps its defaults.
     */
    std::size_t maxSamples = 0;
    std::size_t maxIntersectionPairs = 0;

    /**
     * Limits for a preview, which is made again on every mouse move: an eighth
     * of the output and of the engine's effort. What does not fit is not
     * previewed; the commit uses the full limits.
     */
    static LC_OffsetBatchLimits preview();
};

/** Fixed source eligibility for owned geometry preparation, not a certificate.
 * nativeDefinition is present only for an editable, consistent native source.
 * A stored native cache may be an ordinary RS_Spline in the isolated helper;
 * any new native output still requires the existing producer/checker admission.
 */
struct LC_OffsetSourceContext {
    bool topLevel = false;
    bool nativeSource = false;
    std::optional<LC_EllipseOffsetDefinition> nativeDefinition;
};

enum class LC_OffsetSourceStatus {
    Succeeded,
    InvalidSource,
    NotVisibleOrLocked,
    /** The current layer was asked for, but it is missing, frozen or locked. */
    TargetLayerUnavailable,
    /** Nothing is left at the first distance: the source shrinks away, or a circle's radius would. */
    Vanished,
    OffsetFailed,
    LimitExceeded
};

/**
 * Why a source, or a copy of it, could not be offset, where the engine's
 * status does not say: a cause the pipeline knows before it offsets, or the
 * check of what it made.
 */
enum class LC_OffsetFailureDetail {
    None,
    /** A line with no length has no direction to offset along. */
    ZeroLength,
    /** An unbounded hyperbola has no finite offset. */
    Unbounded,
    /** A polyline with elliptic segments, which only lines and arcs may be. */
    EllipticSegments,
    /** A polyline whose offset would cross itself, come too near, run backwards or break apart. */
    PolylineNotTrimmed,
    /** A source two of whose pieces run over each other. */
    SourceRetraces,
    /** A distance of RS_TOLERANCE or less. */
    InvalidDistance,
    /** The reference point is on the entity, and no second click shows the side. */
    AmbiguousSide
};

enum class LC_OffsetPlainReason {
    NotTopLevel, SeveralPieces, CheckFailed, NotCacheRepresentable, OutputBudget,
    ZeroSum, RebaseRefused, UndefinedDefinition
};
struct LC_OffsetPlainOutcome {
    int copy = 0;
    LC_OffsetPlainReason reason = LC_OffsetPlainReason::NotCacheRepresentable;
    std::optional<LC_OffsetCheckResult::Reason> checkReason;
};

struct LC_OffsetRebaseOutcome {
    int copy = 0;
    std::optional<LC_OffsetCurveAgain::Refusal> proposalRefusal;
    std::optional<LC_OffsetCurveRefusal> nativeRefusal;
    std::optional<LC_CurveOffsetStatus> engineStatus;
    std::optional<LC_OffsetCheckResult::Reason> checkReason;
    LC_EllipseOffsetGeometry::CheckWork checkWork{};
    std::size_t engineRuns = 0;
    std::size_t sampleUnits = 0;
    /** Reserved per-query ceiling, not an observed or total intersection-box count. */
    std::size_t intersectionPairAllowance = 0;
    bool adopted = false;
};

struct LC_OffsetCacheReuse {
    int copy = 0;
    LC_OffsetSourceBudget budget{};
};

struct LC_OffsetSourceOutcome {
    /** Identity only: a source removed by a destructive offset must not be dereferenced. */
    const RS_Entity* source = nullptr;
    LC_OffsetSourceStatus status = LC_OffsetSourceStatus::InvalidSource;
    /**
     * Why the offset engine refused a spline, for OffsetFailed and LimitExceeded,
     * or the copy that stopped a series; Ok otherwise.
     */
    LC_CurveOffsetStatus engineStatus{};
    /** Why the pipeline refused the source, or the copy that stopped its series. */
    LC_OffsetFailureDetail detail = LC_OffsetFailureDetail::None;
    /**
     * Why a series stopped short of its copies: Vanished, nothing is left at
     * the next distance, or OffsetFailed, the next copy could not be made, for
     * the reason in engineStatus and detail. Succeeded while none is missing.
     */
    LC_OffsetSourceStatus stoppedBy = LC_OffsetSourceStatus::Succeeded;
    /** Owned by the batch once handed over. */
    QList<RS_Entity*> createdEntities;
    LC_OffsetOutputUsage usage{};
    /**
     * Copies made and asked for. A copy with nothing left, or one that cannot
     * be made for any reason but the output limits, ends the series, so fewer
     * may be made; the earlier copies are kept, and so is the source.
     */
    int copiesMade = 0;
    int copiesRequested = 0;
    /** One nonzero root count per completed copy, in source/copy order. */
    std::vector<std::size_t> copyOutputCounts;
    /** Actual engine side resolution; retained even if a later attempt refuses. */
    std::optional<LC_CurveOffsetSide> resolvedSide;
    /**
     * The source goes from the drawing with this offset. One that succeeded
     * and stays (originals kept, copies short, a polyline's segment) is left
     * for the caller to unselect.
     */
    bool sourceRemoved = false;
    int copiesKeptPlain = 0;
    /** Bounded by kMaximumOffsetCopies; describes committed visible copies only. */
    std::vector<LC_OffsetPlainOutcome> keptPlain;
    /** Attempted work remains observable even if a later copy discards this source's outputs. */
    std::size_t rebaseEngineRuns = 0, cacheEngineRuns = 0, reusedCacheCopies = 0;
    std::vector<LC_OffsetCacheReuse> reusedCopies;
    std::size_t sampleUnits = 0, intersectionPairAllowance = 0;
    /** Actual adopt() invocations and their returned C work, including refusals.
     * Additive counters sum across copies; depth is the maximum, not a sum.
     * An exception can lose the current check prefix: callers retain reservations.
     */
    std::size_t adoptionAttempts = 0;
    LC_EllipseOffsetGeometry::CheckWork adoptionCheckWork{};
    LC_OffsetOutputUsage attemptedOutputUsage{};
    bool attemptedDeepCountComplete = true;
    /** At most one structural proposal and producer attempt per requested copy. */
    std::vector<LC_OffsetRebaseOutcome> rebases;

    bool succeeded() const {
        return status == LC_OffsetSourceStatus::Succeeded;
    }

    /** Every copy asked for was made. */
    bool complete() const {
        return succeeded() && copiesMade == copiesRequested;
    }
};

/**
 * Spline offsets a preview made, by source, side and distance, so that moving
 * the mouse without changing them draws copies instead of offsetting again.
 * A result is only reused under a budget it fits: a success whose output fits
 * what is left, a refusal for want of budget under no larger a budget, and
 * any other refusal always. The sources must stay unchanged while it is used:
 * an action clears it when its selection or preview ends.
 */
class LC_OffsetPreviewCache {
public:
    LC_OffsetPreviewCache();
    ~LC_OffsetPreviewCache();
    LC_OffsetPreviewCache(const LC_OffsetPreviewCache&) = delete;
    LC_OffsetPreviewCache& operator=(const LC_OffsetPreviewCache&) = delete;

    /** A copy of a reusable result into @p out, if there is one. */
    bool find(const RS_Entity* source, LC_CurveOffsetSide side, double magnitude, const LC_OffsetSourceBudget& budget,
              LC_CurveOffsetMaterializationResult& out) const;
    /** Keeps a copy of @p result, made under @p budget. */
    void keep(const RS_Entity* source, LC_CurveOffsetSide side, double magnitude, const LC_OffsetSourceBudget& budget,
              const LC_CurveOffsetMaterializationResult& result);
    void clear();

    /** Owned, pointer-free ordinary entries, or refusal before unbounded copying. */
    std::unique_ptr<LC_OffsetPreviewSnapshot> snapshot(const RS_Entity* source) const;
    /** Detached helper cache only; replaces all local state atomically. */
    bool restore(const RS_Entity* source, const LC_OffsetPreviewSnapshot& snapshot);
    /** Apply a joined helper's update only while its captured cache revision is
     * current. The caller separately seals the live source and job generation.
     */
    bool merge(const RS_Entity* source, const LC_OffsetPreviewSnapshot& before,
               const LC_OffsetPreviewSnapshot& after);

private:
    struct Entry;
    std::map<std::tuple<const RS_Entity*, int, double>, std::unique_ptr<Entry>> m_entries;
    std::size_t m_otherEntries = 0;
    std::uint64_t m_revision = 0;
    bool m_cleared = false;
};

/** The result of an offset request per source, in the order the sources were given. */
struct LC_OffsetBatchOutcome {
    QList<LC_OffsetSourceOutcome> sources;

    bool anySourceSucceeded() const;
};

/**
 * Holds the data needed for rotation modifications.
 */
struct RS_RotateData : LC_ModifyOperationFlags {
    RS_Vector center;
    RS_Vector refPoint;
    double angle = 0.;
    double secondAngle = 0.0;
    bool secondAngleIsAbsolute = false;
    bool twoRotations = false;
};

/**
 * Holds the data needed for scale modifications.
 */
struct RS_ScaleData : LC_ModifyOperationFlags {
    RS_Vector referencePoint;
    RS_Vector factor = RS_Vector(1.1, 1.0, 0.0);
    // Find the factor by a source and a target point
    bool isotropicScaling = true;
    bool toFindFactor = false;
};

/**
 * Holds the data needed for mirror modifications.
 */
struct RS_MirrorData : LC_ModifyOperationFlags {
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};

/**
 * Holds the data needed for move/rotate modifications.
 */
struct RS_MoveRotateData : LC_ModifyOperationFlags {
    RS_Vector referencePoint;
    RS_Vector offset;
    double angle = 0.;
    bool sameAngleForCopies = false;
};

/**
 * Holds the data needed for rotation around two centers modifications.
 */
struct RS_Rotate2Data : LC_ModifyOperationFlags {
    RS_Vector center1;
    RS_Vector center2;

    double angle1 = 0.;
    double angle2 = 0.;
    bool mirrorAngles = false;
    bool sameAngle2ForCopies = false;
};

/**
 * Holds the data needed for beveling modifications.
 */
struct RS_BevelData {
    double length1 = 0.;
    double length2 = 0.;
    bool trim = false;
};

struct LC_TrimResult {
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    RS_Vector intersection1;
    RS_Vector intersection2;
    bool result = false;
    std::size_t nativeDemotions = 0;
    std::optional<LC_OffsetCacheTrim::Refusal> cacheRefusal;
    LC_OffsetCacheTrim::Work cacheWork;
};

struct LC_StoredCacheTrimOptions {
    LC_OffsetCacheTrim::Limits kernel;
    LC_OffsetCacheTrim::Cancellation cancellation;
    std::size_t maxOutputEntities = 4;
    std::size_t maxDisplayEntities = kDefaultOffsetDeepEntitiesPerSource;
};

/** Synchronous borrow for consuming already joined helper jobs, never stored.
 * Retain the original submitted requests and their owning Completion values.
 * The caller must first validate its captured document/source/view/action seal,
 * including whether publicationUndoEpoch is unchanged or its own one-cycle
 * increment. This record cannot authorize a document or an external undo.
 * One op3 result serves a conic Trim Two; nonconic Trim Two consumes op2 twice
 * in source order, the second request using the exact debited remainder.
 */
struct LC_StoredCacheTrimCompletions {
    const LC_OffsetWorkerRequest::Request& firstRequest;
    const LC_OffsetWorkerCompletion& first;
    const LC_OffsetWorkerRequest::Request* secondRequest = nullptr;
    const LC_OffsetWorkerCompletion* second = nullptr;
    std::uint64_t publicationUndoEpoch = 0;
};

struct LC_BevelResult {
    RS_Vector intersectionPoint = RS_Vector(false);
    RS_Line* bevel = nullptr;
    RS_Polyline* polyline = nullptr;
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    int error = OK;
    bool trimStart1 = false;
    bool isPolyline = false;
    bool trimStart2 = false;

    enum {
        OK,
        ERR_NO_INTERSECTION,
        ERR_NOT_THE_SAME_POLYLINE,
        ERR_VISIBILITY,
        ERR_NOT_LINES
    };
};

struct LC_ModificationContext {
    QList<RS_Entity*>* entitiesOriginal{nullptr};
    QList<RS_Entity*> entitiesToCreate;
    QList<RS_Entity*> entitiesToDelete;

    explicit LC_ModificationContext(QList<RS_Entity*>& originals) : entitiesOriginal{&originals} {
    }
};

struct LC_RoundResult {
    enum TrimMode {
        TRIM_START,
        TRIM_END,
        TRIM_CIRCLE
    };

    bool isPolyline = false;
    RS_Arc* round = nullptr;
    RS_Entity* trimmed1 = nullptr;
    RS_Entity* trimmed2 = nullptr;
    RS_Polyline* polyline = nullptr;
    TrimMode trim1Mode = TRIM_START;
    TrimMode trim2Mode = TRIM_START;
    RS_Vector trimmingPoint1 = RS_Vector(false);
    RS_Vector trimmingPoint2 = RS_Vector(false);
    RS_Vector intersectionPoint = RS_Vector(false);
    bool trimmed{false};
    int error = OK;

    enum {
        OK,
        ERR_NO_INTERSECTION,
        NO_PARALLELS,
        ERR_INPUT
    };
};

/**
 * Holds the data needed for rounding modifications.
 */
struct RS_RoundData {
    double radius = 0.;
    bool trim = false;
};

/**
 * Holds the data needed for moving reference points.
 */
struct RS_MoveRefData {
    RS_Vector ref;
    RS_Vector offset;
};

/**
 * Holds the data needed for changing attributes.
 */
struct RS_AttributesData {
    QString layer;
    RS_Pen pen;
    bool changeLayer = false;
    bool changeColor = false;
    bool changeLineType = false;
    bool changeWidth = false;
    bool applyBlockDeep = false;
};

struct LC_LibraryInsertData : LC_CopyUtils::RS_PasteData {
    LC_LibraryInsertData(const RS_Vector& insertionPoint, double factor, double angle, const QString& blockName, RS_Graphic* source);
    //! Name of the block to create or an empty string to assign a new auto name.
    QString blockName;
    RS_Graphic* source;
};

/**
 * API Class for manipulating entities.
 * There's no interaction handled in this class.
 *
 * All modifications can be undone / redone if the container
 * is a RS_Graphic.
 *
 * This class is connected to an entity container and
 * can be connected to a graphic view.
 *
 * @author Andrew Mustun
 */
class RS_Modification {
public:
    static void revertDirection(QList<RS_Entity*>& originalEntities, LC_DocumentModificationBatch& ctx);
    static void doChangeEntityAttributes(RS_Entity* en, RS_Entity*& clone, const RS_AttributesData& data, QSet<RS_Block*>& blocks);
    static bool changeAttributes(const QList<RS_Entity*>& originalEntities, RS_AttributesData& data, LC_DocumentModificationBatch& ctx);
    static void doChangeBlockAttributes(const RS_Block* block, RS_AttributesData& data, QSet<QString>& processedBlockNames);

    static void libraryInsert(const LC_LibraryInsertData& data, RS_Graphic* destination, LC_DocumentModificationBatch& ctx);

    static void move(const RS_MoveData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                     LC_DocumentModificationBatch& ctx);

    static bool rotate(const RS_RotateData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                       LC_DocumentModificationBatch& ctx);

    static bool scale(const RS_ScaleData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                      LC_DocumentModificationBatch& ctx);

    static bool mirror(const RS_MirrorData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                       LC_DocumentModificationBatch& ctx);

    static bool moveRotate(const RS_MoveRotateData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                           LC_DocumentModificationBatch& ctx);

    static bool rotate2(const RS_Rotate2Data& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                        LC_DocumentModificationBatch& ctx);

    static LC_TrimResult trim(const RS_Vector& trimCoord, RS_AtomicEntity* trimEntity, const RS_Vector& limitCoord, RS_Entity* limitEntity,
                              bool both, LC_DocumentModificationBatch& ctx);
    /** Native-involved path only. Stages BOTH operands before a single batch
     * publication. Ordinary lines retain their type; conics retain exact stored
     * centers/axes/radii and use typed angular restrictions (circles become arcs).
     * Plain normalized degree-1/3 spline caches are also supported when a native
     * participates. Fit/rational/wrapped representations remain explicit refusals.
     * Ordinary-only Trim, Trim Amount and Cut retain their existing paths.
     */
    static LC_TrimResult trimStoredCache(RS_Document& document, RS_Entity& target, const RS_Vector& targetPick,
        RS_Entity& cutter, const RS_Vector& cutterPick, bool both, LC_DocumentModificationBatch& ctx,
        const LC_StoredCacheTrimOptions& options = {});

    /** Same admissions, attributes and final atomic handoff as trimStoredCache;
     * never calls the numeric kernel, including on missing/mismatched receipts.
     * Public decoded wire Result values are deliberately not accepted.
     */
    static LC_TrimResult trimStoredCachePrepared(RS_Document& document, RS_Entity& target,
        const RS_Vector& targetPick, RS_Entity& cutter, const RS_Vector& cutterPick, bool both,
        LC_DocumentModificationBatch& ctx, const LC_StoredCacheTrimCompletions& completed,
        const LC_StoredCacheTrimOptions& options = {});

    /** Initial lowered limits, or the exact remainder after a successful op2.
     * Numeric diagnostics only: this grants no publication or geometry authority.
     * A failed/fatal/timed-out job cannot supply a remainder for a second job.
     */
    static std::optional<LC_OffsetCacheTrim::Limits> storedCacheTrimRemainingLimits(
        const LC_OffsetCacheTrim::Limits& limits, const LC_OffsetWorkerCompletion* previous = nullptr) noexcept;

    static RS_Entity* trimAmount(const RS_Vector& trimCoord, RS_AtomicEntity* entityToTrim, double dist, bool trimBoth, bool& trimStart,
                                 bool& trimEnd, LC_DocumentModificationBatch& ctx);

    /** offsetWithOutcome() with default limits; true if any source was offset. */
    static bool offset(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                       LC_DocumentModificationBatch& ctx);

    /**
     * Offsets each source, once per identity in the order given, as one
     * transaction per source: all of its copies are added or none is, and only
     * a source whose copies were all added is queued for deletion when
     * originals are not kept. The one exception is a copy with nothing left, a
     * spline shrunk away or a circle or arc whose radius would vanish: it ends
     * the series, the copies before it are added, and the source is kept
     * (Vanished if that was the first copy). Deleted, hidden and locked
     * sources, and all sources when the requested current layer cannot take
     * entities, fail before anything is built. Splines go to the offset engine
     * directly, trimmed of what lies nearer to them than the distance, so a
     * spline it refuses is never retried by mutating a clone. Output is counted
     * against @p limits over all copies of a source and over the request; a
     * preview batch holds additions only. @p cache, if given, is consulted and
     * filled for splines.
     */
    static LC_OffsetBatchOutcome offsetWithOutcome(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList,
                                                   bool forPreviewOnly, const LC_OffsetBatchLimits& limits,
                                                   LC_DocumentModificationBatch& ctx,
                                                   LC_OffsetPreviewCache* cache = nullptr);

    /** Prepare one source's numeric copy series in caller-owned roots.
     * The caller has already admitted visibility, target layer, finite nonzero
     * distances and source/request limits. Pass empty roots and a fresh outcome.
     * Eligibility is fixed input; this entry never discovers document membership.
     * Sets copiesRequested and returns status; source identity, status assignment,
     * document/list handoff remain the caller's job. A refusal or exception
     * discards provisional roots/committed usage, retaining attempted-work data.
     * Uses the existing preview cache and unchanged generation/C admission paths.
     */
    static LC_OffsetSourceStatus prepareOffsetSource(const RS_OffsetData& data, RS_Entity& source,
        bool forPreviewOnly, const LC_OffsetBatchLimits& limits, std::size_t requestDeepLeft,
        LC_OffsetSourceContext sourceContext, std::vector<std::unique_ptr<RS_Entity>>& roots,
        LC_OffsetSourceOutcome& result, LC_OffsetPreviewCache* cache = nullptr);

    static bool cut(const RS_Vector& cutCoord, RS_AtomicEntity* cutEntity, LC_DocumentModificationBatch& ctx);

    // FIXME - sand - review & complete
    static bool stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset,
                        const QList<RS_Entity*>& entitiesList, bool removeOriginals, LC_DocumentModificationBatch& ctx);

    static LC_BevelResult bevel(const RS_Vector& coord1, RS_AtomicEntity* entity1, const RS_Vector& coord2, RS_AtomicEntity* entity2,
                                RS_BevelData& data, bool previewOnly, LC_DocumentModificationBatch& ctx);

    static LC_RoundResult round(const RS_Vector& coord, const RS_Vector& coord1, RS_AtomicEntity* entity1, const RS_Vector& coord2,
                                RS_AtomicEntity* entity2, RS_RoundData& data, LC_DocumentModificationBatch& ctx);

    static bool explode(const QList<RS_Entity*>& entitiesList, LC_DocumentModificationBatch& ctx);

    static bool explodeTextIntoLetters(const QList<RS_Entity*>& selectedEntitiesList, LC_DocumentModificationBatch& ctx);

    // fixme - review what for this method is used? action is not enabled
    static bool splitPolyline(RS_Polyline* polyline, const RS_Entity& e1, const RS_Vector& v1, const RS_Entity& e2, const RS_Vector& v2,
                              RS_Polyline** polyline1, RS_Polyline** polyline2, LC_DocumentModificationBatch& ctx);

    static RS_Polyline* addPolylineNode(RS_Polyline* polyline, const RS_AtomicEntity& segment, const RS_Vector& node,
                                        LC_DocumentModificationBatch& ctx);
    static RS_Polyline* deletePolylineNode(RS_Polyline* polyline, const RS_Vector& node, LC_DocumentModificationBatch& ctx);
    // fixme - complete!!!
    static RS_Polyline* deletePolylineNodesBetween(RS_Polyline* polyline, const RS_Vector& node1, const RS_Vector& node2,
                                                   LC_DocumentModificationBatch& ctx);
    static RS_Polyline* polylineTrim(RS_Polyline* polyline, RS_AtomicEntity& segment1, RS_AtomicEntity& segment2,
                                     LC_DocumentModificationBatch& ctx);

    static RS_BoundData getBoundingRect(QList<RS_Entity*>& selected);
    static bool alignRef(const LC_AlignRefData& data, const QList<RS_Entity*>& entitiesList, bool forPreviewOnly,
                         LC_DocumentModificationBatch& ctx);

private:
    friend class LC_OffsetBatchPreparation;
    static LC_OffsetBatchOutcome offsetWithOutcomeImpl(const RS_OffsetData&, const QList<RS_Entity*>&,
        bool, const LC_OffsetBatchLimits&, LC_DocumentModificationBatch&, LC_OffsetPreviewCache*,
        LC_OffsetBatchPreparation*);
    static LC_TrimResult trimStoredCacheImpl(RS_Document& document, RS_Entity& target,
        const RS_Vector& targetPick, RS_Entity& cutter, const RS_Vector& cutterPick, bool both,
        LC_DocumentModificationBatch& ctx, const LC_StoredCacheTrimOptions& options,
        const LC_StoredCacheTrimCompletions* completed);
    static bool trimAtomicByEnding(RS_AtomicEntity* atomicToTrim, const RS_Vector& trimPoint, RS2::Ending ending);
    static bool doExplodeTextIntoLetters(const RS_MText* text, LC_DocumentModificationBatch& ctx);
    static bool doExplodeTextIntoLetters(RS_Text* text, LC_DocumentModificationBatch& ctx);

    /** Null context preserves the synchronous adapter's live eligibility reads
     * at their original callback boundaries; copies were frozen before preflight.
     */
    static LC_OffsetSourceStatus prepareOffsetSourceImpl(const RS_OffsetData& data, RS_Entity& source,
        bool forPreviewOnly, const LC_OffsetBatchLimits& limits, std::size_t requestDeepLeft,
        std::vector<std::unique_ptr<RS_Entity>>& roots, LC_OffsetSourceOutcome& result,
        LC_OffsetPreviewCache* cache, const LC_OffsetSourceContext* sourceContext, int numberOfCopies);

protected:
    static void trimEnding(const RS_Vector& trimCoord, RS_AtomicEntity* trimmed1, const RS_Vector& is);
    static LC_RoundResult::TrimMode roundingTrimEntity(const RS_VectorSolutions& entitiesIntersection, RS_AtomicEntity*& entityToTrim,
                                                       const RS_Arc* arc, const RS_Vector& trimPoint, const RS_Vector& selectionPoint1,
                                                       const RS_Vector& selectionPoint2);

    static RS_Entity* getClone(bool forPreviewOnly, const RS_Entity* e);
    static RS_Entity* getTransformedClone(bool forPreviewOnly, const RS_Entity* e,
        const LC_OffsetCurveTransform::Steps& steps, bool forcePlain = false);
    static void selectClone(const RS_Entity* original, RS_Entity* clone);
};

#endif
