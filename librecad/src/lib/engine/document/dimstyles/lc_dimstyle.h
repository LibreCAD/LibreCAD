/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
 ******************************************************************************/

#ifndef LC_DIMSTYLE_H
#define LC_DIMSTYLE_H

#include "rs_color.h"
#include "rs_graphic.h"


class LC_DimStyle{
   public:
    LC_DimStyle();
    void init();
    LC_DimStyle(const QString &name);

    static QString STANDARD_DIM_STYLE;
    static QString NAME_SEPARATOR;

    class ModificationAware: public RS_Flags {
    public:
        enum CheckFlagMode {
           ALL,
           SET,
           UNSET
        };
        bool checkModifyState(unsigned f);
        CheckFlagMode getModifyCheckMode() const {return m_checkModificationMode;}
        void setModifyCheckMode(CheckFlagMode mode) {m_checkModificationMode = mode;}
    protected:
        void checkModified(int newValue, int currentValue, unsigned flag);
        void checkModified(short newValue, short currentValue, unsigned flag);
        void checkModified(const QString &newValue, const QString& currentValue, unsigned flag);
        void checkModified(bool newValue, bool currentValue, unsigned flag);
        void checkModified(double newValue, double currentValue, unsigned flag);
        void checkModified(const RS_Color& newValue, const RS_Color& currentValue, unsigned int flag);
        void copyFlags(ModificationAware* c);
    private:
        CheckFlagMode m_checkModificationMode{SET};
    };

    class Text: public ModificationAware {
    public:
        enum Fields: unsigned{
            $DIMATFIT = 1 << 0,
            $DIMTFILL = 1 << 1,
            $DIMTFILLCLR = 1 << 2,
            $DIMTAD = 1 << 3,
            $DIMJUST = 1 << 4,
            $DIMCLRT = 1 << 5,
            $DIMTIX = 1 << 6,
            $DIMTIH = 1 << 7,
            $DIMTOH = 1 << 8,
            $DIMTVP = 1 << 9,
            $DIMTXSTY = 1 << 10,
            $DIMTXT = 1 << 11,
            $DIMTXTDIRECTION = 1 << 12,
            $DIMUPT = 1 << 13,
            $DIMTMOVE = 1 << 14,
        };

        enum TextAndArrowUnsufficientSpaceArrangementPolicy {
            OUTSIDE_EXT_LINES  = 0,    // 0  Places both text and arrows outside extension lines
            ARROW_FIRST_THEN_TEXT  = 1,// 1 Moves arrows first, then text
            TEXT_FIRST_THEN_ARROW = 2, // 2 Moves text first, then arrows
            EITHER_TEXT_OR_ARROW = 3   // 3  Moves either text or arrows, whichever fits best
        };

        enum BackgroundColorPolicy {
            NONE  = 0,     // 0 - No background
            DRAWING = 1,  // 1 - The background color of the drawing
            EXPLICIT = 2  // 2 - The background specified by DIMTFILLCLR
        };

        enum HorizontalPositionPolicy {
            ABOVE_AND_CENTERED  = 0,  // 0 Positions the text above the dimension line and center-justifies it between the extension lines
            NEXT_TO_EXT_ONE = 1,     // 1 Positions the text next to the first extension line
            NEXT_TO_EXT_TWO = 2,     // 2 Positions the text next to the second extension line
            ABOVE_ALIGN_EXT_ONE = 3, // 3 Positions the text above and aligned with the first extension line
            ABOVE_ALIGN_EXT_TWO = 4 // 4 Positions the text above and aligned with the second extension line
        };

        enum VerticalPositionPolicy {
          CENTER_BETWEEN_EXT_LINES  = 0,             // 0 Centers the dimension text between the extension lines.
          ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL = 1, // 1 Places the dimension text above the dimension line except when the dimension line is not
                                                // horizontal and text inside the extension lines is forced horizontal(DIMTIH = 1)
                                                // The distance from the dimension line to the baseline of the lowest line of text is the current DIMGAP value.
          FAREST_SIDE_FROM_DEF_POINTS  = 2,          // 2 Places the dimension text on the side of the dimension line farthest away from the defining points.
          JIS_POSTION = 3,                          // 3 Places the dimension text to conform to Japanese Industrial Standards(JIS).
          BELOW_DIMENSION_LINE  = 4                  // 4 Places the dimension text below the dimension line.
        };

        enum PlacementRelatedToExtLinesPolicy {
            PLACE_BETWEEN_IF_SUFFICIENT_ROOM  = 0, // 0 - For linear and angular dimensions, dimension text is placed inside the extension lines if there is sufficient room.
            PLACE_ALWAYS_INSIDE    = 1                  // 1 - Draws dimension text between the extension lines even if it would ordinarily be placed outside those lines.
        };

        enum TextOrientationPolicy {
            ALIGN_WITH_DIM_LINE = 0, // 0 - Aligns text with the dimension line
            DRAW_HORIZONTALLY  = 1// 1 - Draws text horizontally
        };

        enum TextDirection {
            LEFT_TO_RIGHT  = 0, // 0 - Displays dimension text in a Left-to-Right reading style
            RIGHT_TO_LEFT  = 1, // 1 - Displays dimension text in a Right-to-Left reading style
        };

        enum TextMovementPolicy {
            DIM_LINE_WITH_TEXT = 0,    // 0 Moves the dimension line with dimension text
            ADDS_LEADER = 1,           // 1 Adds a leader when dimension text is moved
            ALLOW_FREE_POSITIONING  = 2 // 2 Allows text to be moved freely without a leader
        };

        enum CursorControlPolicy {
            DIM_LINE_LOCATION_ONLY = 0,  // 0 Cursor controls only the dimension line location
            TEXT_AND_DIM_LINE_LOCATION  = 1 // 1 Cursor controls both the text position and the dimension line location
        };

        Text() = default;

        RS_Color explicitBackgroundFillColor() const {return DIMTFILLCLR;}
        TextAndArrowUnsufficientSpaceArrangementPolicy unsufficientSpacePolicy() const {return DIMATFIT;}
        PlacementRelatedToExtLinesPolicy extLinesRelativePlacement() const {return DIMTIX;}
        BackgroundColorPolicy backgroundFillMode() const {return DIMTFILL;}
        HorizontalPositionPolicy horizontalPositioning() const {return DIMJUST;}
        double verticalDistanceToDimLine() const {return DIMTVP;}
        VerticalPositionPolicy verticalPositioning() const {return DIMTAD;}
        TextOrientationPolicy orientationInside() const {return DIMTIH;}
        TextOrientationPolicy orientationOutside() const {return DIMTOH;}
        const QString &style() const {return DIMTXSTY;}
        RS_Color color() const {return DIMCLRT;}
        double   height() const {return DIMTXT;}
        TextDirection readingDirection() const {return DIMTXTDIRECTION;}
        TextMovementPolicy positionMovementPolicy() const {return DIMTMOVE;}
        CursorControlPolicy cursorControlPolicy() const {return DIMUPT;}

        void setExplicitBackgroundFillColor(const RS_Color& dimtfillclr);
        void setUnsufficientSpacePolicy(TextAndArrowUnsufficientSpaceArrangementPolicy dimatfit);
        void setExtLinesRelativePlacement(PlacementRelatedToExtLinesPolicy dimtix);
        void setBackgroundFillMode(BackgroundColorPolicy dimtfill);
        void setHorizontalPositioning(HorizontalPositionPolicy dimjust);
        void setVerticalPositioning(VerticalPositionPolicy dimtad);
        void setVerticalDistanceToDimLine(double dimtvp);
        void setOrientationInside(TextOrientationPolicy dimtih);
        void setOrientationOutside(TextOrientationPolicy dimtoh);
        void setStyle(const QString &dimtxsty);
        void setColor(const RS_Color& dimclrt);
        void setHeight(double dimtxt);
        void setReadingDirection(TextDirection dimtxtdirection);
        void setPositionMovementPolicy(TextMovementPolicy dimtmove);
        void setCursorControlPolicy(CursorControlPolicy dimupt);
        void fillByDefaults();
        void merge(const Text* parent);
        void setUnsufficientSpacePolicyRaw(int dimatfit);
        void setPositionMovementPolicyRaw(int dimtmove);
        void setCursorControlPolicyRaw(int dimupt);
        void setBackgroundFillModeRaw(int dimtfill);
        void setExtLinesRelativePlacementRaw(int dimtix);
        void setVerticalPositioningRaw(int dimtad);
        void setHorizontalPositioningRaw(int dimjust);
        void setOrientationInsideRaw(int dimtih);
        void setOrientationOutsideRaw(int dimtoh);
        void setReadingDirectionRaw(int dimtxtdirection);
        void copyTo(Text* text);
    private:
        /** Determines how dimension text and arrows are arranged when space is not sufficient to place both within the extension lines.
           Initial value:	3
           0  Places both text and arrows outside extension lines
           1 Moves arrows first, then text
           2 Moves text first, then arrows
           3  Moves either text or arrows, whichever fits best
           A leader is added to moved dimension text when DIMTMOVE is set to 1.
           https:  help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-E2E21B42-82AF-4D46-B9DD-F0844D20F719
        */
        TextAndArrowUnsufficientSpaceArrangementPolicy DIMATFIT{EITHER_TEXT_OR_ARROW};  /*!< code 289 V2000+ */ // default - 3

