/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Douglas B. Geiger ()
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


#ifndef RS_ALIGNEDTEXT_H
#define RS_ALIGNEDTEXT_H

#include "rs_entitycontainer.h"

class RS_MText;
class RS_Text;

/**
 * Holds the data that defines an aligned text entity.
 */
struct RS_AlignedTextData {

	/**
	 * Default constructor. Leaves the data object uninitialized.
	 */
	RS_AlignedTextData() = default;

	/**
	 * Constructor with initialisation.
	 *
	 * @param textEntity - text entity being aligned
	 * @param entity - geometry entity is being aligned to
	 * @param offset - offset of text from geometry
	 * @param above - relative position of text to geometry
	 * @param updateMode RS2::Update will update the text entity instantly
	 *    RS2::NoUpdate will not update the entity. You can update
	 *    it later manually using the update() method. This is
	 *    often the case since you might want to adjust attributes
	 *    after creating a text entity.
	 */
	RS_AlignedTextData(RS_Entity *_textEntity,
				 RS_Entity *_entity,
				 RS_Vector _insertionPoint,
				 double _offset,
				 bool _above,
				 RS2::UpdateMode updateMode = RS2::Update);

	/** Text entity */
	RS_Entity *textEntity;
	/** Geometry entity */
	RS_Entity *entity;
	/** insertion point */
	RS_Vector insertionPoint;
	/** Offset from geometry */
	double offset;
	/** Above/below geometry */
	bool above;
	/** Update mode */
	RS2::UpdateMode updateMode;
};

std::ostream& operator << (std::ostream& os, const RS_AlignedTextData& td);


/**
 * Class for an aligned text entity.
 *
 * @author Doug Geiger
 */
class RS_AlignedText : public RS_EntityContainer {
public:
    RS_AlignedText(RS_EntityContainer* parent,
            const RS_AlignedTextData& d);
	virtual ~RS_AlignedText();   // = default;

    virtual RS_Entity* clone() const override;

    /**	@return RS2::EntityAlignedText */
    virtual RS2::EntityType rtti() const override{
        return RS2::EntityAlignedText;
    }

    /** @return Copy of data that defines the text. */
    RS_AlignedTextData getData() const {
        return data;
    }

    void update() override;

	RS_Vector getInsertionPoint() {
		return data.insertionPoint;
	}

	bool AlignedMText()
	{
		return (data.textEntity && data.textEntity->rtti() == RS2::EntityMText);
	}

	bool AlignedText()
	{
		return (data.textEntity && data.textEntity->rtti() == RS2::EntityText);
	}

	RS_Entity *getTextEntity()
	{
		return (data.textEntity);
	}

	RS_Entity *getGeometryEntity()
	{
		return (data.entity);
	}
	
	RS_MText *getRSMText()
	{
		if (AlignedMText())
			return ((RS_MText *)(getTextEntity()));
		else
			return (0);
	}
	RS_Text *getRSText()
	{
		if (AlignedText())
			return ((RS_Text *)(getTextEntity()));
		else
			return (0);
	}

	void setOffset(double _offset)
	{
		data.offset = _offset;
	}

	double getOffset()
	{
		return (data.offset);
	}

	void setAbove(bool _above)
	{
		data.above = _above;
	}

	bool Above()
	{
		return (data.above);
	}

#if 0
     int getNumberOfLines();


    RS_Vector getInsertionPoint() {
        return data.insertionPoint;
    }
    double getHeight() {
        return data.height;
    }
    void setHeight(double h) {
        data.height = h;
    }
    double getWidth() {
        return data.width;
    }
    void setAlignment(int a);
    int getAlignment();
    RS_MTextData::VAlign getVAlign() {
        return data.valign;
    }
    void setVAlign(RS_MTextData::VAlign va) {
        data.valign = va;
    }
    RS_MTextData::HAlign getHAlign() {
        return data.halign;
    }
    void setHAlign(RS_MTextData::HAlign ha) {
        data.halign = ha;
    }
    RS_MTextData::MTextDrawingDirection getDrawingDirection() {
        return data.drawingDirection;
    }
    RS_MTextData::MTextLineSpacingStyle getLineSpacingStyle() {
        return data.lineSpacingStyle;
    }
    void setLineSpacingFactor(double f) {
        data.lineSpacingFactor = f;
    }
    double getLineSpacingFactor() {
        return data.lineSpacingFactor;
    }
    void setText(const QString& t);
    QString getText() {
        return data.text;
    }
    void setStyle(const QString& s) {
        data.style = s;
    }
    QString getStyle() {
        return data.style;
    }
    void setAngle(double a) {
        data.angle = a;
    }
    double getAngle() {
        return data.angle;
    }
    double getUsedTextWidth() {
        return usedTextWidth;
    }
    double getUsedTextHeight() {
        return usedTextHeight;
    }

//	virtual double getLength() const {
//		return -1.0;
//	}

#endif
    /**
     * @return The insertion point as endpoint.
     */
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL) const override;
    virtual RS_VectorSolutions getRefPoints() const override;

    virtual void move(const RS_Vector& offset) override;
    virtual void rotate(const RS_Vector& center, const double& angle) override;
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    virtual void scale(const RS_Vector& center, const RS_Vector& factor) override;
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    virtual bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) override;
    virtual void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset) override;

    friend std::ostream& operator << (std::ostream& os, const RS_Text& p);

    void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

private:
//    double updateAddLine(RS_EntityContainer* textLine, int lineCounter);

protected:
    RS_AlignedTextData data;

};

#endif

