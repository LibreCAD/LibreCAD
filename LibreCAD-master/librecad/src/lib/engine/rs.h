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

#ifndef RS_H
#define RS_H

//#define RS_TEST

// Windoze XP can't handle the original MAX/MINDOUBLE's
#define RS_MAXDOUBLE 1.0E+10
#define RS_MINDOUBLE -1.0E+10
//tolerance
#define RS_TOLERANCE 1.0e-10
//squared tolerance
#define RS_TOLERANCE15 1.5e-15
#define RS_TOLERANCE2 1.0e-20
#define RS_TOLERANCE_ANGLE 1.0e-8

/**
 * Class namespace for various enums along with some simple
 * wrapper methods for converting the enums to the Qt
 * counterparts.
 *
 * @author Andrew Mustun
 */
class RS2 {
public:

    /**
     * Flags.
     */
    enum Flags {
        /** Flag for Undoables. */
        FlagUndone      = 1<<0,
        /** Entity Visibility. */
        FlagVisible     = 1<<1,
        /** Entity attribute (e.g. color) is defined by layer. */
        FlagByLayer     = 1<<2,
        /** Entity attribute (e.g. color) defined by block. */
        FlagByBlock     = 1<<3,
        /** Layer frozen. */
        FlagFrozen      = 1<<4,
        /** Layer frozen by default. */
        FlagDefFrozen   = 1<<5,
        /** Layer locked. */
        FlagLocked      = 1<<6,
        /** Used for invalid pens. */
        FlagInvalid     = 1<<7,
        /** Entity in current selection. */
        FlagSelected    = 1<<8,
        /** Polyline closed? */
        FlagClosed      = 1<<9,
        /** Flag for temporary entities (e.g. hatch) */
        FlagTemp        = 1<<10,
        /** Flag for processed entities (optcontour) */
        FlagProcessed   = 1<<11,
        /** Startpoint selected */
        FlagSelected1   = 1<<12,
        /** Endpoint selected */
        FlagSelected2   = 1<<13,
                /** Entity is highlighted temporarily (as a user action feedback) */
                FlagHighlighted = 1<<14
    };

    /**
     * Variable types used by RS_VariableDict and RS_Variable.
     */
    enum VariableType {
        VariableString,
        VariableInt,
        VariableDouble,
        VariableVector,
        VariableVoid
    };

    /**
     * File types. Used by file dialogs. Note: the extension might not
     * be enough to distinguish file types.
     */
    enum FormatType {
        FormatUnknown,       /**< Unknown / unsupported format. */
        FormatDXF1,          /**< QCad 1 compatibility DXF format. */
        FormatDXFRW,           /**< DXF format. v2007. */
        FormatDXFRW2004,           /**< DXF format. v2004. */
        FormatDXFRW2000,           /**< DXF format. v2000. */
        FormatDXFRW14,           /**< DXF format. v14. */
        FormatDXFRW12,           /**< DXF format. v12. */
#ifdef DWGSUPPORT
        FormatDWG,           /**< DWG format. */
#endif
        FormatLFF,           /**< LibreCAD Font File format. */
        FormatCXF,           /**< CAM Expert Font format. */
        FormatJWW,           /**< JWW Format type */
        FormatJWC            /**< JWC Format type */
    };

    /**
     * Entity types returned by the rtti() method
     */
    enum EntityType {
        EntityUnknown,      /**< Unknown */
        EntityContainer,    /**< Container */
        EntityBlock,        /**< Block (Group definition) */
        EntityFontChar,     /**< Font character */
        EntityInsert,       /**< Insert (Group instance) */
        EntityGraphic,      /**< Graphic with layers */
        EntityPoint,        /**< Point */
        EntityLine,         /**< Line */
        EntityPolyline,     /**< Polyline */
        EntityVertex,       /**< Vertex (part of a polyline) */
        EntityArc,          /**< Arc */
        EntityCircle,       /**< Circle */
        EntityEllipse,      /**< Ellipse */
        EntityHyperbola,      /**< Hyperbola */
        EntitySolid,        /**< Solid */
        EntityConstructionLine, /**< Construction line */
        EntityMText,         /**< Multi-line Text */
        EntityText,         /**< Single-line Text */
        EntityDimAligned,   /**< Aligned Dimension */
        EntityDimLinear,    /**< Linear Dimension */
        EntityDimRadial,    /**< Radial Dimension */
        EntityDimDiametric, /**< Diametric Dimension */
        EntityDimAngular,   /**< Angular Dimension */
        EntityDimLeader,    /**< Leader Dimension */
        EntityHatch,        /**< Hatch */
        EntityImage,        /**< Image */
        EntitySpline,       /**< Spline */
        EntitySplinePoints,       /**< SplinePoints */
        EntityOverlayBox,    /**< OverlayBox */
        EntityPreview,    /**< Preview Container */
        EntityPattern,
        EntityOverlayLine
    };


