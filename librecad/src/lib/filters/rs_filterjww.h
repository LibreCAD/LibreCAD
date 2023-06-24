/****************************************************************************
** $Id: rs_filterjww.h,v 1.1.1.2 2010/02/08 11:58:24 zeronemo2007 Exp $
**
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file is part of the qcadlib Library project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid qcadlib Professional Edition licenses may use 
** this file in accordance with the qcadlib Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ribbonsoft.com for further details.
**
** Contact info@ribbonsoft.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/


#ifndef RS_FILTERJWW_H
#define RS_FILTERJWW_H

#include "rs_filterinterface.h"

#include "rs_color.h"
#include "rs_dimension.h"

#include "dl_creationinterface.h"
#include "dl_jww.h"


class RS_Point;
class RS_Line;
class RS_Circle;
class RS_Arc;
class RS_Ellipse;
class RS_Solid;
class RS_Polyline;
class RS_Spline;
class LC_SplinePoints;
class RS_Insert;
class RS_MText;
class RS_Text;
class RS_Hatch;
class RS_Image;
class RS_Leader;
class RS_Polyline;
class DL_WriterA;

/**
 * This format filter class can import and export JWW files.
 * It depends on the jwwlib library.
 *
 * @author Andrew Mustun
 */
class RS_FilterJWW : public RS_FilterInterface, DL_CreationInterface {
public:
    RS_FilterJWW();
    ~RS_FilterJWW();
	
	/**
	 * @return RS2::FormatJWW.
	 */
	//RS2::FormatType rtti() const{
	//	return RS2::FormatJWW;
	//}

	/*
    virtual bool canImport(RS2::FormatType t) {
		return (t==RS2::FormatJWW);
	}
	
    virtual bool canExport(RS2::FormatType t) {
		return (t==RS2::FormatJWW || t==RS2::FormatJWW12);
	}*/

    // Import:
    bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) override;

    // Methods from DL_CreationInterface:
    void addLayer(const DL_LayerData& data) override;
    void addBlock(const DL_BlockData& data) override;
    void endBlock() override;
    void addPoint(const DL_PointData& data) override;
    void addLine(const DL_LineData& data) override;
    void addArc(const DL_ArcData& data) override;
    void addEllipse(const DL_EllipseData& data) override;
    void addCircle(const DL_CircleData& data) override;
    void addPolyline(const DL_PolylineData& data) override;
    void addVertex(const DL_VertexData& data) override;
    void addSpline(const DL_SplineData& data) override;
    void addKnot(const DL_KnotData&) override {}
    void addControlPoint(const DL_ControlPointData& data) override;
    void addInsert(const DL_InsertData& data) override;
    void addTrace(const DL_TraceData& ) override {}
    void addSolid(const DL_SolidData& ) override {}
    void addMTextChunk(const char* text) override;
    void addMText(const DL_MTextData& data) override;
    void addText(const DL_TextData& data) override;
    //virtual void addDimension(const DL_DimensionData& data);
    RS_DimensionData convDimensionData(const DL_DimensionData& data);
    void addDimAlign(const DL_DimensionData& data, const DL_DimAlignedData& edata) override;
    void addDimLinear(const DL_DimensionData& data, const DL_DimLinearData& edata) override;
    void addDimRadial(const DL_DimensionData& data, const DL_DimRadialData& edata) override;
    void addDimDiametric(const DL_DimensionData& data, const DL_DimDiametricData& edata) override;
    void addDimAngular(const DL_DimensionData& data, const DL_DimAngularData& edata) override;
    void addDimAngular3P(const DL_DimensionData& data, const DL_DimAngular3PData& edata) override;
    void addLeader(const DL_LeaderData& data) override;
    void addLeaderVertex(const DL_LeaderVertexData& data) override;
    void addHatch(const DL_HatchData& data) override;
    void addHatchLoop(const DL_HatchLoopData& data) override;
    void addHatchEdge(const DL_HatchEdgeData& data) override;
    void addImage(const DL_ImageData& data) override;
    void linkImage(const DL_ImageDefData& data) override;
    void endEntity() override;
    void endSequence() override {}

    void add3dFace(const DL_3dFaceData& data) override;
    void addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) override;
    void addComment(const char*) override;

    void setVariableVector(const char* key,
                                   double v1, double v2, double v3, int code) override;
    void setVariableString(const char* key, const char* value, int code) override;
    void setVariableInt(const char* key, int value, int code) override;
    void setVariableDouble(const char* key, double value, int code) override;

    // Export:
    bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) override;

    void writeVariables(DL_WriterA& dw);
    void writeLayer(DL_WriterA& dw, RS_Layer* l);
    void writeLineType(DL_WriterA& dw, RS2::LineType t);
    void writeAppid(DL_WriterA& dw, const char* appid);
    void writeBlock(DL_WriterA& dw, RS_Block* blk);
    void writeEntity(DL_WriterA& dw, RS_Entity* e);
    void writeEntity(DL_WriterA& dw, RS_Entity* e, const DL_Attributes& attrib);
	void writePoint(DL_WriterA& dw, RS_Point* p, const DL_Attributes& attrib);
	void writeLine(DL_WriterA& dw, RS_Line* l, const DL_Attributes& attrib);
	void writePolyline(DL_WriterA& dw, 
		RS_Polyline* l, const DL_Attributes& attrib);
	void writeSpline(DL_WriterA& dw, 
		RS_Spline* s, const DL_Attributes& attrib);
	void writeSplinePoints(DL_WriterA& dw,
		LC_SplinePoints* s, const DL_Attributes& attrib);
	void writeCircle(DL_WriterA& dw, RS_Circle* c, const DL_Attributes& attrib);
	void writeArc(DL_WriterA& dw, RS_Arc* a, const DL_Attributes& attrib);
	void writeEllipse(DL_WriterA& dw, RS_Ellipse* s, const DL_Attributes& attrib);
	void writeInsert(DL_WriterA& dw, RS_Insert* i, const DL_Attributes& attrib);
    void writeText(DL_WriterA& dw, RS_MText* t, const DL_Attributes& attrib);
	void writeDimension(DL_WriterA& dw, RS_Dimension* d, 
		const DL_Attributes& attrib);
	void writeLeader(DL_WriterA& dw, RS_Leader* l, const DL_Attributes& attrib);
	void writeHatch(DL_WriterA& dw, RS_Hatch* h, const DL_Attributes& attrib);
	void writeSolid(DL_WriterA& dw, RS_Solid* s, const DL_Attributes& attrib);
	void writeImage(DL_WriterA& dw, RS_Image* i, const DL_Attributes& attrib);
	void writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con, 
		const DL_Attributes& attrib);
	void writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c, 
		const DL_Attributes& attrib, RS2::ResolveLevel level);
	
    void writeImageDef(DL_WriterA& dw, RS_Image* i);

    void setEntityAttributes(RS_Entity* entity, const DL_Attributes& attrib);
    DL_Attributes getEntityAttributes(RS_Entity* entity);

    static QString toDxfString(const QString& string);
    QString toNativeString(const char* data, const QString& encoding);
    QString getDXFEncoding();