        /** Controls the background of dimension text.
            Initial value:	0
         0 - No background
         1 - The background color of the drawing
         2 - The background specified by DIMTFILLCLR
        */
        BackgroundColorPolicy DIMTFILL{NONE}; /* code 69*/

        /** Sets the color for the text background in dimensions.
           Initial value:	0
        */
        RS_Color DIMTFILLCLR{RS2::FlagByBlock}; /* code 70 */

        /** controls the vertical position of text in relation to the dimension line.
        // 0 (imperial) or 1 (metric)
         0 Centers the dimension text between the extension lines.
         1 Places the dimension text above the dimension line except when the dimension line is not
           horizontal and text inside the extension lines is forced horizontal(DIMTIH = 1)
           The distance from the dimension line to the baseline of the lowest line of text is the current DIMGAP value.
         2 Places the dimension text on the side of the dimension line farthest away from the defining points.
         3 Places the dimension text to conform to Japanese Industrial Standards(JIS).
         4 Places the dimension text below the dimension line.
         https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-863065EC-12D5-4F65-8E6F-2BE140CA68DF
        */
        VerticalPositionPolicy DIMTAD {ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL}; /*!< code 77 */

        /** Controls the horizontal positioning of dimension text.
            Initial value:	0
         0 Positions the text above the dimension line and center-justifies it between the extension lines
         1 Positions the text next to the first extension line
         2 Positions the text next to the second extension line
         3 Positions the text above and aligned with the first extension line
         4 Positions the text above and aligned with the second extension line
         https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-863065EC-12D5-4F65-8E6F-2BE140CA68DF
        */
        HorizontalPositionPolicy DIMJUST{ABOVE_AND_CENTERED}; /*!< code 280 R13+ */ // default - 0

        /** Assigns colors to dimension text. The color can be any valid color number.
           Initial value:	0
        */
        RS_Color DIMCLRT{RS2::FlagByBlock}; /*!< code 178 */

        /** Draws text between extension lines.
            Initial value:	0
            0 - For linear and angular dimensions, dimension text is placed inside the extension lines if there is sufficient room.
            1 - Draws dimension text between the extension lines even if it would ordinarily be placed outside those lines.
           For radius and diameter dimensions, DIMTIX on always forces the dimension text outside the circle or arc.
        */
        PlacementRelatedToExtLinesPolicy DIMTIX{PLACE_BETWEEN_IF_SUFFICIENT_ROOM};  /*!< code 174 */

        /** Controls the position of dimension text inside the extension lines for all dimension types except Ordinate.
            Initial value:	1 (imperial) or 0 (metric)
            0 - Aligns text with the dimension line
            1 - Draws text horizontally
        */
        TextOrientationPolicy DIMTIH{DRAW_HORIZONTALLY};    /*!< code 73 */

        /**Controls the position of dimension text outside the extension lines.
           Initial value: 1 (imperial) or 0 (metric)
          0  Aligns text with the dimension line
          1 Draws text horizontally
        */
        TextOrientationPolicy DIMTOH {DRAW_HORIZONTALLY}; /*!< code 74 */

        /** Controls the vertical position of dimension text above or below the dimension line.
           Initial value:	0.0000
           The DIMTVP value is used when DIMTAD is off. The magnitude of the vertical offset of text is the product
           of the text height and DIMTVP. Setting DIMTVP to 1.0 is equivalent to setting DIMTAD to on. The dimension
           line splits to accommodate the text only if the absolute value of DIMTVP is less than 0.7.
        */
        double DIMTVP{0.0000};            /*!< code 145 */

        // Specifies the text style of the dimension.
        QString DIMTXSTY{"Standard"};      /*!< code 340 R13+ */ // fixme - sand - use constant!

        /** Specifies the height of dimension text, unless the current text style has a fixed height.
           Initial value:	0.1800 (imperial) or 2.5000 (metric)
        */
        double DIMTXT{2.5000};            /*!< code 140 */

        /** Specifies the reading direction of the dimension text.
           Initial value:	0
           0 - Displays dimension text in a Left-to-Right reading style
           1 - Displays dimension text in a Right-to-Left reading style
        */
        TextDirection DIMTXTDIRECTION {LEFT_TO_RIGHT};  /* code 292 */

        /** Controls options for user-positioned text.
           Initial value:	0
           0 Cursor controls only the dimension line location
           1 Cursor controls both the text position and the dimension line location
        */
        CursorControlPolicy DIMUPT {DIM_LINE_LOCATION_ONLY}; /*!< code 288 R13+ */

        /** Sets dimension text movement rules.
           Initial value:	0
           0 Moves the dimension line with dimension text
           1 Adds a leader when dimension text is moved
           2 Allows text to be moved freely without a leader
        */
        TextMovementPolicy DIMTMOVE{DIM_LINE_WITH_TEXT}; /*!< code 279 V2000+ */
    };

    class DimensionLine: public ModificationAware {
    public:
        enum Fields: unsigned{
            $DIMCLRD = 1 << 0,
            $DIMDLE = 1 << 1,
            $DIMDLI = 1 << 2,
            $DIMGAP = 1 << 3,
            $DIMLTYPE = 1 << 4,
            $DIMLWD = 1 << 5,
            $DIMSD1 = 1 << 6,
            $DIMSD2 = 1 << 7,
            $DIMTOFL = 1 << 8,
            $OVER_DIMFLP1 = 1 << 9,
            $OVER_DIMFLP2 = 1 << 10
        };

        enum DimLineAndArrowSuppressionPolicy {
            DONT_SUPPRESS = 0, // 0 Dimension line is not suppressed
            SUPPRESS = 1       // 1 Dimension line is suppressed
        };

        enum DrawPolicyForOutsideText {
            DONT_DRAW_IF_ARROWHEADS_ARE_OUTSIDE = 0,  // 0 - Does not draw dimension lines between the measured points
                                                      // when arrowheads are placed outside the measured points
            DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE = 1   // 1 -  Draws dimension lines between the measured points even
                                                      // when arrowheads are placed outside  the measured points
        };


        DimensionLine() = default;

        RS_Color color() const {return DIMCLRD;}
        double lineGap() const {return DIMGAP;}
        RS2::LineWidth lineWidth() const {return DIMLWD;}
        double distanceBeyondExtLinesForObliqueStroke() const {return DIMDLE;}
        double baseLineDimLinesSpacing() const {return DIMDLI;}
        DimLineAndArrowSuppressionPolicy suppressFirstLine() const {return DIMSD1;}
        DimLineAndArrowSuppressionPolicy suppressSecondLine() const {return DIMSD2;}
        DrawPolicyForOutsideText drawPolicyForOutsideText() const {return DIMTOFL;}
        const QString& lineTypeName() const {return DIMLTYPE;}
        RS2::LineType lineType() const {return DIMLTYPE_LineType;}
        void setLineType(RS2::LineType lineType);
        void setLineGap(double dimgap);
        void setColor(RS_Color dimclrd);
        void setLineWidth(RS2::LineWidth dimlwd);
        void setDistanceBeyondExtLinesForObliqueStroke(double dimdle);
        void setBaselineDimLinesSpacing(double dimdli);
        void setSuppressFirstLine(DimLineAndArrowSuppressionPolicy dimsd1);
        void setSuppressSecondLine(DimLineAndArrowSuppressionPolicy dimsd2);
        void setDrawPolicyForOutsideText(DrawPolicyForOutsideText dimtofl);
        void setLineType(QString dimltype);
        void fillByDefaults();
        void merge(const DimensionLine* parent);
        void setLineWidthRaw(int dimlwd);
        void setDrawPolicyForOutsideTextRaw(int dimtofl);
        void setSuppressSecondLineRaw(int dimsd2);
        void setSuppressFirstLineRaw(int dimsd1);
        void copyTo(DimensionLine* dimension_line);
    private:
        /** Assigns colors to dimension lines, arrowheads, and dimension leader lines.
           Initial value:	0
           Also controls the color of leader lines created with the LEADER command. Color numbers are displayed
           in the Select Color dialog box. For BYBLOCK, enter 0. For BYLAYER, enter 256.
        */
        RS_Color DIMCLRD{RS2::FlagByBlock};              /*!< code 176 */