    /**
     * Action types used by action factories.
     */
    enum ActionType {
        ActionNone,        /**< Invalid action id. */

        ActionDefault,

        ActionFileNew,
        ActionFileNewTemplate,
        ActionFileOpen,
        ActionFileSave,
        ActionFileSaveAs,
        ActionFileExport,
        ActionFileClose,
        ActionFilePrint,
        ActionFilePrintPDF,
        ActionFilePrintPreview,
        ActionFileExportMakerCam,
        ActionFileQuit,

        ActionEditKillAllActions,
        ActionEditUndo,
        ActionEditRedo,
        ActionEditCut,
        ActionEditCutNoSelect,
        ActionEditCopy,
        ActionEditCopyNoSelect,
        ActionEditPaste,
        ActionOrderNoSelect,
        ActionOrderBottom,
        ActionOrderLower,
        ActionOrderRaise,
        ActionOrderTop,

        ActionViewStatusBar,
        ActionViewLayerList,
        ActionViewBlockList,
        ActionViewCommandLine,
        ActionViewLibrary,

        ActionViewPenToolbar,
        ActionViewOptionToolbar,
        ActionViewCadToolbar,
        ActionViewFileToolbar,
        ActionViewEditToolbar,
        ActionViewSnapToolbar,

        ActionViewGrid,
        ActionViewDraft,

        ActionZoomIn,
        ActionZoomOut,
        ActionZoomAuto,
        ActionZoomWindow,
        ActionZoomPan,
        ActionZoomRedraw,
        ActionZoomPrevious,

        ActionSelect,
        ActionSelectSingle,
        ActionSelectContour,
        ActionSelectWindow,
        ActionDeselectWindow,
        ActionSelectAll,
        ActionDeselectAll,
        ActionSelectIntersected,
        ActionDeselectIntersected,
        ActionSelectInvert,
        ActionSelectLayer,
        ActionSelectDouble,
        ActionGetSelect,

        ActionDrawArc,
        ActionDrawArc3P,
        ActionDrawArcParallel,
        ActionDrawArcTangential,
        ActionDrawCircle,
        ActionDrawCircle2P,
        ActionDrawCircle2PR,
        ActionDrawCircle3P,
        ActionDrawCircleCR,
        ActionDrawCircleParallel,
        ActionDrawCircleInscribe,
        ActionDrawCircleTan2_1P,
        ActionDrawCircleTan1_2P,
        ActionDrawCircleTan2,
        ActionDrawCircleTan3,

        ActionDrawEllipseArcAxis,
        ActionDrawEllipseAxis,
        ActionDrawEllipseFociPoint,
        ActionDrawEllipse4Points,
        ActionDrawEllipseCenter3Points,
        ActionDrawEllipseInscribe,

        ActionDrawHatch,
        ActionDrawHatchNoSelect,
        ActionDrawImage,
        ActionDrawLine,
        ActionDrawLineAngle,
        ActionDrawLineBisector,
        ActionDrawLineFree,
        ActionDrawLineHorVert,
        ActionDrawLineHorizontal,
        ActionDrawLineOrthogonal,
        ActionDrawLineOrthTan,
        ActionDrawLineParallel,
        ActionDrawLineParallelThrough,
        ActionDrawLinePolygonCenCor,
        ActionDrawLinePolygonCenTan,//add by txmy
        ActionDrawLinePolygonCorCor,
        ActionDrawLineRectangle,
        ActionDrawLineRelAngle,
        ActionDrawLineTangent1,
        ActionDrawLineTangent2,
        ActionDrawLineVertical,
        ActionDrawMText,
        ActionDrawPoint,
        ActionDrawSpline,
        ActionDrawSplinePoints, //interpolation spline
        ActionDrawPolyline,
        ActionDrawText,

