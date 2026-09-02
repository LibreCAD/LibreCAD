/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_INTERFACE_H
#define DRW_INTERFACE_H

#include <cstring>

#include "drw_datastorage.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"

class DRW_Class;
class DRW_Attdef;

/**
 * Abstract class (interface) for communicate dxfReader with the application.
 * Inherit your class which takes care of the entities in the
 * processed DXF file from this interface.
 *
 * @author Rallaz
 */
class DRW_Interface {
public:
  DRW_Interface() = default;
  virtual ~DRW_Interface() = default;

  /** Called when header is parsed.  */
  virtual void addHeader(const DRW_Header *data) = 0;

  /** Called for every line Type.  */
  virtual void addLType(const DRW_LType &data) = 0;
  /** Called for every layer. */
  virtual void addLayer(const DRW_Layer &data) = 0;
  /** Called for every dim style. */
  virtual void addDimStyle(const DRW_Dimstyle &data) = 0;
  /** Called for every VPORT table. */
  virtual void addVport(const DRW_Vport &data) = 0;
  /** Called for fixed type 71 VPORT_ENTITY_HEADER records. */
  virtual void addViewportEntityHeader(const DRW_ViewportEntityHeader &data) {
    (void)data;
  }
  /** Called for AcDbVxControl objects. */
  virtual void addVxControl(const DRW_VxControl &data) { (void)data; }
  /** Called for AcDbVxTableRecord objects. */
  virtual void addVxTableRecord(const DRW_VxTableRecord &data) { (void)data; }
  /** Called for AcDbTvDeviceProperties objects. */
  virtual void addTvDeviceProperties(const DRW_TvDeviceProperties &data) {
    (void)data;
  }
  /** Called for CSACDOCUMENTOPTIONS objects. */
  virtual void addCsacDocumentOptions(const DRW_CsacDocumentOptions &data) {
    (void)data;
  }
  /** Called for CONTEXTDATAMANAGER objects. */
  virtual void addContextDataManager(const DRW_ContextDataManager &data) {
    (void)data;
  }
  /** Called for every text style. */
  virtual void addTextStyle(const DRW_Textstyle &data) = 0;
  /** Called for every AppId entry. */
  virtual void addAppId(const DRW_AppId &data) = 0;
  /** Called for every named UCS table entry. */
  virtual void addUCS(const DRW_UCS &data) { (void)data; }
  /** Called for every VIEW table entry. */
  virtual void addView(const DRW_View &data) { (void)data; }
  /** Called for every TOLERANCE entity. */
  virtual void addTolerance(const DRW_Tolerance &data) { (void)data; }
  /** Called for every Dictionary (named-object container, ODA fixed type 42).
   */
  virtual void addDictionary(const DRW_Dictionary &data) { (void)data; }
  /** Called for every Dictionary-with-default object. */
  virtual void addDictionaryWithDefault(const DRW_DictionaryWithDefault &data) {
    (void)data;
  }
  /** Called for every DictionaryVar object. */
  virtual void addDictionaryVar(const DRW_DictionaryVar &data) { (void)data; }
  /** Called for every XRECORD object. */
  virtual void addXRecord(const DRW_XRecord &data) { (void)data; }
  /** Called for every FIELD object. */
  virtual void addField(const DRW_Field &data) { (void)data; }
  /** Called for every FIELDLIST object. */
  virtual void addFieldList(const DRW_FieldList &data) { (void)data; }
  /** Called for every DATATABLE object (hidden key/value table). */
  virtual void addDataTable(const DRW_DataTable &data) { (void)data; }
  /** Called for every dynamic-block object (BLOCK*PARAMETER/ACTION/GRIP etc.).
   */
  virtual void addDynamicBlockObject(const DRW_DynamicBlockObject &data) {
    (void)data;
  }
  /** Called for every RASTERVARIABLES object. */
  virtual void addRasterVariables(const DRW_RasterVariables &data) {
    (void)data;
  }
  /** Called for every WIPEOUTVARIABLES object (global display-frame flag). */
  virtual void addWipeoutVariables(const DRW_WipeoutVariables &data) {
    (void)data;
  }
  /** Called for every SORTENTSTABLE object. */
  virtual void addSortEntsTable(const DRW_SortEntsTable &data) { (void)data; }
  /** Called for every MATERIAL object. */
  virtual void addMaterial(const DRW_Material &data) { (void)data; }
  /** Called for every TABLESTYLE object. */
  virtual void addTableStyle(const DRW_TableStyle &data) { (void)data; }
  /** Called for every standalone TABLECONTENT object. */
  virtual void addTableContent(const DRW_TableContentObject &data) {
    (void)data;
  }
  /** Called for typed standalone *OBJECTCONTEXTDATA metadata shells. */
  virtual void addObjectContextData(const DRW_ObjectContextData &data) {
    (void)data;
  }
  /** Called for every CELLSTYLEMAP object. */
  virtual void addCellStyleMap(const DRW_CellStyleMap &data) { (void)data; }
  /** Called for every Layout (paperspace, ODA fixed type 82). */
  virtual void addLayout(const DRW_Layout &data) { (void)data; }
  /** Called for every MLineStyle (ODA fixed type 73). */
  virtual void addMLineStyle(const DRW_MLineStyle &data) { (void)data; }
  /** Called for unsupported DWG object/entity payloads kept as raw bytes. */
  virtual void addUnsupportedObject(const DRW_UnsupportedObject &data) {
    (void)data;
  }
  /** Called once after a validated DWG frame has reached its public views. */
  virtual void addDwgFramePublication(const DRW_DwgFramePublication &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated fixed DICTIONARY membership. */
  virtual void
  addDwgDictionaryMembership(const DRW_DwgDictionaryMembership &data) {
    (void)data;
  }
  /** Called with a complete AcDbDictionaryWithDefault membership receipt. */
  virtual void addDwgDictionaryWithDefaultMembership(
      const DRW_DwgDictionaryWithDefaultMembership &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated fixed GROUP membership. */
  virtual void addDwgGroupMembership(const DRW_DwgGroupMembership &data) {
    (void)data;
  }
  /** Called with complete parser-validated AcDbSortentsTable rows. */
  virtual void addDwgSortEntsMembership(const DRW_DwgSortEntsMembership &data) {
    (void)data;
  }
  /** Called with complete parser-validated FIELDLIST handle slots. */
  virtual void
  addDwgFieldListMembership(const DRW_DwgFieldListMembership &data) {
    (void)data;
  }
  /** Called with a complete parser-validated AcDbField payload receipt. */
  virtual void
  addDwgFieldPayloadReceipt(const DRW_DwgFieldPayloadReceipt &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated typed object reference. */
  virtual void addDwgTypedReference(const DRW_DwgTypedReference &data) {
    (void)data;
  }
  /** Called with a complete, parser-validated modern BLOCK_RECORD walk. */
  virtual void addDwgBlockReachability(const DRW_DwgBlockReachability &data) {
    (void)data;
  }
  /** Called after a DWG reader finishes its HANDLE-map frame accounting. */
  virtual void
  addDwgFrameCoverageReport(const DRW_DwgFrameCoverageReport &data) {
    (void)data;
  }
  /** Called after a DWG reader finalizes its independent CLASSES report. */
  virtual void
  addDwgClassCoverageReport(const DRW_DwgClassCoverageReport &data) {
    (void)data;
  }
  /** Called for unsupported DWG data sections kept as raw bytes. */
  virtual void addRawDwgSection(const DRW_RawDwgSection &data) { (void)data; }
  /** Called for typed AcDb:AcDsPrototype_1b (DataStorage) index (PR-2a). */
  virtual void addDataStorage(const DRW_DataStorageSection &data) {
    (void)data;
  }
  //! Lossless DXF passthrough (slice A1): an OBJECTS-section object libdxfrw
  //! does not model is delivered verbatim so it can be re-emitted unchanged.
  virtual void addRawDxfObject(const DRW_RawDxfObject &data) { (void)data; }
  //! Lossless DXF passthrough (slice A4): an ENTITIES/BLOCKS entity libdxfrw
  //! does not model is delivered verbatim for unchanged re-emit.
  virtual void addRawDxfEntity(const DRW_RawDxfObject &data) { (void)data; }
  //! Parsed DXF CLASSES-section entry. Default no-op keeps older consumers
  //! source-compatible while filters that preserve raw custom data can retain
  //! exact class metadata for re-emission.
  virtual void addDxfClass(const DRW_Class &data) { (void)data; }
  //! Lossless passthrough for an unmodeled DXF section. Comments follow the
  //! same policy as the rest of section parsing and are not included.
  virtual void addRawDxfSection(const DRW_RawDxfSection &data) { (void)data; }

  /**
   * Called for every block. Note: all entities added after this
   * command go into this block until endBlock() is called.
   *
   * @see endBlock()
   */
  virtual void addBlock(const DRW_Block &data) = 0;

  /**
   * In DWG called when the following entities corresponding to a
   * block different from the current. Note: all entities added after this
   * command go into this block until setBlock() is called already.
   *
   * int handle are the value of DRW_Block::handleBlock added with addBlock()
   */
  virtual void setBlock(int handle) = 0;

  /** Called to end the current block */
  virtual void endBlock() = 0;

  /** Called for every point */
  virtual void addPoint(const DRW_Point &data) = 0;

  /** Called for every line */
  virtual void addLine(const DRW_Line &data) = 0;

  /** Called for modern custom-class 3DLINE entities. The default keeps
   * existing consumers source-compatible and exposes the same projected
   * geometry through their ordinary line path. */
  virtual void add3DLine(const DRW_3DLine &data) { addLine(data); }

  /** Called for every ray */
  virtual void addRay(const DRW_Ray &data) = 0;

  /** Called for every xline */
  virtual void addXline(const DRW_Xline &data) = 0;

  /** Called for every arc */
  virtual void addArc(const DRW_Arc &data) = 0;

  /** Called for every circle */
  virtual void addCircle(const DRW_Circle &data) = 0;

  /** Called for every ellipse */
  virtual void addEllipse(const DRW_Ellipse &data) = 0;

  /** Called for every lwpolyline */
  virtual void addLWPolyline(const DRW_LWPolyline &data) = 0;

  /** Called for every multiline (MLINE) entity. Default no-op so
   *  existing implementers compile unchanged; LibreCAD's filter
   *  decomposes the entity into N parallel polylines.
   */
  virtual void addMLine(const DRW_MLine *data) { (void)data; }

  /** Called for every UNDERLAY entity (PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY).
   *  Default no-op. Note: entities arrive during readDwgEntities, BEFORE
   *  the matching UNDERLAYDEFINITION objects (linkUnderlay) — implementers
   *  must defer filename resolution.
   */
  virtual void addUnderlay(const DRW_Underlay *data) { (void)data; }
  /** Called for every SHAPE entity. Default no-op; LibreCAD stores metadata. */
  virtual void addShape(const DRW_Shape &data) { (void)data; }
  /** Called for every OLE2FRAME entity. Default no-op; payload is opaque. */
  virtual void addOle2Frame(const DRW_Ole2Frame &data) { (void)data; }
  /** Called for every legacy OLEFRAME entity. Default no-op; payload is opaque.
   */
  virtual void addOleFrame(const DRW_OleFrame &data) { (void)data; }
  /** Called for ACAD_PROXY_ENTITY entities. */
  virtual void addProxyEntity(const DRW_ProxyEntity &data) { (void)data; }

  /** Called for every UNDERLAYDEFINITION (Pdf/Dgn/Dwf) object.
   *  Carries the filename + sheet name that UNDERLAY entities reference
   *  via definitionHandle. Default no-op. Mirrors addImage/linkImage.
   */
  virtual void linkUnderlay(const DRW_UnderlayDefinition *data) { (void)data; }

  /** Called for every polyline start */
  virtual void addPolyline(const DRW_Polyline &data) = 0;

  /** Called for every spline */
  virtual void addSpline(const DRW_Spline *data) = 0;

  /** Called for every helix (AcDbHelix). Default delegates nothing; the
   *  base no-op lets readers add HELIX without forcing every consumer to
   *  implement it. */
  virtual void addHelix(const DRW_Helix *data) { (void)data; }

  /** Called for every MESH (AcDbSubDMesh) subdivision-surface entity. Default
   *  no-op so readers can deliver MESH without forcing every consumer to
   *  implement it (trailing-virtual ABI convention, static-link only). */
  virtual void addMesh(const DRW_Mesh &data) { (void)data; }

  /** Called for every spline knot value */
  virtual void addKnot(const DRW_Entity &data) = 0;

  /** Called for every insert. */
  virtual void addInsert(const DRW_Insert &data) = 0;
  /** Called for every ACAD_TABLE entity. Defaults to INSERT rendering. */
  virtual void addTable(const DRW_Table &data) { addInsert(data); }

  /** Called for every trace start */
  virtual void addTrace(const DRW_Trace &data) = 0;

  /** Called for every 3dface start */
  virtual void add3dFace(const DRW_3Dface &data) = 0;

  /** Called for REGION/3DSOLID/BODY opaque modeler geometry shells. */
  virtual void addModelerGeometry(const DRW_ModelerGeometry &data) {
    (void)data;
  }

  /** Called for LIGHT entity metadata. */
  virtual void addLight(const DRW_Light &data) { (void)data; }

  /** Called for CAMERA entity metadata. */
  virtual void addCamera(const DRW_Camera &data) { (void)data; }

  /** Called for GEOPOSITIONMARKER entity metadata. */
  virtual void addGeoPositionMarker(const DRW_GeoPositionMarker &data) {
    (void)data;
  }

  /** Called for SECTIONOBJECT (AcDbSection) live-section plane metadata. */
  virtual void addSectionObject(const DRW_SectionObject &data) { (void)data; }

  /** Called for every solid start */
  virtual void addSolid(const DRW_Solid &data) = 0;

  /** Called for every Multi Text entity. */
  virtual void addMText(const DRW_MText &data) = 0;

  /** Called for every Text entity. */
  virtual void addText(const DRW_Text &data) = 0;

  /** Called for every attribute definition in a BLOCK. The default preserves
   * source compatibility for consumers that do not model attribute metadata. */
  virtual void addAttDef(const DRW_Attdef &data) { (void)data; }

  /**
   * Called for every aligned dimension entity.
   */
  virtual void addDimAlign(const DRW_DimAligned *data) = 0;
  /**
   * Called for every linear or rotated dimension entity.
   */
  virtual void addDimLinear(const DRW_DimLinear *data) = 0;

  /**
   * Called for every radial dimension entity.
   */
  virtual void addDimRadial(const DRW_DimRadial *data) = 0;

  /**
   * Called for every diametric dimension entity.
   */
  virtual void addDimDiametric(const DRW_DimDiametric *data) = 0;

  /**
   * Called for every angular dimension (2 lines version) entity.
   */
  virtual void addDimAngular(const DRW_DimAngular *data) = 0;

  /**
   * Called for every angular dimension (3 points version) entity.
   */
  virtual void addDimAngular3P(const DRW_DimAngular3p *data) = 0;

  /**
   * Called for every ordinate dimension entity.
   */
  virtual void addDimOrdinate(const DRW_DimOrdinate *data) = 0;

  /**
   * Called for every arc length dimension entity (ARC_DIMENSION).
   */
  virtual void addDimArc(const DRW_DimArc *data) = 0;

  /**
   * Called for every leader start.
   */
  virtual void addLeader(const DRW_Leader *data) = 0;

  /**
   * Called for every hatch entity.
   */
  virtual void addHatch(const DRW_Hatch *data) = 0;

  /**
   * Called for every MPOLYGON (AcDbMPolygon) entity.  Default renders it as a
   * hatch (it is hatch-derived: same boundary loops, solid flag and pattern).
   * Override only if the MPOLYGON-specific fill color / outline needs distinct
   * treatment.
   */
  virtual void addMPolygon(const DRW_MPolygon *data) { addHatch(data); }

  /**
   * Called for every viewport entity.
   */
  virtual void addViewport(const DRW_Viewport &data) = 0;

  /**
   * Called for every image entity.
   */
  virtual void addImage(const DRW_Image *data) = 0;

  /**
   * Called for every image definition.
   */
  virtual void linkImage(const DRW_ImageDef *data) = 0;

  /**
   * Called for every WIPEOUT entity.  WIPEOUT shares the binary layout of
   * IMAGE (subclass AcDbWipeout); the meaningful payload is the polygon
   * stored in DRW_Wipeout::clipPath.  Default no-op so existing implementers
   * compile unchanged; override to consume.
   */
  virtual void addWipeout(const DRW_Wipeout *data) { (void)data; }

  /**
   * Called for every POINTCLOUD entity. Default no-op; override to consume.
   */
  virtual void addPointCloud(const DRW_PointCloud *data) { (void)data; }

  /**
   * Called for every POINTCLOUDEX entity. Default no-op; override to consume.
   */
  virtual void addPointCloudEx(const DRW_PointCloudEx *data) { (void)data; }

  /** Called for every NAVISWORKSMODEL / COORDINATION_MODEL entity. */
  virtual void addNavisworksModel(const DRW_NavisworksModel *data) {
    (void)data;
  }

  /**
   * Called for every surface entity (PLANESURFACE, EXTRUDEDSURFACE, etc.).
   * Default no-op; override to consume.
   */
  virtual void addSurface(const DRW_Surface *data) { (void)data; }

  /**
   * Called for every MULTILEADER (MLEADER) entity.  AcDbMLeader subclass,
   * a custom-class object in DWG (oType >= 500 with classesmap recName
   * "MULTILEADER").  Default no-op for back-compat.
   */
  virtual void addMLeader(const DRW_MLeader *data) { (void)data; }

  /**
   * Called for every MLEADERSTYLE dictionary object (AcDbMLeaderStyle).
   * Lives in the OBJECTS section, referenced by MLEADER entities via
   * their style handle.  ODA spec §20.4.87.  Default no-op for back-compat.
   */
  virtual void addMLeaderStyle(const DRW_MLeaderStyle *data) { (void)data; }

  /**
   * Called for every DBCOLOR (AcDbColor) object encountered in the OBJECTS
   * section.  ODA spec §20.4.  Default no-op for back-compat — most
   * implementers don't need a per-object hook because the reader patches
   * referencing entities' color24/colorName via dbColorMap before
   * delivering them.  This hook is for filters that want to track the
   * full set of named colors (e.g., to emit a layer book-color map).
   */
  virtual void addDbColor(const DRW_DbColor &data) { (void)data; }

  /**
   * Called for every VISUALSTYLE (AcDbVisualStyle) object encountered in
   * the OBJECTS section. ODA spec §20.4.95. Default no-op — LibreCAD has
   * no 3D rendering and treats these as opaque round-trip metadata.
   */
  virtual void addVisualStyle(const DRW_VisualStyle &data) { (void)data; }

  /**
   * Called for every SCALE (AcDbScale) annotation-scale entry encountered
   * in the OBJECTS section.  Lives under ACAD_SCALELIST in the named-object
   * dictionary.  ODA §20.4.93 / libreDWG dwg2.spec:1195.  Default no-op;
   * the foundation for per-viewport-scale resolution of MLEADER/DIMENSION/
   * MTEXT/ATTRIB at render time.  scaleFactor() returns the design factor
   * (drawingUnits / paperUnits, e.g. "1:48" → 48).
   */
  virtual void addScale(const DRW_Scale &data) { (void)data; }
  /** Called for every DIMASSOC associative-dimension metadata object. */
  virtual void addDimensionAssociation(const DRW_DimensionAssociation &data) {
    (void)data;
  }
  /** Called for every ACAD_EVALUATION_GRAPH dynamic/associative metadata
   * object. */
  virtual void addEvaluationGraph(const DRW_EvaluationGraph &data) {
    (void)data;
  }
  /** Called for every fixed BLOCKREPRESENTATIONDATA object. */
  virtual void
  addBlockRepresentationData(const DRW_BlockRepresentationData &data) {
    (void)data;
  }
  /** Called for every DETAILVIEWSTYLE model-document view style object. */
  virtual void addDetailViewStyle(const DRW_DetailViewStyle &data) {
    (void)data;
  }
  /** Called for every SECTIONVIEWSTYLE model-document view style object. */
  virtual void addSectionViewStyle(const DRW_SectionViewStyle &data) {
    (void)data;
  }
  /** Called for every BREAKDATA broken-view/dimension-break metadata object. */
  virtual void addBreakData(const DRW_BreakData &data) { (void)data; }
  /** Called for every BREAKPOINTREF broken-view reference metadata object. */
  virtual void addBreakPointRef(const DRW_BreakPointRef &data) { (void)data; }
  /** Called for every GROUP object. */
  virtual void addGroup(const DRW_Group &data) { (void)data; }
  /** Called for every IMAGEDEF_REACTOR object. */
  virtual void
  addImageDefinitionReactor(const DRW_ImageDefinitionReactor &data) {
    (void)data;
  }
  /** Called for every SPATIAL_FILTER xref clipping object. */
  virtual void addSpatialFilter(const DRW_SpatialFilter &data) { (void)data; }
  /** Called for every GEODATA geolocation metadata object. */
  virtual void addGeoData(const DRW_GeoData &data) { (void)data; }
  /** Called for every TABLEGEOMETRY cache object. */
  virtual void addTableGeometry(const DRW_TableGeometry &data) { (void)data; }
  /** Called for ACDBPLACEHOLDER fixed objects. */
  virtual void addAcDbPlaceholder(const DRW_AcDbPlaceholder &data) {
    (void)data;
  }
  /** Called for VBA_PROJECT fixed objects. */
  virtual void addVbaProject(const DRW_VbaProject &data) { (void)data; }
  /** Called for ACAD_PROXY_OBJECT fixed objects. */
  virtual void addProxyObject(const DRW_ProxyObject &data) { (void)data; }
  /** Called for SUN view/vport lighting objects. */
  virtual void addSun(const DRW_Sun &data) { (void)data; }
  /** Called for LIGHTLIST objects containing named LIGHT references. */
  virtual void addLightList(const DRW_LightList &data) { (void)data; }
  /** Called for DATALINK objects containing external-data metadata. */
  virtual void addDataLink(const DRW_DataLink &data) { (void)data; }
  /** Called for GEOMAPIMAGE LiveMap raster-image objects. */
  virtual void addGeoMapImage(const DRW_GeoMapImage &data) { (void)data; }
  /** Called for every render background (AcDb*Background) object. */
  virtual void addBackground(const DRW_Background &data) { (void)data; }
  /** Called for every point-cloud definition (AcDbPointCloudDef*) object. */
  virtual void addPointCloudDef(const DRW_PointCloudDef &data) { (void)data; }
  /** Called for every Navisworks external-model definition object. */
  virtual void addNavisworksModelDef(const DRW_NavisworksModelDef &data) {
    (void)data;
  }
  /** Called for every point-cloud color-map object. */
  virtual void addPointCloudColorMap(const DRW_PointCloudColorMap &data) {
    (void)data;
  }
  /** Called for every SUNSTUDY (AcDbSunStudy) object. */
  virtual void addSunStudy(const DRW_SunStudy &data) { (void)data; }
  /** Called for LAYERFILTER objects containing layer names. */
  virtual void addLayerFilter(const DRW_LayerFilter &data) { (void)data; }
  /** Called for every MOTIONPATH (AcDbMotionPath) object. */
  virtual void addMotionPath(const DRW_MotionPath &data) { (void)data; }
  /** Called for every CURVEPATH (AcDbCurvePath) object. */
  virtual void addCurvePath(const DRW_CurvePath &data) { (void)data; }
  /** Called for every POINTPATH (AcDbPointPath) object. */
  virtual void addPointPath(const DRW_PointPath &data) { (void)data; }
  /** Called for every OBJECT_PTR (AcDbObjectPtr) object. */
  virtual void addObjectPtr(const DRW_ObjectPtr &data) { (void)data; }
  /** Called for every PARTIAL_VIEWING_INDEX object. */
  virtual void addPartialViewingIndex(const DRW_PartialViewingIndex &data) {
    (void)data;
  }
  /** Called for every render-settings (AcDbRenderSettings family) object. */
  virtual void addRenderSettings(const DRW_RenderSettings &data) { (void)data; }
  /** Called for every SECTION_MANAGER / SECTION_SETTINGS object. */
  virtual void addSection(const DRW_Section &data) { (void)data; }
  /** Called for ACDBASSOC* action/dependency/action-param shell objects. */
  virtual void addAssociativeObject(const DRW_AssociativeObject &data) {
    (void)data;
  }
  /** Called for ACSH_* history/action shell objects. */
  virtual void addAcShHistoryObject(const DRW_AcShHistoryObject &data) {
    (void)data;
  }
  /** Called for IDBUFFER objects (ODA §20.4.79). */
  virtual void addIDBuffer(const DRW_IDBuffer &data) { (void)data; }
  /** Called for generic INDEX objects (AcDbIndex). */
  virtual void addIndex(const DRW_Index &data) { (void)data; }
  /** Called for LAYER_INDEX objects (ODA §20.4.83). */
  virtual void addLayerIndex(const DRW_LayerIndex &data) { (void)data; }
  /** Called for SPATIAL_INDEX objects (ODA §20.4.95). */
  virtual void addSpatialIndex(const DRW_SpatialIndex &data) { (void)data; }

  /**
   * Called for every comment in the DXF file (code 999).
   */
  virtual void addComment(const char *comment) = 0;

  /**
   * Called for PLOTSETTINGS object definition.
   */
  virtual void addPlotSettings(const DRW_PlotSettings *data) = 0;

  virtual void writeHeader(DRW_Header &data) = 0;
  /** DWG-only pre-CLASSES callback. Implementations may register imported
   * custom classes or raw replay payloads before the binary CLASSES section
   * is emitted. DXF writers ignore this hook. */
  virtual void writeDwgClasses() {}
  /** DWG-only pre-table callback. Implementations may register APPIDs
   * referenced by entity or typed-object EED/XDATA before the APPID table is
   * emitted. DXF writers ignore this hook. */
  virtual void collectDwgAppIds() {}
  virtual void writeBlocks() = 0;
  virtual void writeBlockRecords() = 0;
  virtual void writeEntities() = 0;
  virtual void writeLTypes() = 0;
  virtual void writeLayers() = 0;
  virtual void writeTextstyles() = 0;
  virtual void writeVports() = 0;
  virtual void writeDimstyles() = 0;
  virtual void writeObjects() = 0;
  /**
   * Called after DWG content callbacks and before deferred structural
   * objects are emitted. Returning false aborts the DWG transaction.
   */
  virtual bool finalizeDwgWrite() { return true; }
  /** Called after deferred BLOCK_CONTROL and BLOCK_RECORD objects have
   * been emitted. Returning false aborts the DWG transaction. */
  virtual bool finalizeDwgWriteStructure() { return true; }
  virtual void writeAppId() = 0;
  /** Called when the writer wants the implementation to emit named VIEW
   * records. Default-empty for ABI compatibility with consumers (LibreCAD_3,
   * dx_iface) that don't carry View data. */
  virtual void writeViews() {}
  /** Called when the writer wants the implementation to emit named UCS
   * records. Default-empty (same rationale as writeViews). */
  virtual void writeUCSs() {}
};

#endif
