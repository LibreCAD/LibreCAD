/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
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
**********************************************************************/


#ifndef RS_FILTERDXFRW_H
#define RS_FILTERDXFRW_H

#include "rs_filterinterface.h"

#include "rs_color.h"
#include "rs_dimension.h"
#include "drw_interface.h"
#include "libdxfrw.h"

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
 * This format filter class can import and export DXF files.
 * It depends on the libdxfrw library.
 *
 * @author Rallaz
 */
class RS_FilterDXFRW : public RS_FilterInterface, DRW_Interface {
public:
    RS_FilterDXFRW();
    ~RS_FilterDXFRW();
        
     bool canImport(const QString &/*fileName*/, RS2::FormatType t) const override {
#ifdef DWGSUPPORT
        return (t==RS2::FormatDXFRW || t==RS2::FormatDWG);
#else
        return (t==RS2::FormatDXFRW);
#endif
        }
        
     bool canExport(const QString &/*fileName*/, RS2::FormatType t) const override {
        return (t==RS2::FormatDXFRW || t==RS2::FormatDXFRW2004 || t==RS2::FormatDXFRW2000
                || t==RS2::FormatDXFRW14 || t==RS2::FormatDXFRW12);
    }

    // Error messages
     QString lastError() const override;

    // Import:
     bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType type) override;

    // Methods from DRW_CreationInterface:
     void addHeader(const DRW_Header* data) override;
     void addLType(const DRW_LType& /*data*/) override{}
     void addLayer(const DRW_Layer& data) override;
     void addDimStyle(const DRW_Dimstyle& data) override;
     void addVport(const DRW_Vport& data) override;
     void addTextStyle(const DRW_Textstyle& /*data*/) override{}
     void addAppId(const DRW_AppId& /*data*/) override{}
     void addBlock(const DRW_Block& data) override;
     void setBlock(const int handle) override;
     void endBlock() override;
     void addPoint(const DRW_Point& data) override;
     void addLine(const DRW_Line& data) override;
     void addRay(const DRW_Ray& data) override;
     void addXline(const DRW_Xline& data) override;
     void addCircle(const DRW_Circle& data) override;
     void addArc(const DRW_Arc& data) override;
     void addEllipse(const DRW_Ellipse& data) override;
     void addLWPolyline(const DRW_LWPolyline& data) override;
     void addText(const DRW_Text& data) override;
     void addPolyline(const DRW_Polyline& data) override;
     void addSpline(const DRW_Spline* data) override;
     void addKnot(const DRW_Entity&) override{}
     void addInsert(const DRW_Insert& data) override;
     void addTrace(const DRW_Trace& data) override;
     void addSolid(const DRW_Solid& data) override;
     void addMText(const DRW_MText& data) override;
    RS_DimensionData convDimensionData(const DRW_Dimension* data);
     void addDimAlign(const DRW_DimAligned *data) override;
     void addDimLinear(const DRW_DimLinear *data) override;
     void addDimRadial(const DRW_DimRadial *data) override;
     void addDimDiametric(const DRW_DimDiametric *data) override;
     void addDimAngular(const DRW_DimAngular *data) override;
     void addDimAngular3P(const DRW_DimAngular3p *data) override;
     void addDimOrdinate(const DRW_DimOrdinate *data) override;
     void addLeader(const DRW_Leader *data) override;
     void addHatch(const DRW_Hatch* data) override;
     void addViewport(const DRW_Viewport& /*data*/) override{}
     void addImage(const DRW_Image* data) override;
     void linkImage(const DRW_ImageDef* data) override;

     void add3dFace(const DRW_3Dface& data) override;
     void addComment(const char*) override;

     void addPlotSettings(const DRW_PlotSettings* data) override;

    // Export:
     bool fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) override;

     void writeHeader(DRW_Header& data) override;
     void writeEntities() override;
     void writeLTypes() override;
     void writeLayers() override;
     void writeTextstyles() override;
     void writeVports() override;
     void writeBlockRecords() override;
     void writeBlocks() override;
     void writeDimstyles() override;
     void writeObjects() override;
     void writeAppId() override;

    void writePoint(RS_Point* p);
    void writeLine(RS_Line* l);
    void writeCircle(RS_Circle* c);
    void writeArc(RS_Arc* a);
    void writeEllipse(RS_Ellipse* s);
    void writeSolid(RS_Solid* s);
    void writeLWPolyline(RS_Polyline* l);
    void writeSpline(RS_Spline* s);
	void writeSplinePoints(LC_SplinePoints *s);
    void writeInsert(RS_Insert* i);
    void writeMText(RS_MText* t);
    void writeText(RS_Text* t);
    void writeHatch(RS_Hatch* h);
    void writeImage(RS_Image* i);
    void writeLeader(RS_Leader* l);
    void writeDimension(RS_Dimension* d);
    void writePolyline(RS_Polyline* p);

/*	void writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                const DRW_Entity& attrib);
	void writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c, 
                const DRW_Entity& attrib, RS2::ResolveLevel level);*/
	

    void setEntityAttributes(RS_Entity* entity, const DRW_Entity* attrib);
    void getEntityAttributes(DRW_Entity* ent, const RS_Entity* entity);

    static QString toDxfString(const QString& str);
    static QString toNativeString(const QString& data);

public:
    RS_Pen attributesToPen(const DRW_Layer* att) const;

    static RS_Color numberToColor(int num);
    static int colorToNumber(const RS_Color& col, int *rgb);

    static RS2::LineType nameToLineType(const QString& name);
    static QString lineTypeToName(RS2::LineType lineType);
    //static QString lineTypeToDescription(RS2::LineType lineType);

    static RS2::LineWidth numberToWidth(DRW_LW_Conv::lineWidth lw);
    static DRW_LW_Conv::lineWidth widthToNumber(RS2::LineWidth width);

        static RS2::AngleFormat numberToAngleFormat(int num);
        static int angleFormatToNumber(RS2::AngleFormat af);

	static RS2::Unit numberToUnit(int num);
	static int unitToNumber(RS2::Unit unit);
	
        static bool isVariableTwoDimensional(const QString& var);

    static RS_FilterInterface* createFilter(){return new RS_FilterDXFRW();}

private:
    void prepareBlocks();
    void writeEntity(RS_Entity* e);
#ifdef DWGSUPPORT
    void printDwgError(int le);
    QString printDwgVersion(int v);
#endif

private:
    /** Pointer to the graphic we currently operate on. */
    RS_Graphic* graphic;
    /** File name. Used to find out the full path of images. */
    QString file;
    /** Pointer to current entity container (either block or graphic) */
    RS_EntityContainer* currentContainer;
    /** File codePage. Used to find the text coder. */
    QString codePage;
    /** File version. */
    QString versionStr;
    int version;
    /** Library File version. */
#define LIBDXFRW_VERSION(version,release,patch) (((version) << 16) | ((release) << 8) | (patch))
    bool isLibDxfRw {false};
    uint libDxfRwVersion;
    /** dimension style. */
    QString dimStyle;
    /** text style. */
    QString textStyle;
    /** Temporary list to handle unnamed blocks to write R12 dxf. */
    QHash <RS_Entity*, QString> noNameBlock;
    QHash <QString, QString> fontList;
    bool oldMText;
    dxfRW *dxfW;
    /** If saved version are 2004 or above can save color in RGB value. */
    bool exactColor;
    /** hash of block containers and handleBlock numbers to read dwg files */
    QHash<int, RS_EntityContainer*> blockHash;
    /** Pointer to entity container to store possible orphan entities like paper space */
    RS_EntityContainer* dummyContainer;
};

#endif