        ActionPolylineAdd,
        ActionPolylineAppend,
        ActionPolylineDel,
        ActionPolylineDelBetween,
        ActionPolylineTrim,
        ActionPolylineEquidistant,
        ActionPolylineSegment,

        ActionDimAligned,
        ActionDimLinear,
        ActionDimLinearVer,
        ActionDimLinearHor,
        ActionDimRadial,
        ActionDimDiametric,
        ActionDimAngular,
        ActionDimLeader,

        ActionModifyAttributes,
        ActionModifyAttributesNoSelect,
        ActionModifyDelete,
        ActionModifyDeleteNoSelect,
        ActionModifyDeleteQuick,
        ActionModifyDeleteFree,
        ActionModifyMove,
        ActionModifyMoveNoSelect,
        ActionModifyRotate,
        ActionModifyRotateNoSelect,
        ActionModifyScale,
        ActionModifyScaleNoSelect,
        ActionModifyMirror,
        ActionModifyMirrorNoSelect,
        ActionModifyMoveRotate,
        ActionModifyMoveRotateNoSelect,
		ActionModifyRevertDirection,
		ActionModifyRevertDirectionNoSelect,
        ActionModifyRotate2,
        ActionModifyRotate2NoSelect,
        ActionModifyEntity,
        ActionModifyTrim,
        ActionModifyTrim2,
        ActionModifyTrimAmount,
        ActionModifyCut,
        ActionModifyStretch,
        ActionModifyBevel,
        ActionModifyRound,
        ActionModifyOffset,
        ActionModifyOffsetNoSelect,

        ActionSnapFree,
        ActionSnapGrid,
        ActionSnapEndpoint,
        ActionSnapOnEntity,
        ActionSnapCenter,
        ActionSnapMiddle,
        ActionSnapDist,
        ActionSnapIntersection,
        ActionSnapIntersectionManual,

        ActionRestrictNothing,
        ActionRestrictOrthogonal,
        ActionRestrictHorizontal,
        ActionRestrictVertical,

        ActionSetRelativeZero,
        ActionLockRelativeZero,
        ActionUnlockRelativeZero,

        ActionInfoInside,
        ActionInfoDist,
        ActionInfoDist2,
        ActionInfoAngle,
        ActionInfoTotalLength,
        ActionInfoTotalLengthNoSelect,
        ActionInfoArea,

        ActionLayersDefreezeAll,
        ActionLayersFreezeAll,
        ActionLayersUnlockAll,
        ActionLayersLockAll,
        ActionLayersAdd,
        ActionLayersRemove,
        ActionLayersEdit,
        ActionLayersToggleView,
        ActionLayersToggleLock,
        ActionLayersTogglePrint,
        ActionLayersToggleConstruction,

        ActionBlocksDefreezeAll,
        ActionBlocksFreezeAll,
        ActionBlocksAdd,
        ActionBlocksRemove,
        ActionBlocksAttributes,
        ActionBlocksEdit,
        ActionBlocksSave,
        ActionBlocksInsert,
        ActionBlocksToggleView,
        ActionBlocksCreate,
        ActionBlocksCreateNoSelect,
        ActionBlocksExplode,
        ActionBlocksExplodeNoSelect,
        ActionBlocksImport,

        ActionModifyExplodeText,
        ActionModifyExplodeTextNoSelect,

        ActionLibraryInsert,

        ActionOptionsGeneral,
        ActionOptionsDrawing,

        ActionToolRegenerateDimensions,

        ActionScriptOpenIDE,
        ActionScriptRun,

        /** Needed to loop through all actions */
        ActionLast
    };

    /**
    * Entity ending. Used for returning which end of an entity is meant.
     */
    enum Ending {
        EndingStart,    /**< Start point. */
        EndingEnd,      /**< End point. */
        EndingNone      /**< Neither. */
    };