        /** Sets the distance the dimension line extends beyond the extension line when oblique strokes are
           drawn instead of arrowheads. Initial value:	0.0000
        */
        double DIMDLE{0.0};            /*!< code 46 */

        /** Controls the spacing of the dimension lines in baseline dimensions.
           Each dimension line is offset from the previous one by this amount, if necessary, to avoid drawing over it.
           Changes made with DIMDLI are not applied to existing dimensions.
           Initial value:	0.3800 (imperial) or 3.7500 (metric)
           https:  help.autodesk.com/view/ACD/2018/ENU/?guid=GUID-4701B99C-7895-4265-9CC3-47D3C42A463E
        */
        // fixme - add support in actions for baseline!!
        // fixme - support of DIMCONTINUEMODE (System Variable) https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-2424CA1C-B005-449B-A61A-C4F2D38C4E66
        // fixme - for baseline and continue actions
        double DIMDLI{3.7500};            /*!< code 43 */

        /** Sets the distance around the dimension text when the dimension line breaks to accommodate dimension text.
           Also sets the gap between annotation and a hook line created with the LEADER command. If you enter a
           negative value, DIMGAP places a box around the dimension text.

           Initial value:	0.0900 (imperial) or 0.6250 (metric)
           The value of DIMGAP is also used as the minimum length of each segment of the dimension line.
           To locate the components of a linear dimension within the extension lines, enough space must be available
           for both arrowheads(2 x DIMASZ), both dimension line segments(2 x DIMGAP), a gap on either side of the
           dimension text(another2 x DIMGAP), and the length of the dimension text, which depends on its size and
           number of decimal places displayed.
           https:  help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-EB367187-A5D6-404A-8121-A6D1D25B626A
        */
        double DIMGAP{0.6250};            /*!< code 147 */

        /** Sets the linetype of the dimension line.
           The value is BYLAYER, BYBLOCK, or the name of a linetype.
        */
        QString DIMLTYPE;  /* code 346*/

        RS2::LineType DIMLTYPE_LineType{RS2::LineByBlock};

        /** Assigns lineweight to dimension lines.
           Initial value:	-2
           -1 Sets the lineweight to  "BYLAYER."
           -2  Sets the lineweight to "BYBLOCK."
           -3 Sets the lineweight to "DEFAULT." "DEFAULT" is controlled by the LWDEFAULT system variable.
           Other valid values entered in hundredths of millimeters include 0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50,
           53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, and 211.
           All values must be entered in hundredths of millimeters.
           (Multiply a value by 2540 to convert values from inches to hundredths of millimeters.)
        */
        RS2::LineWidth DIMLWD = RS2::WidthByBlock; /*!< code 371 V2000+ */

        /** Controls suppression of the first dimension line and arrowhead.
           Initial value:	0
           When turned on, suppresses the display of the dimension line and arrowhead between the first extension line and the text.
           0 First dimension line is not suppressed
           1 First dimension line is suppressed
        */
        DimLineAndArrowSuppressionPolicy DIMSD1{DONT_SUPPRESS}; /*!< code 281 R13+ */

        /* Controls suppression of the second dimension line and arrowhead.
        */
        DimLineAndArrowSuppressionPolicy DIMSD2{DONT_SUPPRESS};               /*!< code 282 R13+ */

        /** Controls whether a dimension line is drawn between the extension lines even when the text is placed outside.
           Initial value:	0 (imperial) or 1 (metric)
           For radius and diameter dimensions, a dimension line is drawn inside the circle or arc when the text,
           arrowheads, and leader are placed outside.
           0 - Does not draw dimension lines between the measured points when arrowheads are placed
           outside the measured points
           1 -  Draws dimension lines between the measured points even when arrowheads are placed outside
           the measured points
        */
        DrawPolicyForOutsideText DIMTOFL{DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE};              /*!< code 172 */
    };

    class ExtensionLine: public ModificationAware {
    public:
        enum Fields: unsigned{
            $DIMCLRE = 1 << 0,
            $DIMEXE = 1 << 1,
            $DIMEXO = 1 << 2,
            $DIMFXL = 1 << 3,
            $DIMFXLON = 1 << 4,
            $DIMLTEX1 = 1 << 5,
            $DIMLTEX2 = 1 << 6,
            $DIMLWE = 1 << 7,
            $DIMSE1 = 1 << 8,
            $DIMSE2 = 1 << 9,
        };
        enum ExtensionLineAndArrowSuppressionPolicy {
            DONT_SUPPRESS = 0, // 0 line is not suppressed
            SUPPRESS = 1       // 1 line is suppressed
        };

        ExtensionLine() = default;

        RS_Color color() const {return DIMCLRE;}
        double distanceBeyondDimLine() const {return DIMEXE;}
        double distanceFromOriginPoint() const {return DIMEXO;}
        double fixedLength() const {return DIMFXL;}
        RS2::LineWidth lineWidth() const {return DIMLWE;}
        bool hasFixedLength() const {return DIMFXLON;}
        ExtensionLineAndArrowSuppressionPolicy suppressFirstLine() const{return DIMSE1;}
        ExtensionLineAndArrowSuppressionPolicy suppressSecondLine() const{return DIMSE2;}

        QString lineTypeFirstRaw() const {return DIMLTEX1;}
        QString lineTypeSecondRaw() const {return DIMLTEX2;}
        RS2::LineType lineTypeFirst() const {return DIMLTEX1_linetype;}
        RS2::LineType lineTypeSecond() const {return DIMLTEX2_linetype;}

        void setColor(RS_Color dimclre);
        void setDistanceBeyondDimLine(double dimexe);
        void setDistanceFromOriginPoint(double dimexo);
        void setFixedLength(double dimfxl);
        void setHasFixedLength(bool dimfxlon) ;
        void setLineWidth(RS2::LineWidth dimlwe);
        void setLineWidthRaw(int dimlwe);
        void setLineTypeFirst(const QString& dimltex1);
        void setLineTypeSecond(const QString& dimltex2);
        void setLineTypeFirst(RS2::LineType lineType);
        void setLineTypeSecond(RS2::LineType lineType);
        void setSuppressFirst(ExtensionLineAndArrowSuppressionPolicy dimse1);
        void setSuppressSecond(ExtensionLineAndArrowSuppressionPolicy dimses);
        void setSuppressFirstRaw(int dimse1);
        void setSuppressSecondRaw(int dimse2);
        void fillByDefaults();
        void merge(const ExtensionLine* parent);
        void copyTo(ExtensionLine* extension_line);
    private:
        static ExtensionLineAndArrowSuppressionPolicy int2SuppressionPolicy(int value);

        /**Assigns colors to extension lines, center marks, and centerlines.
           Color numbers are displayed in the Select Color dialog box. For BYBLOCK, enter 0. For BYLAYER, enter 256.
        */
        RS_Color DIMCLRE{RS2::FlagByBlock};              /*!< code 177 */

        /** Specifies how far to extend the extension line beyond the dimension line.
          Initial value:	0.1800 (imperial) or 1.2500 (metric)
        */
        double DIMEXE{1.2500};            /*!< code 44 */

        /** Specifies how far extension lines are offset from origin points.
           With fixed-length extension lines, this value determines the minimum offset.
           Initial value:	0.0625 (imperial) or 0.6250 (metric)
        */
        double DIMEXO{0.625};            /*!< code 42 */