public:
    RS_Pen attributesToPen(const DL_Attributes& attrib) const;

    static RS_Color numberToColor(int num, bool comp=false);
    static int colorToNumber(const RS_Color& col);

    static RS2::LineType nameToLineType(const QString& name);
    static QString lineTypeToName(RS2::LineType lineType);
    //static QString lineTypeToDescription(RS2::LineType lineType);

    static RS2::LineWidth numberToWidth(int num);
    static int widthToNumber(RS2::LineWidth width);

	static RS2::AngleFormat numberToAngleFormat(int num);
	static int angleFormatToNumber(RS2::AngleFormat af);

	static RS2::Unit numberToUnit(int num);
	static int unitToNumber(RS2::Unit unit);
	
	static bool isVariableTwoDimensional(const QString& var);

    bool canImport(const QString & /*fileName*/, RS2::FormatType t) const override {
        return (t==RS2::FormatJWW);
    }

    bool canExport(const QString& /*fileName*/, RS2::FormatType t) const override {
        return (t==RS2::FormatJWW);
    }

    static RS_FilterInterface *createFilter() {return new RS_FilterJWW();}
private:
    /** Pointer to the graphic we currently operate on. */
    RS_Graphic* graphic = nullptr;
	/** File name. Used to find out the full path of images. */
	QString file;
    /** string for concatinating text parts of MTEXT entities. */
    QString mtext;
    /** Pointer to current polyline entity we're adding vertices to. */
    RS_Polyline* polyline = nullptr;
    /** Pointer to current spline entity we're adding control points to. */
    RS_Spline* spline = nullptr;
    LC_SplinePoints* splinePoints = nullptr;
    /** Pointer to current leader entity we're adding vertices to. */
    RS_Leader* leader = nullptr;
    /** Pointer to current entity container (either block or graphic) */
    RS_EntityContainer* currentContainer = nullptr;

    /** Pointer to current hatch or NULL. */
    RS_Hatch* hatch = nullptr;
    /** Pointer to current hatch loop or NULL. */
    RS_EntityContainer* hatchLoop = nullptr;

    DL_Jww jww;
    RS_VariableDict variables;
};

#endif