    /**
     * Update mode for non-atomic entities that need to be updated when
     * they change. e.g. texts, inserts, ...
     */
    enum UpdateMode {
        NoUpdate,       /**< No automatic updates. */
        Update,         /**< Always update automatically when modified. */
                PreviewUpdate   /**< Update automatically but only for previews (quick update) */
    };

    /**
     * Drawing mode.
     */
    enum DrawingMode {
        ModeFull,       /**< Draw everything always detailed (default) */
        ModeAuto,       /**< Draw details when reasonable */
        ModePreview,    /**< Draw only in black/white without styles */
                ModeBW          /**< Black/white. Can be used for printing. */
    };

    /**
     * Undoable rtti.
     */
    enum UndoableType {
        UndoableUnknown,    /**< Unknown undoable */
        UndoableEntity,     /**< Entity */
        UndoableLayer       /**< Layer */
    };

    /**
     * Units.
     */
    enum Unit {
        None = 0,               /**< No unit (unit from parent) */
        Inch = 1,               /**< Inch */
        Foot = 2,               /**< Foot: 12 Inches */
        Mile = 3,               /**< Mile: 1760 Yards = 1609 m */
        Millimeter = 4,         /**< Millimeter: 0.001m */
        Centimeter = 5,         /**< Centimeter: 0.01m */
        Meter = 6,              /**< Meter */
        Kilometer = 7,          /**< Kilometer: 1000m */
        Microinch = 8,          /**< Microinch: 0.000001 */
        Mil = 9,                /**< Mil = 0.001 Inch*/
        Yard = 10,              /**< Yard: 3 Feet */
        Angstrom = 11,          /**< Angstrom: 10^-10m  */
        Nanometer = 12,         /**< Nanometer: 10^-9m  */
        Micron = 13,            /**< Micron: 10^-6m  */
        Decimeter = 14,         /**< Decimeter: 0.1m */
        Decameter = 15,         /**< Decameter: 10m */
        Hectometer = 16,        /**< Hectometer: 100m */
        Gigameter = 17,         /**< Gigameter: 1000000m */
        Astro = 18,             /**< Astro: 149.6 x 10^9m */
        Lightyear = 19,         /**< Lightyear: 9460731798 x 10^6m */
        Parsec = 20,            /**< Parsec: 30857 x 10^12 */

        LastUnit = 21           /**< Used to iterate through units */
    };


    /**
     * Format for length values.
     */
    enum LinearFormat {
        /** Scientific (e.g. 2.5E+05) */
        Scientific,
        /** Decimal (e.g. 9.5)*/
        Decimal,
        /** Engineering (e.g. 7' 11.5")*/
        Engineering,
        /** Architectural (e.g. 7'-9 1/8")*/
        Architectural,
        /** Fractional (e.g. 7 9 1/8) */
        Fractional,
        /** Metric Architectural using DIN 406 (e.g. 1.12⁵)*/
        ArchitecturalMetric
    };

    /**
     * Angle Units.
     */
    enum AngleUnit {
        Deg,               /**< Degrees */
        Rad,               /**< Radians */
        Gra                /**< Gradians */
    };

    /**
     * Display formats for angles.
     */
    enum AngleFormat {
        /** Degrees with decimal point (e.g. 24.5�) */
        DegreesDecimal,
        /** Degrees, Minutes and Seconds (e.g. 24�30'5'') */
        DegreesMinutesSeconds,
        /** Gradians with decimal point (e.g. 390.5)*/
        Gradians,
        /** Radians with decimal point (e.g. 1.57)*/
        Radians,
        /** Surveyor's units */
        Surveyors
    };

    /**
     * Enum of levels of resolving when iterating through an entity tree.
     */
    enum ResolveLevel {
        /** Groups are not resolved */
        ResolveNone,
        /**
        * Resolve all but not Inserts.
        */
        ResolveAllButInserts,
        /**
         * Resolve all but not Text or MText.
         */
        ResolveAllButTexts,
        /**
         * Resolve no text or images, added as a quick fix for bug#422
         */
        ResolveAllButTextImage,
        /**
         * all Entity Containers are resolved
         * (including Texts, Polylines, ...)
         */
        ResolveAll
    };

    /**
     * Direction used for scrolling actions.
     */
    enum Direction {
        Up, Left, Right, Down
    };