        /** Sets the total length of the extension lines starting from the dimension line toward the dimension origin.
           Initial value:	1.0000
        */
        double DIMFXL{1.0};            /*!< code 49 V2007+ */

        /** Controls whether extension lines are set to a fixed length.
           Initial value:	0
           When DIMFXLON is on (1), extension lines are set to the length specified by DIMFXL.
        */
        bool DIMFXLON{false};             /*!< code 290 V2007+ */


        /** Sets the linetype of the first extension line.
          Initial value:	""
           The value is BYLAYER, BYBLOCK, or the name of a linetype.
        */
        QString DIMLTEX1{""};  /* code 347*/

        /** Sets the linetype of the second extension line.
         * Initial value:	""
           The value is BYLAYER, BYBLOCK, or the name of a linetype.
        */
        QString DIMLTEX2{""}; /* code 348 */

        RS2::LineType DIMLTEX1_linetype = RS2::LineType::LineByBlock;
        RS2::LineType DIMLTEX2_linetype = RS2::LineType::LineByBlock;

        /** Assigns lineweight to extension lines.
           Initial value:	-2
           -1 Sets the lineweight to "BYLAYER."
           -2 Sets the lineweight to "BYBLOCK."
           -3 Sets the lineweight to "DEFAULT." "DEFAULT" is controlled by the LWDEFAULT system variable.
           Other valid values entered in hundredths of millimeters include 0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40,
           50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, and 211.
           All values must be entered in hundredths of millimeters.
           (Multiply a value by 2540 to convert values from inches to hundredths of millimeters.)
        */
        RS2::LineWidth DIMLWE =  RS2::WidthByBlock;               /*!< code 372 V2000+ */

        /** Suppresses display of the first extension line.
           OFF Extension line is not suppressed
           ON  Extension line is suppressed
           Initial value:	OFF
        */
        ExtensionLineAndArrowSuppressionPolicy DIMSE1{DONT_SUPPRESS};              /*!< code 75 */

        /** Suppresses display of the second extension line.
           OFF Extension line is not suppressed
           ON  Extension line is suppressed
           Initial value:	OFF
        */
        ExtensionLineAndArrowSuppressionPolicy DIMSE2{DONT_SUPPRESS};                /*!< code 76 */
    };

