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
struct RS_DimensionData : public RS_Flags {
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
     */
	RS_DimensionData(const RS_Vector& definitionPoint,
                     const RS_Vector& middleOfText,
                     RS_MTextData::VAlign valign,
                     RS_MTextData::HAlign halign,
                     RS_MTextData::MTextLineSpacingStyle lineSpacingStyle,
                     double lineSpacingFactor,
                     QString text,
                     QString style,
                     double angle,
                     double hdir,
                     bool autoTextLocation,
                     LC_DimStyle* dimStyleOverride,
                     bool flipArr1,
                     bool flipArr2);

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
    std::unique_ptr<LC_DimStyle> m_dimStyleOverride;

    bool flipArrow1{false};
    bool flipArrow2{false};
};

std::ostream& operator << (std::ostream& os,
								  const RS_DimensionData& dd);

/**
 * Abstract base class for dimension entity classes.
 *
 * @author Andrew Mustun
 */
class RS_Dimension : public RS_EntityContainer {
public:
    RS_Dimension(RS_EntityContainer* parent,const RS_DimensionData& d);
    RS_Dimension(const RS_Dimension& entity);

    RS_Vector getNearestRef( const RS_Vector& coord, double* dist = nullptr) const override;
    RS_Vector getNearestSelectedRef( const RS_Vector& coord, double* dist = nullptr) const override;

    /** @return Copy of data that defines the dimension. */
    RS_DimensionData getData() const {return m_dimGenericData;}
    QString getLabel(bool resolve=true);
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
    LC_DimStyle* getGlobalDimStyle();
    LC_DimStyle* getEffectiveDimStyle();
    void resolveEffectiveDimStyleAndUpdateDim();
    void updateDim(bool autoText=false);

    RS_Vector getDefinitionPoint() {return m_dimGenericData.definitionPoint;}
    RS_Vector getMiddleOfText() {return m_dimGenericData.middleOfText;}
    RS_MTextData::VAlign getVAlign() {return m_dimGenericData.valign;}
    RS_MTextData::HAlign getHAlign() {return m_dimGenericData.halign;}
    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() {return m_dimGenericData.lineSpacingStyle;}
    double getLineSpacingFactor() {return m_dimGenericData.lineSpacingFactor;}
    QString getText() {return m_dimGenericData.text;}
    QString getStyle() const {return m_dimGenericData.style;}
    double getAngle() {return m_dimGenericData.angle;}
    double getHDir() const{return m_dimGenericData.horizontalAxisDirection;}
    double hasUserDefinedTextLocation(){return !m_dimGenericData.autoText;}
    void setHDir(double hdir) {m_dimGenericData.horizontalAxisDirection = hdir;}
    void setDefinitionPoint(RS_Vector defPoint) {m_dimGenericData.definitionPoint = defPoint;}
    void setStyle(const QString& style){m_dimGenericData.style = style;}
    bool isFlipArrow1() const {return m_dimGenericData.flipArrow1;}
    bool isFlipArrow2() const {return m_dimGenericData.flipArrow2;}

    void setFlipArrow1(bool val){m_dimGenericData.flipArrow1 = val;}
    void setFlipArrow2(bool val){m_dimGenericData.flipArrow2 = val;}

    double getGeneralFactor();
    double getGeneralScale();
    double getArrowSize();
    double getTickSize();
    double getExtensionLineExtension();
    double getExtensionLineOffset();
    double getDimensionLineGap();
    double getVerticalDistanceToDimLine();
    double getTextHeight();
    bool getInsideHorizontalText();
    bool getFixedLengthOn();
    double getFixedLength();
    RS2::LineWidth getExtensionLineWidth();
    RS2::LineType getExtensionLineTypeFirst();
    RS2::LineType getExtensionLineTypeSecond();
    RS2::LineWidth getDimensionLineWidth();
    RS2::LineType getDimensionLineType();
    RS_Color getDimensionLineColor();
    RS_Color getExtensionLineColor();
    RS_Color getTextColor();
    QString getTextStyle();
    RS2::LinearFormat getDimLinearFormat();
    int getDimDecimalPlaces();
    int getDimTrailingZerosSuppressionMode();
    int getDimDecimalFormatSeparatorChar();

    double getGraphicVariable(const QString& key, double defMM, int code);
    static QString stripZerosAngle(QString angle, int zeros=0);
    static QString stripZerosLinear(QString linear, int zeros=1);
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear([[maybe_unused]] double k) override { return *this; } // TODO

    LC_DimStyle* getDimStyleOverride() const {
        return m_dimGenericData.m_dimStyleOverride.get();
    }

    void setDimStyleOverride(LC_DimStyle* dimStyleOverride) {
        if (dimStyleOverride == nullptr) {
            m_dimGenericData.m_dimStyleOverride.reset(nullptr);
        }
        else {
            m_dimGenericData.m_dimStyleOverride.reset(dimStyleOverride->getCopy());
        }
    }

private:
    static RS_VectorSolutions getIntersectionsLineContainer(const RS_Line* l, const RS_EntityContainer* c,
                                                            bool infiniteLine = false);
    void createHorizontalTextDimensionLine(const RS_Vector& p1, const RS_Vector& p2, bool showArrow1,
                                                 bool showArrow2,
                                                 bool showLine1, bool showLine2,
                                                 bool autoText = false);
    RS_VectorSolutions* determineTextAreaBounds(RS_MText* text, double dimGap);
    void createAlignedTextDimensionLine(const RS_Vector& p1, const RS_Vector& p2, bool showArrow1,
                                        bool showArrow2,
                                        bool showLine1, bool showLine2,
                                        bool autoText = false);
protected:
    /** Data common to all dimension entities. */
    RS_DimensionData m_dimGenericData;
    // dim style used during updateDim()
    LC_DimStyle* m_dimStyleTransient = nullptr;

    virtual void doUpdateDim() = 0;

    RS_Pen getPenForText();
    RS_Pen getPenExtensionLine(bool first);
    RS_Pen getPenDimensionLine();
    RS_MText* createDimText(RS_Vector textPos, double textHeight, double textAngle);
    void addDimComponentEntity(RS_Entity* en, const RS_Pen &pen);
    RS_MText* addDimText(RS_MTextData &textData);
    RS_MTextData createDimTextData(RS_Vector textPos, double textHeight, double textAngle);
    RS_Line* addDimExtensionLine(RS_Vector start, RS_Vector end, bool first);
    RS_Line* addDimDimensionLine(RS_Vector start, RS_Vector end);
    RS_Line* addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen& pen);
    RS_Arc* addDimArc(RS_ArcData& arcData);
    void addBoundsAroundText(double dimgap, RS_MText* text);
    void addArrow(RS_Entity* arrow, RS_Pen &dimensionPen);
    QString createLinearMeasuredLabel(double dist);
    double prepareLabelLinearDistance(double distance);
    void createDimensionLine(const RS_Vector& dimLineStart, const RS_Vector& dimLineEnd,
              bool showArrow1=true, bool showArrow2=true, bool showLine1 = false, bool showLine2 = false, bool autoText=false);
};

#endif