    /**
     * Vertical alignments.
     */
//    enum VAlign {
//        VAlignTop,      /**< Top. */
//        VAlignMiddle,   /**< Middle */
//        VAlignBottom    /**< Bottom */
//    };

    /**
     * Horizontal alignments.
     */
//    enum HAlign {
//        HAlignLeft,     /**< Left */
//        HAlignCenter,   /**< Centered */
//        HAlignRight     /**< Right */
//    };

    /**
     * Text drawing direction.
     */
//    enum TextDrawingDirection {
//        LeftToRight,     /**< Left to right */
//        TopToBottom,     /**< Top to bottom */
//        ByStyle          /**< Inherited from associated text style */
//    };

    /**
     * Line spacing style for texts.
     */
//    enum TextLineSpacingStyle {
//        AtLeast,        /**< Taller characters will override */
//        Exact           /**< Taller characters will not override */
//    };

    /**
     * Leader path type.
     */
    enum LeaderPathType {
        Straight,      /**< Straight line segments */
        Spline         /**< Splines */
    };

    /**
     * Direction for zooming actions.
     */
    enum ZoomDirection {
        In, Out
    };

    /**
     * Axis specification for zooming actions.
     */
    enum Axis {
        OnlyX, OnlyY, Both
    };

    /**
     * Crosshair type
     */
    enum CrosshairType {
        LeftCrosshair,         /**< Left type isometric Crosshair */
        TopCrosshair,         /**< Top type isometric Crosshair */
        RightCrosshair,         /**< Right type isometric Crosshair */
        OrthogonalCrosshair         /**< Orthogonal Crosshair */
    };

    /**
     * Snapping modes
     */
    enum SnapMode {
        SnapFree,         /**< Free positioning */
        SnapGrid,         /**< Snap to grid points */
        SnapEndpoint,     /**< Snap to endpoints */
        SnapMiddle,       /**< Snap to middle points */
        SnapCenter,       /**< Snap to centers */
        SnapOnEntity,     /**< Snap to the next point on an entity */
        SnapDist,         /**< Snap to points with a distance to an endpoint */
        SnapIntersection, /**< Snap to intersection */
        SnapIntersectionManual  /**< Snap to intersection manually */
    };

    /**
     * Snap restrictions
     */
    enum SnapRestriction {
        RestrictNothing,        /**< No restriction to snap mode */
        RestrictHorizontal,     /**< Restrict to 0,180 degrees */
        RestrictVertical,       /**< Restrict to 90,270 degrees */
        RestrictOrthogonal      /**< Restrict to 90,180,270,0 degrees */
    };

    /**
     * Enum of line styles:
     */
    enum LineType {
        LineByBlock = -2,      /**< Line type defined by block not entity */
        LineByLayer = -1,     /**< Line type defined by layer not entity */
        NoPen = 0,            /**< No line at all. */
        SolidLine = 1,        /**< Normal line. */

        DotLine = 2,          /**< Dotted line. */
        DotLineTiny = 3,          /**< Dotted line tiny */
        DotLine2 = 4,         /**< Dotted line small. */
        DotLineX2 = 5,        /**< Dotted line large. */

        DashLine = 6,         /**< Dashed line. */
        DashLineTiny=7,       /**< Dashed line tiny */
        DashLine2 = 8,        /**< Dashed line small. */
        DashLineX2 = 9,       /**< Dashed line large. */

        DashDotLine = 10,      /**< Alternate dots and dashes. */
        DashDotLineTiny = 11,      /**< Alternate dots and dashes tiny. */
        DashDotLine2 = 12,     /**< Alternate dots and dashes small. */
        DashDotLineX2 = 13,   /**< Alternate dots and dashes large. */

        DivideLine = 14,      /**< dash, dot, dot. */
        DivideLineTiny = 15,      /**< dash, dot, dot, tiny */
        DivideLine2 = 16,     /**< dash, dot, dot small. */
        DivideLineX2 = 17,    /**< dash, dot, dot large. */

        CenterLine = 18,      /**< dash, small dash. */
        CenterLineTiny = 19,      /**< dash, small dash tiny */
        CenterLine2 = 20,     /**< dash, small dash small. */
        CenterLineX2 = 21,    /**< dash, small dash large. */