    class Arrowhead : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMASZ = 1 << 0,
            $DIMBLK = 1 << 1,
            $DIMBLK1 = 1 << 2,
            $DIMBLK2 = 1 << 3,
            $DIMSAH = 1 << 4,
            $DIMSOXD = 1 << 5,
            $DIMTSZ = 1 << 6
        };
        enum ArrowHeadSuppressionPolicy {
            DONT_SUPPRESS, // 0 line is not suppressed
            SUPPRESS       // 1 line is suppressed
        };

        Arrowhead() = default;

        ArrowHeadSuppressionPolicy suppression() const {return DIMSOXD;}
        void copyTo(Arrowhead* arrowhead);
        QString obtainFirstArrowName();
        QString obtainSecondArrowName();
        double size() const {return DIMASZ;};
        const QString &sameBlockName() const {return DIMBLK;}
        const QString &arrowHeadBlockNameFirst() const {return DIMBLK1;};
        const QString &arrowHeadBlockNameSecond() const {return DIMBLK2;};
        bool isUseSeparateArrowHeads() const {return DIMSAH;}
        double tickSize() const {return DIMTSZ;}

        void setSuppressions(ArrowHeadSuppressionPolicy dimsoxd);
        void setSuppressionsRaw(int dimsoxd);
        void setSize(double dimasz);
        void setSameBlockName(const QString &dimblk);
        void setArrowHeadBlockNameFirst(const QString &dimblk1);
        void setArrowHeadBlockNameSecond(const QString &dimblk2);
        void setUseSeparateArrowHeads(bool dimsah);
        void setTickSize(double dimtsz);
        void fillByDefaults();
        void merge(const Arrowhead* parent);
    private:
        /** Controls the size of dimension line and leader line arrowheads. Also controls the size of hook lines.
           Multiples of the arrowhead size determine whether dimension lines and text should fit between the
           extension lines. DIMASZ is also used to scale arrowhead blocks if set by DIMBLK. DIMASZ has no
           effect when DIMTSZ is other than zero.
           Initial value:	0.1800 (imperial) or 2.5000 (metric)
        */
        double DIMASZ{2.5};            /*!< code 41 */

        /** Sets the arrowhead block displayed at the ends of dimension lines.
         * Initial value:	""
         */
        QString DIMBLK{""};        /*!< code 5, code 342 V2000+ */

        /** Sets the arrowhead for the first end of the dimension line when DIMSAH is on.
           Initial value:	""
        */
        QString DIMBLK1{""};       /*!< code 6, code 343 V2000+ */

        /** Sets the arrowhead for the second end of the dimension line when DIMSAH is on.
           Initial value:	""
        */
        QString DIMBLK2{""};       /*!< code 7, code 344 V2000+ */

        /** Controls the display of dimension line arrowhead blocks.
           Initial value:	OFF
           OFF Use arrowhead blocks set by DIMBLK
           ON  Use arrowhead blocks set by DIMBLK1 and DIMBLK2
        */
        bool DIMSAH{false}; /*!< code 173 */

        /** Suppresses arrowheads if not enough space is available inside the extension lines.
           Initial value:	OFF
           If not enough space is available inside the extension lines and DIMTIX is on, setting DIMSOXD to On
           suppresses the arrowheads. If DIMTIX is off, DIMSOXD has no effect.
           OFF Arrowheads are not suppressed
           ON Arrowheads are suppressed
        */
        ArrowHeadSuppressionPolicy DIMSOXD{DONT_SUPPRESS};              /*!< code 175 */

        /** Specifies the size of oblique strokes drawn instead of arrowheads for linear, radius, and
           diameter dimensioning.
           Initial value:	0.0000
           0 Draws arrowheads.
           >0 Draws oblique strokes instead of arrowheads. The size of the oblique strokes is determined
           by this value multiplied by the DIMSCALE value.
        */
        double DIMTSZ{0};            /*!< code 142 */
    };

    class ZerosSuppression : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMZIN = 1 << 0,
            $DIMAZIN = 1 << 1,
            $DIMALTTZ = 1 << 2,
            $DIMALTZ = 1 << 3,
            $DIMTZIN = 1 << 4,
        };
        enum LinearSuppressionPolicy {
            SUPPRESS_ZERO_FEET_AND_ZERO_INCHES = 0,           // 0 Suppresses zero feet and precisely zero inches
            INCLUDE_ZERO_FEET_AND_ZERO_INCHES = 1,            // 1 Includes zero feet and precisely zero inches
            INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES = 2,   // 2 Includes zero feet and suppresses zero inches
            INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET = 3,   // 3 Includes zero inches and suppresses zero feet
            LEADING_IN_DECIMAL = 4,                           // 4 Suppresses leading zeros in decimal dimensions (for example, 0.5000 becomes .5000)
            TRAILING_IN_DECIMAL = 8,                          // 8 Suppresses trailing zeros in decimal dimensions (for example, 12.5000 becomes 12.5)
            BOTH_LEADING_AND_TRAILING = 12                     // 12 Suppresses both leading and trailing zeros (for example, 0.5000 becomes .5)
        };

        enum AngularSuppressionPolicy{
            DONT_SUPPRESS = 0,              // 0 Displays all leading and trailing zeros
            SUPPRESS_LEADING_DECIMAL = 1,   // 1 Suppresses leading zeros in decimal dimensions(for example, 0.5000 becomes.5000)
            SUPPRESS_TRALINING_DECIMAL = 2, // 2 Suppresses trailing zeros in decimal dimensions(for example, 12.5000 becomes12.5)
            SUPPRESS_ALL_DECIMAL = 3        // 3 Suppresses leading and trailing zeros(for example, 0.5000 becomes.5)
        };

        enum ToleranceSuppressionPolicy {
           TOL_SUPPRESS_ZERO_FEET_AND_ZERO_INCHES = 0,  // 0 Suppresses zero feet and precisely zero inches
           TOL_INCLUDE_ZERO_FEET_AND_ZERO_INCHES = 1,   // 1 Includes zero feet and precisely zero inches
           TOL_INCLUDE_ZERO_FEET_AND_SUPPRESS_ZERO_INCHES = 2, // 2 Includes zero feet and suppresses zero inches
           TOL_INCLUDE_ZERO_INCHES_AND_SUPPRESS_ZERO_FEET = 3, // 3 Includes zero inches and suppresses zero feet
            // To suppress leading or trailing zeros, add the following values to one of the preceding values:
            SUPPRESS_LEADING_ZEROS = 4, // 4 Suppresses leading zeros
            SUPPRESS_TRAILING_ZEROS = 8, // 8  Suppresses trailing zeros
        };

        ZerosSuppression() = default;

        int angularRaw() const {return DIMAZIN;}
        bool isAngularSuppress(int flag) {return DIMAZIN &flag;}
        int linearRaw() const {return DIMZIN;}
        bool isLinearSuppress(int flag) {return DIMZIN &flag;}
        int toleranceRaw() const {return DIMTZIN;}
        bool isToleranceSuppress(int flag){return DIMTZIN &flag;}
        int altLinearRaw() const {return DIMALTZ;}
        bool isAltLinearSuppress(int flag) {return DIMALTZ &flag;}
        int altToleranceRaw() const  {return DIMALTTZ;}
        bool isAltToleranceSuppress(int flag){return DIMALTTZ &flag;}

        void clearLinear(){DIMZIN = 0;}
        void setLinearRaw(int dimzin);
        void setLinearFlag(LinearSuppressionPolicy dimzin, bool set);

        void clearAngular(){DIMAZIN = 0;}
        void setAngularRaw(int dimazin);
        void setAngularFlag(AngularSuppressionPolicy dimzin, bool set);

        void clearTolerance(){DIMTZIN = 0;}
        void setToleranceRaw(int dimtzin);
        void setToleranceFlag(ToleranceSuppressionPolicy dimtzin, bool set);

        void clearAltLinear(){DIMALTZ = 0;}
        void setAltLinearRaw(int dimaltz);
        void setAltLinearFlag(LinearSuppressionPolicy dimaltz, bool set);

        void clearAltTolerance(){DIMALTTZ = 0;}
        void setAltToleranceRaw(int dimalttz);
        void setAltToleranceFlag(ToleranceSuppressionPolicy dimalttz, bool set);
        void fillByDefaults();
        void merge(const ZerosSuppression* parent);
        void copyTo(ZerosSuppression* zeros_suppression);
    private:
        /** controls the suppression of zeros in the primary unit value.
           Initial value:	0 (imperial) or 8 (metric)
           Values 0-3 affect feet-and-inch dimensions only:
           0 Suppresses zero feet and precisely zero inches
           1 Includes zero feet and precisely zero inches
           2 Includes zero feet and suppresses zero inches
           3 Includes zero inches and suppresses zero feet
           4 Suppresses leading zeros in decimal dimensions (for example, 0.5000 becomes .5000)
           8 Suppresses trailing zeros in decimal dimensions (for example, 12.5000 becomes 12.5)
           12 Suppresses both leading and trailing zeros (for example, 0.5000 becomes .5)
        */
        int DIMZIN{TRAILING_IN_DECIMAL};               /*!< code 78 */

        /** Suppresses zeros for angular dimensions.
           Initial value:	0
           0 Displays all leading and trailing zeros
           1 Suppresses leading zeros in decimal dimensions(for example, 0.5000 becomes.5000)
           2 Suppresses trailing zeros in decimal dimensions(for example, 12.5000 becomes12.5)
           3 Suppresses leading and trailing zeros(for example, 0.5000 becomes.5)
        */
        int DIMAZIN{DONT_SUPPRESS}; /*!< code 79 V2000+ */

        /** Controls suppression of zeros in tolerance values.
           Initial value:	0
           0 Suppresses zero feet and precisely zero inches
           1 Includes zero feet and precisely zero inches
           2 Includes zero feet and suppresses zero inches
           3 Includes zero inches and suppresses zero feet
           To suppress leading or trailing zeros, add the following values to one of the preceding values:
           4 Suppresses leading zeros
           8  Suppresses trailing zeros
        */
        int DIMALTTZ {TOL_SUPPRESS_ZERO_FEET_AND_ZERO_INCHES}; /*!< code 286 R13+ */

        /** Controls the suppression of zeros for alternate unit dimension values.
           Initial value:	0
           DIMALTZ values 0-3 affect feet-and-inch dimensions only.
           0  Suppresses zero feet and precisely zero inches
           1 Includes zero feet and precisely zero inches
           2 Includes zero feet and suppresses zero inches
           3 Includes zero inches and suppresses zero feet
           4 Suppresses leading zeros in decimal dimensions (for example, 0.5000 becomes .5000)
           8 Suppresses trailing zeros in decimal dimensions (for example, 12.5000 becomes 12.5)
           12 Suppresses both leading and trailing zeros (for example, 0.5000 becomes .5)
        */
        int DIMALTZ{LinearSuppressionPolicy::SUPPRESS_ZERO_FEET_AND_ZERO_INCHES};              /*!< code 285 R13+ */

        /** Controls the suppression of zeros in tolerance values.
           Initial value:	0 (imperial) or 8 (metric)
           Values 0-3 affect feet-and-inch dimensions only.
           0 Suppresses zero feet and precisely zero inches
           1 Includes zero feet and precisely zero inches
           2 Includes zero feet and suppresses zero inches
           3 Includes zero inches and suppresses zero feet
           4 Suppresses leading zeros in decimal dimensions(for example, 0.5000 becomes.5000)
           8 Suppresses trailing zeros in decimal dimensions(for example, 12.5000 becomes12.5)
           12 Suppresses both leading and trailing zeros(for example, 0.5000 becomes.5)
        */
        int DIMTZIN{TRAILING_IN_DECIMAL}; /*!< code 284 R13+ */
    };

    class Scaling : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMLFAC = 1 << 0,
            $DIMSCALE = 1 << 1,
        };

        Scaling() = default;

        double linearFactor() const {return DIMLFAC;}
        double scale() const {return DIMSCALE;}
        void copyTo(Scaling* scaling);
        void setLinearFactor(double dimlfac);
        void setScale(double dimscale);
        void fillByDefaults();
        void merge(const Scaling* parent);
    private:
        /** Sets a scale factor for linear dimension measurements.
           Initial value:	1.0000
           All linear dimension distances, including radii, diameters, and coordinates, are multiplied by DIMLFAC
           before being converted to dimension text. Positive values of DIMLFAC are applied to dimensions in both
           model space and paper space; negative values are applied to paper space only.
           DIMLFAC applies primarily to nonassociative dimensions(DIMASSOC set0 or 1).
           For nonassociative dimensions in paper space, DIMLFAC must be set individually for
           each layout viewport to accommodate viewport scaling.
           DIMLFAC has no effect on angular dimensions, and is not applied to the values held in DIMRND,
           DIMTM, or DIMTP.
        */
        double DIMLFAC {1.0};           /*!< code 144 */

        /** Sets the overall scale factor applied to dimensioning variables that specify sizes, distances, or offsets.
           Initial value:	1.0000
           Also affects the leader objects with the LEADER command.
           Use MLEADERSCALE to scale multileader objects created with the MLEADER command.
           0.0 A reasonable default value is computed based on the scaling between the current model space viewport
           and paper space . If you are in paper space or model space and not using the paper space feature, the
           scale factor is  1.0.
           > 0 A scale factor is computed that leads text sizes, arrowhead sizes, and other scaled distances to plot
           at their face values.
           DIMSCALE does not affect measured lengths, coordinates, or angles.
           Use DIMSCALE to control the overall scale of dimensions. However, if the current dimension style is
           annotative,  DIMSCALE is automatically set to zero and the dimension scale is controlled by the
           CANNOSCALE system variable. DIMSCALE cannot be set to a non - zero value when using annotative dimensions.
        */
        double DIMSCALE{1.0}; /*!< code 40 */
    };

    class LinearRoundOff : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMALTRND = 1 << 0,
            $DIMRND = 1 << 1,
        };

        LinearRoundOff() = default;

        double altRoundTo() const {return DIMALTRND;}
        void setAltRoundToValue(double dimaltrnd);
        double roundTo() const {return DIMRND;}
        void setRoundToValue(double dimrnd);
        void fillByDefaults();
        void merge(const LinearRoundOff* parent);
        void copyTo(LinearRoundOff* round_off);
    private:
        /** Rounds off the alternate dimension units.
            Initial value:	0.0000
        */
        double DIMALTRND{0};         /*!< code 148 V2000+ */

        /** Rounds all dimensioning distances to the specified value.
           Initial value:	0.0000
           For instance, if DIMRND is set to 0.25, all distances round to the nearest 0.25 unit.
           If you set DIMRND to 1.0, all distances round to the nearest integer. Note that the number of
           digits edited after the decimal point depends on the precision set by DIMDEC.
           DIMRND does not apply to angular dimensions.
        */
        double DIMRND {0};            /*!< code 45 */
    };

    class Fractions : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMFRAC = 1 << 0,
        };
        enum FractionStylePolicy {
            HORIZONTAL = 0,
            DIAGONAL_STACKING = 1,
            NOT_STACKED = 2
        };

        Fractions() = default;

        FractionStylePolicy style() const {return DIMFRAC;}
        void setStyle(FractionStylePolicy dimfrac);
        void setStyleRaw(int dimfrac);
        void fillByDefaults();
        void merge(const Fractions* parent);
        void copyTo(Fractions* fractions);
    private:
        /** Sets the fraction format when DIMLUNIT is set to 4 (Architectural) or 5 (Fractional).
           Initial value:	0
           0 Horizontal stacking
           1 Diagonal stacking
           2 Not stacked (for example, 1/2)
           https:  help.autodesk.com/view/ACD/2018/ENU/?guid=GUID-AA5F50E5-1376-4E5A-AEFB-55C9FB95E883
        */
        FractionStylePolicy DIMFRAC{HORIZONTAL}; /*!< code 276 V2000+ */
    };

    class LinearFormat : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMALT = 1 << 0,
            $DIMALTD = 1 << 1,
            $DIMALTF = 1 << 2,
            $DIMALTU = 1 << 3,
            $DIMAPOST = 1 << 4,
            $DIMDEC = 1 << 5,
            $DIMSEP = 1 << 6,
            $DIMLUNIT = 1 << 7,
            $DIMPOST = 1 << 8,
        };
        enum AlternateUnitsPolicy {
            DISABLE,
            ENABLE
        };

        LinearFormat() = default;
        ~LinearFormat() override;

        class TextPattern {
        public:
            TextPattern(bool primary, const QString& text, LinearFormat* f);
            QString update();
            void parse(const QString& val);
            QString getPrefix();
            QString getSuffix();
            bool isSuffixEndsWithLineFeed();
            void setPrefix(const QString& prefix);
            void setSuffix(const QString& suffix);
            void setSuffixEndsWithNewLineFeed(bool set);
        private:
            QString prefix;
            QString suffix;
            QString separator;
            QString completeString;
            bool suffixEndsWithLineFeed{false};
            LinearFormat* format;
            bool forAltUnit{false};
        };

        AlternateUnitsPolicy alternateUnits() const {return DIMALT;}
        int altDecimalPlaces() const {return DIMALTD;}
        double altUnitsMultiplier() const {return DIMALTF;}
        const QString &altPrefixOrSuffix() const {return DIMAPOST;}
        RS2::LinearFormat altFormat() const {return DIMALTU;}
        int altFormatRaw();

        TextPattern* getPrimaryPrefixOrSuffix();
        TextPattern* getAlternativePrefixOrSuffix();

        RS2::LinearFormat  format() const {return DIMLUNIT;}
        int formatRaw();
        int decimalPlaces() const {return DIMDEC;}
        const QString &prefixOrSuffix() const {return DIMPOST;}
        int decimalFormatSeparatorChar() const {return DIMDSEP;}
        void setAltUnitsMultiplier(double dimaltf);
        void setAlternateUnits(AlternateUnitsPolicy dimalt);
        void setAltPrefixOrSuffix(const QString &dimapost);
        void setDecimalPlaces(int dimdec);
        void setAltFormat( RS2::LinearFormat dimaltu);
        void setAltDecimalPlaces(int dimaltd);
        void setDecimalFormatSeparatorChar(int dimsep);
        void setPrefixOrSuffix(const QString &dimpost);

        void setFormat(RS2::LinearFormat  dimlunit);
        void fillByDefaults();
        void merge(const LinearFormat* parent);
        void setAltFormatRaw(int dimaltu);
        void copyTo(LinearFormat* linear_format);

        static RS2::LinearFormat dxfInt2LinearFormat(int f);
        static int linearFormat2dxf(RS2::LinearFormat f);
        void setAlternateUnitsRaw(int dimalt);
        void setFormatRaw(int dimlunit);
    private:
        /** Controls the display of alternate units in dimensions.
         Initial value:	OFF
         OFF  Disables alternate units
         ON  Enables alternate units
        */
        AlternateUnitsPolicy DIMALT{DISABLE};               /*!< code 170 */

        /* Controls the number of decimal places in alternate units.
           Initial value:	2 (imperial) or 3 (metric)
           If DIMALT is turned on, DIMALTD sets the number of digits displayed to the right of
           the decimal point in the alternate measurement.
        */
        int DIMALTD {3};              /*!< code 171 */

        /** Controls the multiplier for alternate units.
           Initial value:	25.4000 (imperial) or 0.0394 (metric)
           If DIMALT is turned on, DIMALTF multiplies linear dimensions by a factor to produce a value
           in an alternate system of measurement. The initial value represents the number of millimeters in an inch.
         */
        double DIMALTF{0.0394};           /*!< code 143 */

        /** Sets the units format for alternate units of all dimension substyles except Angular.
           Initial value:	2
           1 Scientific
           2 Decimal
           3 Engineering
           4 Architectural(always displayed stacked)
           5 Fractional(always displayed stacked)
           6  Microsoft Windows Desktop(decimal formatusing Control Panel settingsfor decimal separator
           and number grouping symbols)
         */
        RS2::LinearFormat DIMALTU = RS2::Decimal;/*RS_Graphic::convertLinearFormatDXF2LC(2)*/;              /*!< code 273 R13+ */

        /** Specifies a text prefix or suffix (or both) to the alternate dimension measurement for
           all types of dimensions except angular.
           For instance, if the current units are Architectural, DIMALT is on, DIMALTF is 25.4
           (the number of millimeters per inch), DIMALTD is 2, and DIMAPOST is set to "mm", a
           distance of 10 units would be displayed as 10"[254.00mm].
           To turn off an established prefix or suffix (or both), set it to a single period (.).
         */
         QString DIMAPOST{""};      /*!< code 4 */


        /** Sets the number of decimal places displayed for the primary units of a dimension.
           Initial value:	4 (imperial) or 2 (metric)
           The precision is based on the units or angle format you have selected. Specified value is applied to
           angular dimensions when DIMADEC is set to -1.
        */
        int DIMDEC {2};               /*!< code 271 R13+ */

        /** Specifies a single-character decimal separator to use when creating dimensions whose unit format is decimal.
           Initial value:	. (imperial) or , (metric)
           When prompted, enter a single character at the Command prompt. If dimension units is set to Decimal,
           the DIMDSEP character is used instead of the default decimal point. If DIMDSEP is set to NULL
           (default value, reset by entering a period), the decimal point is used as the dimension separator.
         */
        int DIMDSEP {','};              /*!< code 278 V2000+ */

        /** Sets units for all dimension types except Angular.
           Initial value:	2
           1 Scientific
           2 Decimal
           3 Engineering
           4 Architectural(always displayed stacked)
           5 Fractional(always displayed stacked)
           6  Microsoft Windows Desktop(decimal formatusing Control Panel settingsfor decimal separator
           and number grouping symbols)
        */
        RS2::LinearFormat DIMLUNIT = RS2::Decimal;/*RS_Graphic::convertLinearFormatDXF2LC(2);*/ /*!< code 277 V2000+ */

        //V12
        /** Specifies a text prefix or suffix (or both) to the dimension measurement.
           Initial value:	None
           For example, to establish a suffix for millimeters, set DIMPOST to mm; a distance of 19.2 units would be
           displayed as 19.2 mm.
           If tolerances are turned on, the suffix is applied to the tolerances as well as to the main dimension.
           Use <> to indicate placement of the text in relation to the dimension value. For example, enter <> mm
           to display a 5.0 millimeter
           radial dimension as "5.0mm". If you entered mm <>, the dimension would be displayed as "mm 5.0".
           Use the <> mechanism for angular dimensions.
           */
        QString DIMPOST{""};       /*!< code 3 */

        TextPattern* primaryPrefixSuffix {nullptr};
        TextPattern* alternativePrefixSuffix {nullptr};
    };

    class AngularFormat: public ModificationAware {
    public:
        enum Fields: unsigned{
            $DIMADEC = 1 << 0,
            $DIMAUNIT = 1 << 1,
        };

        ~AngularFormat() override = default;
        void copyTo(AngularFormat* angular_format);

        AngularFormat() = default;

        int decimalPlaces() const {return DIMADEC;}
        RS2::AngleFormat format() const {return DIMAUNIT;}

        void setDecimalPlaces(int dimadec);
        void setFormat( RS2::AngleFormat dimaunit);
        void fillByDefaults();
        void merge(const AngularFormat* parent);
        void setFormatRaw(int dimaunit);
    private:
        /**Controls the number of precision places displayed in angular dimensions.
         Initial value:	0
         -1  Angular dimensions display the number of decimal places specified by DIMDEC
         0-8 Specifies the number of decimal places displayed in angular dimensions(independent of DIMDEC)
         */
        int DIMADEC{0}; /*!< code 179 V2000+ */

        /** Sets the units format for angular dimensions.
          Initial value:	0
         0 Decimal degrees
         1 Degrees/minutes/seconds
         2 Gradians
         3 Radians
         4 - LC Surveyors?? // fixme - sand - check this!!
         */
        RS2::AngleFormat DIMAUNIT =  RS2::DegreesDecimal; /* RS_Units::numberToAngleFormat(0)*/;             /*!< code 275 R13+ */
    };

    class LatteralTolerance: public ModificationAware {
    public:
        enum Fields: unsigned{
            $DIMALTTD = 1 << 0,
            $DIMLIM = 1 << 1,
            $DIMTDEC = 1 << 2,
            $DIMTFAC = 1 << 3,
            $DIMTM = 1 << 4,
            $DIMTOL = 1 << 5,
            $DIMTOLJ = 1 << 6,
            $DIMTP = 1 << 7,
            $DIMTALN = 1 << 8
        };
        enum VerticalJustificationToDimText{
            BOTTOM = 0,  // 0 Bottom
            MIDDLE = 1,  // 1  Middle
            TOP  = 2    // 2  Top
        };

        enum AdjustmentMode {
            ALIGN_DECIMAL_SEPARATORS = 0,
            ALIGN_OPERATIONAL_SYMBOLS = 1
        };

        LatteralTolerance() = default;

        int decimalPlacesAltDim() const {return DIMALTTD;}
        bool isLimitsGeneratedAsDefaultText() const {return DIMLIM;}
        int decimalPlaces() const {return DIMTDEC;}
        double heightScaleFactorToDimText() const {return DIMTFAC;}
        double lowerToleranceLimit() const {return DIMTM;}
        bool isAppendTolerancesToDimText() const {return DIMTOL;}
        VerticalJustificationToDimText verticalJustification() const {return DIMTOLJ;}
        AdjustmentMode adjustment() const {return DIMTALN;}
        double upperToleranceLimit() const {return DIMTP;}

        void setDecimalPlacesAltDim(int dimalttd);
        void setLimitsAreGeneratedAsDefaultText(bool dimlim);
        void setDecimalPlaces(int dimtdec);
        void setHeightScaleFactorToDimText(double dimtfac);
        void setLowerToleranceLimit(double dimtm);
        void setAppendTolerancesToDimText(bool dimtol);
        void setUpperToleranceLimit(double dimtp);
        void setVerticalJustification(VerticalJustificationToDimText dimtolj);
        void setVerticalJustificationRaw(int dimtolj);
        void setAdjustment(AdjustmentMode dimtaln);
        void setAdjustmentRaw(int dimtaln);
        void fillByDefaults();
        void merge(const LatteralTolerance* parent);
        void copyTo(LatteralTolerance* latteral_tolerance);
    private:
        /**
         Sets the number of decimal places for the tolerance values in the alternate units of a dimension.
         Initial value:	2 (imperial) or 3 (metric)
        */
        int DIMALTTD {3};             /*!< code 274 R13+ */

        /** Generates dimension limits as the default text.
            Initial value:	OFF
            OFF Dimension limits are not generated as default text
            ON Dimension limits are generated as default text
        */
        bool DIMLIM {false};               /*!< code 72 */

        /** Sets the number of decimal places to display in tolerance values for the primary units in a dimension.
            Initial value:	4 (imperial) or 2 (metric)
            This system variable has no effect unless DIMTOL is set to On. The default for DIMTOL is Off.
        */
        int DIMTDEC {2};              /*!< code 272 R13+ */

        /** Specifies a scale factor for the text height of fractions and tolerance values relative to the dimension
            text height, as set by DIMTXT.
            Initial value:	1.0000
            For example, if DIMTFAC is set to 1.0, the text height of fractions and tolerances is the same height as
            the dimension text. If DIMTFAC is set to 0.7500, the text height of fractions and tolerances is
            three-quarters the size of dimension text.
        */
        double DIMTFAC{1.0};           /*!< code 146 */

        /** Sets the minimum (or lower) tolerance limit for dimension text when DIMTOL or DIMLIM is on.
            Initial value:	0.0000
            DIMTM accepts signed values. If DIMTOL is on and DIMTP and DIMTM are set to the same value,
            a tolerance value is drawn.
            If DIMTM and DIMTP values differ, the upper tolerance is drawn above the lower, and a plus
            sign is added to the DIMTP value if it is positive.
            For DIMTM, the program uses the negative of the value you enter (adding a minus sign if you
            specify a positive number and a plus sign if you specify a negative number).
        */
        double DIMTM{0.0};             /*!< code 48 */

        /** Appends tolerances to dimension text.
          Initial value:	Off
          Setting DIMTOL to on turns DIMLIM off.
        */
        bool DIMTOL {false};               /*!< code 71 */

        /** Sets the vertical justification for tolerance values relative to the nominal dimension text.
          Initial value:	1 (imperial) or 0 (metric)
          This system variable has no effect unless DIMTOL is set to On. The default for DIMTOL is Off.
            0 Bottom
            1  Middle
            2  Top
         */
        VerticalJustificationToDimText DIMTOLJ{BOTTOM};  /*!< code 283 R13+ */

        /** Sets the maximum (or upper) tolerance limit for dimension text when DIMTOL or DIMLIM is on.
            Initial value:	0.0000
            DIMTP accepts signed values. If DIMTOL is on and DIMTP and DIMTM are set to the same value,
            a tolerance value is drawn.
            If DIMTM and DIMTP values differ, the upper tolerance is drawn above the lower and a
            plus sign is added to the DIMTP value if it is positive.
        */
        double DIMTP {0.0};             /*!< code 47 */

        /** sets align for tolerance's measurements scale
         * 0 - align decimal separators
         * 1 - align operational symbols
         */
        AdjustmentMode DIMTALN {ALIGN_DECIMAL_SEPARATORS}; /* dimstyle extended data, code 392 in group ACAD_DSTYLE_DIMTALN */
    };

    class Leader : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMLDRBLK = 1 << 0, // fixme - sand - LOAD/SAVE
        };

        Leader() = default;

        const QString &arrowBlockName() const {return DIMLDRBLK;}
        void setArrowBlockName(const QString &dimldrblk);
        void fillByDefaults();
        void merge(const Leader* parent);
        void copyTo(Leader* leader);
    private:
        /** Specifies the arrow type for leaders.
            Initial value:	""
            To return to the default, closed-filled arrowhead display, enter a single period (.). For a list of arrowhead entries, see DIMBLK.
        */
        QString DIMLDRBLK{""};     /*!< code 341 V2000+ */
    };

    // fixme - sand - move to MLeaderStyle out of DimStyle
    class MLeader : public ModificationAware{
    public:
        enum Fields: unsigned{
            $MLEADERSCALE = 1 << 0,
        };

        MLeader() = default;

        double scale() const {return MLEADERSCALE;}
        void setScale(double mleaderscale);
        void fillByDefaults();
        void merge(const MLeader* parent);
        void copyTo(MLeader* leader);
    private:
        /** Sets the overall scale factor applied to multileader objects.
         Initial value:	1.0000
         Use DIMSCALE to scale leader objects created with the LEADER command.
         0.0 - A reasonable default value is computed based on the scaling between the current model space viewport
              and paper space. If you are in paper space or model space and not using the paper space feature, the
              scale factor is 1.0.
        > 0 - A scale factor is computed that leads text sizes, arrowhead sizes, and other scaled distances
              to plot at their face values.

         MLEADERSCALE does not affect measured lengths, coordinates, or angles.
        When MLEADERSCALE is set to 0, and the current multileader style is not annotative, the overall multileader
        scale of multileader objects created in paper space viewports is determined by the viewport scale. When the
        current multileader style is annotative, the MLEADERSCALE value is set to 0. Changes to the MLEADERSCALE
         value are ignored and the value is reset to 0.
        */
        double MLEADERSCALE{1.0}; // fixme code - should it actually be there?
    };

    class Radial : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMCEN = 1 << 0,
            $DIMJOGANG = 1 << 1,
        };
        enum CenterMarkDrawingMode {
            DRAW_NOTHING,
            DRAW_CENTERLINES,
            DRAW_CENTERMARKS
        };

        Radial() = default;

        double centerCenterMarkOrLineSize() const {return DIMCEN;}
        void setCenterMarkOrLineSize(double dimcen);
        double transverseSegmentAngleInJoggedRadius() const {return DIMJOGANG;}
        void setTransverseSegmentAngleInJoggedRadius(double dimjogang);

        CenterMarkDrawingMode drawingMode() const;
        double size() const {return std::abs(DIMCEN);}
        void fillByDefaults();
        void merge(const Radial* parent);
        void copyTo(Radial* radial);
    private:
        /** Controls drawing of circle or arc center marks and centerlines by the DIMCENTER, DIMDIAMETER, and DIMRADIUS commands.
        Initial value:	0.0900 (imperial) or 2.5000 (metric)
        For DIMDIAMETER and DIMRADIUS, the center mark is drawn only if you place the dimension line outside the circle or arc.
            0 No center marks or lines are drawn
            <0 Centerlines are drawn
            >0 Center marks are drawn

        The absolute value specifies the size of the center mark or centerline.
        The size of the centerline is the length of the centerline segment that extends outside the circle or arc.
        It is also the size of the gap between the center mark and the start of the centerline.
        The size of the center mark is the distance from the center of the circle or
        arc to the end of the center mark.
        https://help.autodesk.com/view/ACD/2018/ENU/?guid=GUID-9A8AB1F2-4754-444C-B90D-CD3F2FC8A3E0
        */
        double DIMCEN{2.5};            /*!< code 141 */

        /** Determines the angle of the transverse segment of the dimension line in a jogged radius dimension.
         Initial value:	45
         Jogged radius dimensions are often created when the center point is located off the page. Valid settings range is 5 to 90.
         */
        double DIMJOGANG {45}; /* DIMJOGANG, stored in ExtData for DimStyle */
    };

    class Arc : public ModificationAware{
    public:
        enum Fields: unsigned{
            $DIMARCSYM = 1 << 0,
        };
        Arc() = default;

        enum DimArcSymbolPositionPolicy {
            BEFORE = 0,
            ABOVE = 1,
            NONE = 2
        };

        DimArcSymbolPositionPolicy arcSymbolPosition() const {return DIMARCSYM;}
        void setArcSymbolPosition(DimArcSymbolPositionPolicy dimarcsym);
        void setArcSymbolPositionRaw(int dimarcsym);
        void fillByDefaults();
        void merge(const Arc* parent);
        void copyTo(Arc* arc);
    private:
        /** Controls display of the arc symbol in an arc length dimension.
         Initial value:	0
         0 Places arc length symbols before the dimension text
         1 Places arc length symbols above the dimension text
         2 Suppresses the display of arc length symbols
        */
        DimArcSymbolPositionPolicy DIMARCSYM {BEFORE}; // DIMARCSYM - code 90 !!!! 2 - none, 1 - above, 0 (if empty) - preceeding
    };

    void fillByDefaults();
    void merge(const LC_DimStyle* src);
    void mergeWith(const LC_DimStyle* src, ModificationAware::CheckFlagMode mergeMode, ModificationAware::CheckFlagMode nextMode);
    ModificationAware::CheckFlagMode getModifyCheckMode();
    void copyTo(LC_DimStyle* copy);
    void resetFlags(bool toUnset = true);
    LC_DimStyle* getCopy();
    const QString& getName() const;
    void setName(const QString& name);
    static QString getDimStyleNameSuffixForType(RS2::EntityType dimType);
    static void parseStyleName(const QString& fullName, QString& baseName, RS2::EntityType& dimensionType);
    static QString getStyleNameForBaseAndType(const QString& baseName, RS2::EntityType dimType);
    RS2::EntityType getDimensionType();
    QString getBaseName();
    bool isBaseStyle();

    AngularFormat* angularFormat() const {return m_angularUnitFormattingStyle.get();}
    Arrowhead* arrowhead() const {return m_arrowheadStyle.get();}
    Arc* arc() const {return m_arcStyle.get();}
    DimensionLine* dimensionLine() const {return m_dimensionLineStyle.get();}
    ExtensionLine* extensionLine() const {return m_extensionLineStyle.get();}
    Leader* leader() const {return m_leaderStyle.get();}
    LatteralTolerance* latteralTolerance() const {return m_latteralToleranceStyle.get();}
    MLeader* mleader() const {return m_mleaderStyle.get();}
    Radial* radial() const {return m_radialStyle.get();}
    LinearRoundOff* roundOff() const {return m_roundOffStyle.get();}
    Scaling* scaling() const {return m_scalingStyle.get();}
    Text* text() const {return m_textStyle.get();}
    LinearFormat* linearFormat() const {return m_unitFormattingStyle.get();}
    Fractions* fractions() const {return m_unitFractionsStyle.get();}
    ZerosSuppression* zerosSuppression() const {return m_unitZeroSuppressionStyle.get();}

    void setModifyCheckMode(ModificationAware::CheckFlagMode mode);

    bool isFromVars(){return m_fromVars;}
    void setFromVars(bool v){m_fromVars = v;}

    /*
    int getDimunit() const;
    void setDimunit(int dimunit);

    int getDimfit() const;
    void setDimfit(int dimfit);*/

