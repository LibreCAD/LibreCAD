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
#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include <QList>

#include "lc_copyutils.h"
#include "lc_offsetoutputbudget.h"
#include "rs_pen.h"
#include "rs_vector.h"

class RS_Arc;
class RS_AtomicEntity;
class RS_Entity;
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
class LC_SelectedSet;
class LC_UndoSection;
struct LC_DocumentModificationBatch;
struct LC_CurveOffsetMaterializationResult;
enum class LC_CurveOffsetSide;
enum class LC_CurveOffsetStatus;

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
     * Where coord lies on a curve it gives no side, and this point's side
     * decides instead. Modify > Offset with a distance taken from two clicks
     * sets it to the second one: its first click, the reference point, is
     * where snapping puts it, on the curve, and the second shows the side.
     */
    RS_Vector sideFallback{false};
    double distance = 0.;
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

struct LC_OffsetSourceOutcome {
    /** Identity only: a source removed by a destructive offset must not be dereferenced. */
    const RS_Entity* source = nullptr;
    LC_OffsetSourceStatus status = LC_OffsetSourceStatus::InvalidSource;
    /** Why the offset engine refused a spline, for OffsetFailed and LimitExceeded; Ok otherwise. */
    LC_CurveOffsetStatus engineStatus{};
    /** Owned by the batch once handed over. */
    QList<RS_Entity*> createdEntities;
    LC_OffsetOutputUsage usage{};
    /**
     * Copies made and asked for. A copy with nothing left ends the series, so
     * fewer may be made; the earlier copies are kept, and so is the source.
     */
    int copiesMade = 0;
    int copiesRequested = 0;

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

private:
    struct Entry;
    std::map<std::tuple<const RS_Entity*, int, double>, std::unique_ptr<Entry>> m_entries;
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
    static bool trimAtomicByEnding(RS_AtomicEntity* atomicToTrim, const RS_Vector& trimPoint, RS2::Ending ending);
    static bool doExplodeTextIntoLetters(const RS_MText* text, LC_DocumentModificationBatch& ctx);
    static bool doExplodeTextIntoLetters(RS_Text* text, LC_DocumentModificationBatch& ctx);

protected:
    static void trimEnding(const RS_Vector& trimCoord, RS_AtomicEntity* trimmed1, const RS_Vector& is);
    static LC_RoundResult::TrimMode roundingTrimEntity(const RS_VectorSolutions& entitiesIntersection, RS_AtomicEntity*& entityToTrim,
                                                       const RS_Arc* arc, const RS_Vector& trimPoint, const RS_Vector& selectionPoint1,
                                                       const RS_Vector& selectionPoint2);

    static RS_Entity* getClone(bool forPreviewOnly, const RS_Entity* e);
    static void selectClone(const RS_Entity* original, RS_Entity* clone);
};

#endif