        BorderLine = 22,      /**< dash, dash, dot. */
        BorderLineTiny = 23,      /**< dash, dash, dot tiny */
        BorderLine2 = 24,     /**< dash, dash, dot small. */
        BorderLineX2 = 25,    /**< dash, dash, dot large. */

        LineTypeUnchanged=26      /**< Line type defined by block not entity */
    };

    /**
     * Enum of line widths:
     */
    enum LineWidth {
        Width00 = 0,       /**< Width 1.  (0.00mm) */
        Width01 = 5,       /**< Width 2.  (0.05mm) */
        Width02 = 9,       /**< Width 3.  (0.09mm) */
        Width03 = 13,      /**< Width 4.  (0.13mm) */
        Width04 = 15,      /**< Width 5.  (0.15mm) */
        Width05 = 18,      /**< Width 6.  (0.18mm) */
        Width06 = 20,      /**< Width 7.  (0.20mm) */
        Width07 = 25,      /**< Width 8.  (0.25mm) */
        Width08 = 30,      /**< Width 9.  (0.30mm) */
        Width09 = 35,      /**< Width 10. (0.35mm) */
        Width10 = 40,      /**< Width 11. (0.40mm) */
        Width11 = 50,      /**< Width 12. (0.50mm) */
        Width12 = 53,      /**< Width 13. (0.53mm) */
        Width13 = 60,      /**< Width 14. (0.60mm) */
        Width14 = 70,      /**< Width 15. (0.70mm) */
        Width15 = 80,      /**< Width 16. (0.80mm) */
        Width16 = 90,      /**< Width 17. (0.90mm) */
        Width17 = 100,     /**< Width 18. (1.00mm) */
        Width18 = 106,     /**< Width 19. (1.06mm) */
        Width19 = 120,     /**< Width 20. (1.20mm) */
        Width20 = 140,     /**< Width 21. (1.40mm) */
        Width21 = 158,     /**< Width 22. (1.58mm) */
        Width22 = 200,     /**< Width 23. (2.00mm) */
        Width23 = 211,     /**< Width 24. (2.11mm) */
        WidthByLayer = -1, /**< Line width defined by layer not entity. */
        WidthByBlock = -2, /**< Line width defined by block not entity. */
        WidthDefault = -3  /**< Line width defaults to the predefined line width. */
    };

    /**
     * Wrapper for Qt
     */
    /*
       static int qw(RS2::LineWidth w) {
           switch (w) {
           case Width00:
               return 1;  // 0 is more accurate but quite slow
               break;
           case WidthByLayer:
           case WidthByBlock:
           case WidthDefault:
               return 1;
               break;
           case Width01:
           case Width02:
           case Width03:
           case Width04:
           case Width05:
           case Width06:
           case Width07:
           case Width08:
               return 1;
               break;
           case Width09:
           case Width10:
               return 3;
               break;
           case Width11:
               return 4;
               break;
           case Width12:
           case Width13:
               return 5;
               break;
           case Width14:
               return 6;
               break;
           case Width15:
               return 7;
               break;
           case Width16:
               return 8;
               break;
           case Width17:
               return 9;
               break;
           case Width18:
           case Width19:
               return 10;
               break;
           case Width20:
               return 12;
               break;
           case Width21:
           case Width22:
           case Width23:
           //case Width24:
               return 14;
               break;
           default:
               return (int)w;
               break;
           }
           return (int)w;
       }
    */

    /**
     * Wrapper for Qt
     */
	static LineWidth intToLineWidth(int w);