protected:
        /* handle are code 105 */
        int dimunit;              /*!< code 270 R13+ (obsolete 2000+, use dimlunit & dimfrac) */
        int dimfit; /*!< code 287 R13+  (obsolete 2000+, use dimatfit & dimtmove)*/

        // Displays the unit type (imperial/standard or ISO-25/metric) used by dimensions in the drawing.
        // Initial value:	Standard (imperial) or ISO-25 (metric)
        // This system variable has the same name as a command. Use the SETVAR command to access this system variable.
        // The DIMSTYLE system variable is read-only; to change the current dimension style, use the DIMSTYLE command.
        int dimstyle;

private:
    QString m_name{""};
    bool m_fromVars = false;
    std::unique_ptr<AngularFormat> m_angularUnitFormattingStyle;
    std::unique_ptr<Arrowhead> m_arrowheadStyle;
    std::unique_ptr<Arc> m_arcStyle;
    std::unique_ptr<DimensionLine> m_dimensionLineStyle;
    std::unique_ptr<ExtensionLine> m_extensionLineStyle;
    std::unique_ptr<Leader> m_leaderStyle;
    std::unique_ptr<LatteralTolerance> m_latteralToleranceStyle;
    std::unique_ptr<MLeader> m_mleaderStyle;
    std::unique_ptr<Radial> m_radialStyle;
    std::unique_ptr<LinearRoundOff> m_roundOffStyle;
    std::unique_ptr<Scaling> m_scalingStyle;
    std::unique_ptr<Text> m_textStyle;
    std::unique_ptr<LinearFormat> m_unitFormattingStyle;
    std::unique_ptr<Fractions> m_unitFractionsStyle;
    std::unique_ptr<ZerosSuppression> m_unitZeroSuppressionStyle;
};

#endif // LC_DIMSTYLE_H
