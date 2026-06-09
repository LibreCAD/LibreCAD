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

#ifndef RS_DIMENSION_H
#define RS_DIMENSION_H

#include "lc_dimstyle.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_mtext.h"

struct RS_ArcData;
class RS_Arc;
class RS_Color;
class RS_Line;

/**
 * Holds the data that is common to all dimension entities.
 */
// fixme - sand - no assignment operator!
struct RS_DimensionData : RS_Flags {
    /**
  * Default constructor
     */
    RS_DimensionData();
    RS_DimensionData(const RS_DimensionData& other);

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point.
     * @param middleOfText Middle point of dimension text.
     * @param valign Vertical alignment.
     * @param halign Horizontal alignment.
     * @param lineSpacingStyle Line spacing style.
     * @param lineSpacingFactor Line spacing factor.
     * @param text Text string entered explicitly by user or null
     *         or "<>" for the actual measurement or " " (one blank space).
     *         for suppressing the text.
     * @param style Dimension style name.
     * @param angle Rotation angle of dimension text away from
     *         default orientation.
     * @param hdir
     * @param autoTextLocation
     * @param dsOverride
     * @param flipArr1
     * @param flipArr2
     */
    RS_DimensionData(const RS_Vector& definitionPoint, const RS_Vector& middleOfText, RS_MTextData::VAlign valign,
                     RS_MTextData::HAlign halign, RS_MTextData::MTextLineSpacingStyle lineSpacingStyle, double lineSpacingFactor,
                     const QString& text, const QString& style, double angle, double hdir, bool autoTextLocation, LC_DimStyle* dsOverride,
                     bool flipArr1, bool flipArr2);

    /** Definition point */
    RS_Vector definitionPoint;
    /** Middle point of dimension text */
    RS_Vector middleOfText;
    /** Vertical alignment */
    RS_MTextData::VAlign valign = RS_MTextData::VABottom;
    /** Horizontal alignment */
    RS_MTextData::HAlign halign = RS_MTextData::HALeft;
    /** Line spacing style */
    RS_MTextData::MTextLineSpacingStyle lineSpacingStyle = RS_MTextData::Exact;
    /** Line spacing factor */
    double lineSpacingFactor = 0.;
    /**
    * Text string entered explicitly by user or null
    * or "<>" for the actual measurement or " " (one blank space)
    * for suppressing the text.
    */
    QString text;
    /** Dimension style name */
    QString style;
    /** Rotation angle of dimension text away from default orientation */
    double angle = 0.;
    /**
     * direction of horizontal coordinate axis
     */
    double horizontalAxisDirection = 0.0;
    bool autoText = true;
    std::unique_ptr<LC_DimStyle> dimStyleOverride;

    bool flipArrow1{false};
    bool flipArrow2{false};
};

std::ostream& operator <<(std::ostream& os, const RS_DimensionData& dd);

/**
 * Abstract base class for dimension entity classes.
 *
 * @author Andrew Mustun
 */
// fixme - sand - no copy assignment operator!
class RS_Dimension : public RS_EntityContainer {
public:
    RS_Dimension(RS_EntityContainer* parent, const RS_DimensionData& d);
    RS_Dimension(const RS_Dimension& entity);

    /** @return Copy of data that defines the dimension. */
    RS_DimensionData getGenericData() const {
        return m_dimGenericData;
    }

    QString getLabel(bool resolve = true);
    void setLabel(const QString& l);

    /**
     * Needs to be implemented by the dimension class to return the
     * measurement of the dimension (e.g. 10.5 or 15'14").
     */
    virtual QString getMeasuredLabel() = 0;

    /**
     * Must be overwritten by implementing dimension entity class
     * to update the subentities which make up the dimension entity.
     */
    void update() override;
    LC_DimStyle* getGlobalDimStyle() const;
    LC_DimStyle* getEffectiveDimStyle() const;
    LC_DimStyle* getEffectiveCachedDimStyle();
    LC_DimStyle* getEffectiveDimStyleOverride();
    void clearCachedDimStyle();
    void resolveEffectiveDimStyleAndUpdateDim();
    void updateDim(bool autoText = false);

    RS_Vector getDefinitionPoint() const {
        return m_dimGenericData.definitionPoint;
    }

    RS_Vector getMiddleOfText() const {
        return m_dimGenericData.middleOfText;
    }

    void setMiddleOfText(const RS_Vector& v) {
        m_dimGenericData.middleOfText = v;
    }

    RS_MTextData::VAlign getVAlign() const {
        return m_dimGenericData.valign;
    }

    RS_MTextData::HAlign getHAlign() const {
        return m_dimGenericData.halign;
    }

    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() const {
        return m_dimGenericData.lineSpacingStyle;
    }

    double getLineSpacingFactor() const {
        return m_dimGenericData.lineSpacingFactor;
    }

    QString getText() const {
        return m_dimGenericData.text;
    }

    QString getStyle() const {
        return m_dimGenericData.style;
    }

    double getTextAngle() const {
        return m_dimGenericData.angle;
    }

    void setTextAngle(const double angle) {
        m_dimGenericData.angle = angle;
    }

    double getHDir() const {
        return m_dimGenericData.horizontalAxisDirection;
    }

