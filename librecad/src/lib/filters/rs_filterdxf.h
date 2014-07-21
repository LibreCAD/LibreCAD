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


#ifndef RS_FILTERDXF_H
#define RS_FILTERDXF_H

#include "rs_filterinterface.h"

#include "rs_block.h"
#include "rs_color.h"
#include "rs_dimension.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_text.h"
#include "rs_image.h"

#include "dl_creationinterface.h"
#include "dl_dxf.h"

class RS_Spline;
class LC_SplinePoints;
class RS_Hatch;
class DL_WriterA;

/**
 * This format filter class can import and export DXF files.
 * It depends on the dxflib library.
 *
 * @author Andrew Mustun
 */
class RS_FilterDXF : public RS_FilterInterface, DL_CreationInterface {
public:
    RS_FilterDXF();
    ~RS_FilterDXF();
	
    virtual bool canImport(const QString &/*fileName*/, RS2::FormatType t) const {
        return (t==RS2::FormatDXFOLD);
	}
	
	virtual bool canExport(const QString &fileName, RS2::FormatType t) const {
        return (t==RS2::FormatDXFOLD || t==RS2::FormatDXFOLD12);
    }

    // Import:
    virtual bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    // Methods from DL_CreationInterface:
    virtual void addLayer(const DL_LayerData& data);
    virtual void addBlock(const DL_BlockData& data);
    virtual void endBlock();
    virtual void addPoint(const DL_PointData& data);
    virtual void addLine(const DL_LineData& data);
    virtual void addArc(const DL_ArcData& data);
    virtual void addEllipse(const DL_EllipseData& data);
    virtual void addCircle(const DL_CircleData& data);
    virtual void addPolyline(const DL_PolylineData& data);
    virtual void addVertex(const DL_VertexData& data);
    virtual void addSpline(const DL_SplineData& data);
    virtual void addKnot(const DL_KnotData&) {}
    virtual void addControlPoint(const DL_ControlPointData& data);
    virtual void addInsert(const DL_InsertData& data);
    virtual void addTrace(const DL_TraceData& ) {}
    virtual void addSolid(const DL_SolidData& ) {}
    virtual void addMTextChunk(const char* text);
    virtual void addMText(const DL_MTextData& data);
    virtual void addText(const DL_TextData& data);
    //virtual void addDimension(const DL_DimensionData& data);
    RS_DimensionData convDimensionData(const DL_DimensionData& data);
    virtual void addDimAlign(const DL_DimensionData& data,
                             const DL_DimAlignedData& edata);
    virtual void addDimLinear(const DL_DimensionData& data,
                              const DL_DimLinearData& edata);
    virtual void addDimRadial(const DL_DimensionData& data,
                              const DL_DimRadialData& edata);
    virtual void addDimDiametric(const DL_DimensionData& data,
                                 const DL_DimDiametricData& edata);
    virtual void addDimAngular(const DL_DimensionData& data,
                               const DL_DimAngularData& edata);
    virtual void addDimAngular3P(const DL_DimensionData& data,
                                 const DL_DimAngular3PData& edata);
    virtual void addLeader(const DL_LeaderData& data);
    virtual void addLeaderVertex(const DL_LeaderVertexData& data);
    virtual void addHatch(const DL_HatchData& data);
    virtual void addHatchLoop(const DL_HatchLoopData& data);
    virtual void addHatchEdge(const DL_HatchEdgeData& data);
    virtual void addImage(const DL_ImageData& data);
    virtual void linkImage(const DL_ImageDefData& data);
    virtual void endEntity();
    virtual void endSequence() {}

    virtual void add3dFace(const DL_3dFaceData& data);
    virtual void addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&);
    virtual void addComment(const char*);

    virtual void setVariableVector(const char* key,
                                   double v1, double v2, double v3, int code);
    virtual void setVariableString(const char* key, const char* value, int code);
    virtual void setVariableInt(const char* key, int value, int code);
    virtual void setVariableDouble(const char* key, double value, int code);

    // Export:
    virtual bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type);

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
	void writeText(DL_WriterA& dw, RS_Text* t, const DL_Attributes& attrib);
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
    static QString toNativeString(const char* data, const QString& encoding);
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

    static RS_FilterInterface* createFilter(){return new RS_FilterDXF();}

private:
    /** Pointer to the graphic we currently operate on. */
    RS_Graphic* graphic;
	/** File name. Used to find out the full path of images. */
        QString file;
    /** string for concatinating text parts of MTEXT entities. */
    QString mtext;
    /** Pointer to current polyline entity we're adding vertices to. */
    RS_Polyline* polyline;
    /** Pointer to current spline entity we're adding control points to. */
    RS_Spline* spline;
	LC_SplinePoints* splinePoints;
    /** Pointer to current leader entity we're adding vertices to. */
    RS_Leader* leader;
    /** Pointer to current entity container (either block or graphic) */
    RS_EntityContainer* currentContainer;

    /** Pointer to current hatch or NULL. */
    RS_Hatch* hatch;
    /** Pointer to current hatch loop or NULL. */
    RS_EntityContainer* hatchLoop;

    DL_Dxf dxf;
    RS_VariableDict variables;
    bool omitHatchLoop;
}
;

#endif