    /**
     * Enum of cursor types.
     */
    enum CursorType {
        ArrowCursor,          /**< ArrowCursor - standard arrow cursor. */
        UpArrowCursor,        /**< UpArrowCursor - upwards arrow. */
        CrossCursor,          /**< CrossCursor - crosshair. */
        WaitCursor,           /**< WaitCursor - hourglass/watch. */
        IbeamCursor,          /**< IbeamCursor - ibeam/text entry. */
        SizeVerCursor,        /**< SizeVerCursor - vertical resize. */
        SizeHorCursor,        /**< SizeHorCursor - horizontal resize. */
        SizeBDiagCursor,      /**< SizeBDiagCursor - diagonal resize (/). */
        SizeFDiagCursor,      /**< SizeFDiagCursor - diagonal resize (\). */
        SizeAllCursor,        /**< SizeAllCursor - all directions resize. */
        BlankCursor,          /**< BlankCursor - blank/invisible cursor. */
        SplitVCursor,         /**< SplitVCursor - vertical splitting. */
        SplitHCursor,         /**< SplitHCursor - horziontal splitting. */
        PointingHandCursor,   /**< PointingHandCursor - a pointing hand. */
        ForbiddenCursor,      /**< ForbiddenCursor - a slashed circle. */
        WhatsThisCursor,      /**< WhatsThisCursor - an arrow with a ?. */
        OpenHandCursor,       /**< Qt OpenHandCursor */
        ClosedHandCursor,       /**< Qt ClosedHandCursor */
        CadCursor,            /**< CadCursor - a bigger cross. */
        DelCursor,            /**< DelCursor - cursor for choosing entities */
        SelectCursor,         /**< SelectCursor - for selecting single entities */
        MagnifierCursor,      /**< MagnifierCursor - a magnifying glass. */
        MovingHandCursor      /**< Moving hand - a little flat hand. */
    };

    /**
     * Wrapper for Qt.
     */
	/*
    static Qt::CursorShape rsToQtCursorType(RS2::CursorType t) {
        switch (t) {
        case ArrowCursor:
            return Qt::ArrowCursor;
        case UpArrowCursor:
            return Qt::UpArrowCursor;
        case CrossCursor:
            return Qt::CrossCursor;
        case WaitCursor:
            return Qt::WaitCursor;
        case IbeamCursor:
            return Qt::IBeamCursor;
        case SizeVerCursor:
            return Qt::SizeVerCursor;
        case SizeHorCursor:
            return Qt::SizeHorCursor;
        case SizeBDiagCursor:
            return Qt::SizeBDiagCursor;
        case SizeFDiagCursor:
            return Qt::SizeFDiagCursor;
        case SizeAllCursor:
            return Qt::SizeAllCursor;
        case BlankCursor:
            return Qt::BlankCursor;
        case SplitVCursor:
            return Qt::SplitVCursor;
        case SplitHCursor:
            return Qt::SplitHCursor;
        case PointingHandCursor:
            return Qt::PointingHandCursor;
        case OpenHandCursor:
            return Qt::OpenHandCursor;
        case ClosedHandCursor:
            return Qt::ClosedHandCursor;
        case ForbiddenCursor:
            return Qt::ForbiddenCursor;
        case WhatsThisCursor:
            return Qt::WhatsThisCursor;
        default:
            return Qt::ArrowCursor;
        }
    }
*/
    /**
     * Paper formats.
     */
    enum PaperFormat {
        Custom,
                Letter,
                Legal,
                Executive,
        A0,
                A1,
                A2,
                A3,
                A4,
                A5,
                A6,
                A7,
                A8,
                A9,
        B0,
                B1,
                B2,
                B3,
                B4,
                B5,
                B6,
                B7,
                B8,
                B9,
                B10,
        C5E,
                Comm10E,
        DLE,
                Folio,
                Ledger,
                Tabloid,
        Arch_A,
        Arch_B,
        Arch_C,
        Arch_D,
        Arch_E,
        Arch_E1,
        Arch_E2,
        Arch_E3,

                NPageSize
        };

        /**
         * Items that can be put on a overlay, the items are rendered in this order. Best is to leave snapper as last so
         * it always shows up
         */
        enum OverlayGraphics {
                ActionPreviewEntity, // Action Entities
                Snapper // Snapper
        };

        //Different re-draw methods to speed up rendering of the screen
        enum RedrawMethod {
                RedrawNone = 0,
                RedrawGrid = 1,
                RedrawOverlay = 2,
                RedrawDrawing = 4,
                RedrawAll = 0xffff
        };

        /**
         * Text drawing direction.
         */
        enum TextLocaleDirection {
            locLeftToRight,     /** Left to right **/
            locRightToLeft      /** Right to Left **/
        };

};

#endif