    bool hasUserDefinedTextLocation() const {
        return !m_dimGenericData.autoText;
    }

    void setHDir(const double hdir) {
        m_dimGenericData.horizontalAxisDirection = hdir;
    }

    void setDefinitionPoint(const RS_Vector& defPoint) {
        m_dimGenericData.definitionPoint = defPoint;
    }

    void setStyle(const QString& style) {
        m_dimGenericData.style = style;
    }

    bool isFlipArrow1() const {
        return m_dimGenericData.flipArrow1;
    }

    bool isFlipArrow2() const {
        return m_dimGenericData.flipArrow2;
    }

    void setFlipArrow1(const bool val) {
        m_dimGenericData.flipArrow1 = val;
    }

    void setFlipArrow2(const bool val) {
        m_dimGenericData.flipArrow2 = val;
    }

    double getGeneralFactor() const;
    double getGeneralScale() const;
    double getArrowSize() const;
    double getTickSize() const;
    double getExtensionLineExtension() const;
    double getExtensionLineOffset() const;
    double getDimensionLineGap() const;
    double getVerticalDistanceToDimLine() const;
    double getTextHeight() const;
    bool getInsideHorizontalText() const;
    bool getFixedLengthOn() const;
    double getFixedLength() const;

    double getMeasurement() const {
        return m_dimMeasurement;
    }

    RS2::LineWidth getExtensionLineWidth() const;
    RS2::LineType getExtensionLineTypeFirst() const;
    RS2::LineType getExtensionLineTypeSecond() const;
    RS2::LineWidth getDimensionLineWidth() const;
    RS2::LineType getDimensionLineType() const;
    RS_Color getDimensionLineColor() const;
    RS_Color getExtensionLineColor() const;
    RS_Color getTextColor() const;
    QString getTextStyle() const;
    RS2::LinearFormat getDimLinearFormat() const;
    int getDimDecimalPlaces() const;
    int getDimTrailingZerosSuppressionMode() const;
    int getDimDecimalFormatSeparatorChar() const;

    double getGraphicVariable(const QString& key, double defMM, int code) const;
    static QString stripZerosAngle(QString angle, int zeros = 0);
    static QString stripZerosLinear(QString linear, int zeros = 1);
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    RS_Entity& shear([[maybe_unused]] double k) override {
        return *this;
    } // TODO

    LC_DimStyle* getDimStyleOverride() const {
        return m_dimGenericData.dimStyleOverride.get();
    }

    void setDimStyleOverride(const LC_DimStyle* dimStyleOverride) {
        if (dimStyleOverride == nullptr) {
            m_dimGenericData.dimStyleOverride.reset(nullptr);
        }
        else {
            m_dimGenericData.dimStyleOverride.reset(dimStyleOverride->getCopy());
        }
    }

    void reparent(RS_EntityContainer* newParent) override {
        RS_Entity::reparent(newParent);
    }

private:
    static RS_VectorSolutions getIntersectionsLineContainer(const RS_Line* l, const RS_EntityContainer* c, bool infiniteLine = false);
    void createHorizontalTextDimensionLine(const RS_Vector& p1, const RS_Vector& p2, bool showArrow1, bool showArrow2, bool showLine1,
                                           bool showLine2, bool forceAutoText = false);
    void determineTextAreaBounds(RS_VectorSolutions& sol, const RS_MText* text, double dimGap);
    void createAlignedTextDimensionLine(const RS_Vector& p1, const RS_Vector& p2, bool showArrow1, bool showArrow2, bool showLine1,
                                        bool showLine2, bool forceAutoText = false);
    RS_Vector doGetNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;
    RS_Vector doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const override;
protected:
    /** Data common to all dimension entities. */
    RS_DimensionData m_dimGenericData;
    // dim style used during updateDim()
    LC_DimStyle* m_dimStyleTransient = nullptr;

    double m_dimMeasurement = 0.0;

    virtual void doUpdateDim() = 0;

    RS_Pen getPenForText() const;
    RS_Pen getPenExtensionLine(bool first) const;
    RS_Pen getPenDimensionLine() const;
    RS_MText* createDimText(const RS_Vector& textPos, double textHeight, double textAngle);
    void addDimComponentEntity(RS_Entity* en, const RS_Pen& pen);
    RS_MText* addDimText(const RS_MTextData& textData);
    RS_MTextData createDimTextData(const RS_Vector& textPos, double textHeight, double textAngle);
    RS_Line* addDimExtensionLine(const RS_Vector& start, const RS_Vector& end, bool first);
    RS_Line* addDimDimensionLine(const RS_Vector& start, const RS_Vector& end);
    RS_Line* addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen& pen);
    RS_Arc* addDimArc(const RS_ArcData& arcData);
    void addBoundsAroundText(double dimgap, const RS_MText* text);
    void addArrow(RS_Entity* arrow, const RS_Pen& dimensionPen);
    QString createLinearMeasuredLabel(double dist) const;
    double prepareLabelLinearDistance(double distance) const;
    void createDimensionLine(const RS_Vector& dimLineStart, const RS_Vector& dimLineEnd, bool showArrow1 = true, bool showArrow2 = true,
                             bool showLine1 = false, bool showLine2 = false, bool forceAutoText = false);
};

#endif
