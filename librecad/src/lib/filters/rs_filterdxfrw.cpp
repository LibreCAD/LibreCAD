/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include<cstdlib>
#include <stack>
#include<utility>

#include <QRegularExpression>
#include <QStringList>
#include <QStringConverter>
#include <QFile>
#include <QFileInfo>

#include "rs_filterdxfrw.h"
#include "lc_containertraverser.h"
#include "lc_parabola.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_system.h"
#include "rs_text.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_math.h"
#include "dxf_format.h"
#include "lc_defaults.h"
#include "lc_dimarrowregistry.h"
#include "lc_dimordinate.h"
#include "lc_dimstyle.h"
#include "lc_extentitydata.h"
#include "lc_tolerance.h"

#ifdef DWGSUPPORT
#include "lc_graphicviewport.h"
#include "lc_ucs.h"
#include "lc_view.h"
#include "libdwgr.h"
#include "rs_debug.h"
#endif // DWGSUPPORT

/**
 * Default constructor.
 *
 */
RS_FilterDXFRW::RS_FilterDXFRW()
    :RS_FilterInterface(),DRW_Interface() {
    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW()");

    m_currentContainer = nullptr;
    m_graphic = nullptr;
// Init hash to change the QCAD "normal" style to the more correct ISO-3059
// or draftsight symbol (AR*.shx) to sy*.lff
    m_fontList["arastro"] = "syastro";
    m_fontList["armap"] = "symap";
    m_fontList["armeteo"] = "symeteo";
    m_fontList["armusic"] = "symusic";
    m_fontList["math"] = "symath";
    m_fontList["normal"] = "iso";
    m_fontList["normallatin1"] = "iso";
    m_fontList["normallatin2"] = "iso";

    RS_DEBUG->print("RS_FilterDXFRW::RS_FilterDXFRW(): OK");
}

/**
 * Destructor.
 */
RS_FilterDXFRW::~RS_FilterDXFRW() {
    RS_DEBUG->print("RS_FilterDXFRW::~RS_FilterDXFRW(): OK");
}

QString RS_FilterDXFRW::lastError() const{
    switch (errorCode) {
    case DRW::BAD_NONE:
        return (QObject::tr( "no DXF/DWG error", "RS_FilterDXFRW"));
    case DRW::BAD_OPEN:
        return (QObject::tr( "error opening DXF/DWG file", "RS_FilterDXFRW"));
    case DRW::BAD_VERSION:
        return (QObject::tr( "unsupported DXF/DWG file version", "RS_FilterDXFRW"));
    case DRW::BAD_READ_METADATA:
        return (QObject::tr( "error reading DXF/DWG meta data", "RS_FilterDXFRW"));
    case DRW::BAD_READ_FILE_HEADER:
        return (QObject::tr( "error reading DXF/DWG file header", "RS_FilterDXFRW"));
    case DRW::BAD_READ_HEADER:
        return (QObject::tr( "error reading DXF/DWG header dara", "RS_FilterDXFRW"));
    case DRW::BAD_READ_HANDLES:
        return (QObject::tr( "error reading DXF/DWG object map", "RS_FilterDXFRW"));
    case DRW::BAD_READ_CLASSES:
        return (QObject::tr( "error reading DXF/DWG classes", "RS_FilterDXFRW"));
    case DRW::BAD_READ_TABLES:
        return (QObject::tr( "error reading DXF/DWG tables", "RS_FilterDXFRW"));
    case DRW::BAD_READ_BLOCKS:
        return (QObject::tr( "error reading DXF/DWG blocks", "RS_FilterDXFRW"));
    case DRW::BAD_READ_ENTITIES:
        return (QObject::tr( "error reading DXF/DWG entities", "RS_FilterDXFRW"));
    case DRW::BAD_READ_OBJECTS:
        return (QObject::tr( "error reading DXF/DWG objects", "RS_FilterDXFRW"));
    case DRW::BAD_READ_SECTION:
        return (QObject::tr( "error reading DXF/DWG sections", "RS_FilterDXFRW"));
    case DRW::BAD_CODE_PARSED:
        return (QObject::tr( "error reading DXF/DWG code", "RS_FilterDXFRW"));
    default:
        break;
    }

    return RS_FilterInterface::lastError();
}

/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The m_graphic in which the entities from the m_file
 * will be created or the graphics from which the entities are
 * taken to be stored in a m_file.
 */
bool RS_FilterDXFRW::fileImport(RS_Graphic& g, const QString& file, [[maybe_unused]] RS2::FormatType type) {
    RS_DEBUG->print("RS_FilterDXFRW::fileImport");
    RS_DEBUG->print("DXFRW Filter: importing file '%s'...", (const char*)QFile::encodeName(file));

    m_graphic = &g;
    m_currentContainer = m_graphic;
    m_dummyContainer = new RS_EntityContainer(nullptr, true);

    this->m_file = file;
    // add some variables that need to be there for DXF drawings:
    m_graphic->addVariable("$DIMSTYLE", "Standard", 2);
    m_dimStyle = "Standard";
    m_codePage = "ANSI_1252";
    m_textStyle = "Standard";
    //reset library version
    m_isLibDxfRw = false;
    m_libDxfRwVersion = 0;

#ifdef DWGSUPPORT
    if (type == RS2::FormatDWG) {
        dwgR dwgr(QFile::encodeName(file));
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading DWG file");
        if (RS_DEBUG->getLevel()== RS_Debug::D_DEBUGGING)
            dwgr.setDebug(DRW::DebugLevel::Debug);
        bool success = dwgr.read(this, true);
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading DWG file: OK");
        RS_DIALOGFACTORY->commandMessage(QObject::tr("Opened dwg file version %1.").arg(printDwgVersion(dwgr.getVersion())));
        int  lastError = dwgr.getError();
        if (false == success) {
            printDwgError(lastError);
            RS_DEBUG->print(RS_Debug::D_WARNING,"Cannot open DWG file '%s'.", (const char*)QFile::encodeName(file));
            errorCode = dwgr.getError();
            return false;
        }
    } else {
#endif

        m_dxfR = new dxfRW(QFile::encodeName(file));

        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file");
        if (RS_Debug::D_DEBUGGING == RS_DEBUG->getLevel()) {
            m_dxfR->setDebug(DRW::DebugLevel::Debug);
        }
        bool success {false};
        if (file.startsWith(":")) { // load content from resources. It SHOULD be present in resource!
            QFile resourceFile(file);
            if (resourceFile.open(QIODevice::ReadOnly)) {
                QByteArray contentString = resourceFile.readAll();
                resourceFile.close();
                std::string content = contentString.toStdString();
                success = m_dxfR->readAscii(this, true, content);
            }
        }
        else {
            success = m_dxfR->read(this, true);
        }
        RS_DEBUG->print("RS_FilterDXFRW::fileImport: reading file: OK");
        //graphic->setAutoUpdateBorders(true);

        if (false == success) {
            RS_DEBUG->print(RS_Debug::D_WARNING,"Cannot open DXF file '%s'.", (const char*)QFile::encodeName(file));
            errorCode = m_dxfR->getError();
            delete m_dxfR;
            return false;
        }
        else {
            delete m_dxfR;
        }
#ifdef DWGSUPPORT
    }
#endif

    delete m_dummyContainer;
    /*set current layer */
    auto cl = m_graphic->findLayer(m_graphic->getVariableString("$CLAYER", "0"));
	if (cl ){
        //require to notify
        m_graphic->getLayerList()->activate(cl, true);
    }
    RS_DEBUG->print("RS_FilterDXFRW::fileImport: updating inserts");
    m_graphic->updateInserts();

    RS_DEBUG->print("RS_FilterDXFRW::fileImport OK");
    return true;
}

/**
 * Implementation of the method which handles layers.
 */
void RS_FilterDXFRW::addLayer(const DRW_Layer &data) {
    RS_DEBUG->print("RS_FilterDXF::addLayer");
    RS_DEBUG->print("  adding layer: %s", data.name.c_str());

    RS_DEBUG->print("RS_FilterDXF::addLayer: creating layer");

    QString name = QString::fromUtf8(data.name.c_str());
    if (name != "0" && m_graphic->findLayer(name)) {
        return;
    }
    auto* layer = new RS_Layer(name);
    RS_DEBUG->print("RS_FilterDXF::addLayer: set pen");
    layer->setPen(attributesToPen(&data));

    RS_DEBUG->print("RS_FilterDXF::addLayer: flags");
    if (data.flags&0x01) {
        layer->freeze(true);
    }
    if (data.flags&0x04) {
        layer->lock(true);
    }
    layer->setPrint(data.plotF);

    //parse extended data to read construction flag
    if (!data.extData.empty()){
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::addLayer: layer %s have extended data", layer->getName().toStdString().c_str());
        bool isLCdata = false;
        for (std::vector<DRW_Variant*>::const_iterator it=data.extData.begin(); it!=data.extData.end(); ++it){
            if ((*it)->code() == 1001){
                if (*(*it)->content.s == std::string("LibreCad")) {
                    isLCdata = true;
                }
                else {
                    isLCdata = false;
                }
            } else if (isLCdata && (*it)->code() == 1070){
                if ((*it)->content.i == 1){
                    layer->setConstruction(true);
                }
            }
        }
    }
    //pre dxfrw 0.5.13 plot flag are used to store construction layer
    if( m_isLibDxfRw && m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 5, 13)) {
        layer->setConstruction(! data.plotF);
    }

    if (layer->isConstruction()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::addLayer: layer %s is construction layer", layer->getName().toStdString().c_str());
    }

    RS_DEBUG->print("RS_FilterDXF::addLayer: add layer to graphic");
    m_graphic->addLayer(layer);
    RS_DEBUG->print("RS_FilterDXF::addLayer: OK");
}

/**
 * Implementation of the method which handles dimension styles.
 */
void RS_FilterDXFRW::addDimStyle(const DRW_Dimstyle& data){
    RS_DEBUG->print("RS_FilterDXFRW::addLayer");
    QString dimStyleName = m_graphic->getVariableString("$DIMSTYLE", "standard");

    if (QString::compare(data.name.c_str(), dimStyleName, Qt::CaseInsensitive) == 0) {
        if( m_isLibDxfRw && m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 6, 2)) {
            m_graphic->addVariable("$DIMDEC", m_graphic->getVariableInt("$DIMDEC",
                                            m_graphic->getVariableInt("$LUPREC", 4)), 70);
            m_graphic->addVariable("$DIMADEC", m_graphic->getVariableInt("$DIMADEC",
                                             m_graphic->getVariableInt("$AUPREC", 2)), 70);
            //do nothing;
        } else {
            m_graphic->addVariable("$DIMDEC", data.dimdec, 70);
            m_graphic->addVariable("$DIMADEC", data.dimadec, 70);
        }
    }

    LC_DimStyle* dimStyle = createDimStyle(data);
    m_graphic->addDimStyle(dimStyle);
}

// todo - sand - ucs - rework to direct setup of LC_GraphicViewport instead of RS_GraphicView
// But this modification requires cleanup of the overall file open flow, so leave it as it is for now
/**
 * Implementation of the method which handles vports.
 */
void RS_FilterDXFRW::addVport(const DRW_Vport &data) {
    QString name = QString::fromStdString(data.name);
    if (name.toLower() == "*active") {
        data.grid == 1? m_graphic->setGridOn(true):m_graphic->setGridOn(false);
        m_graphic->setIsometricGrid(data.snapStyle);
        m_graphic->setIsoView( (RS2::IsoGridViewType)data.snapIsopair);
        RS_GraphicView *gv = m_graphic->getGraphicView();  // fixme - sand - review this dependency
        if (gv ) {
            double width = data.height * data.ratio;
            // todo - sand - ucs - investigate support/usage of different x and y factors.
            double factorX= gv->getWidth() / width;
            double factorY= gv->getHeight() / data.height;
            if (factorX > factorY) {
                factorX = factorY;
            }
            int ox = gv->getWidth() - data.center.x*2*factorX;
            int oy = gv->getHeight() - data.center.y*2*factorX;
            gv->getViewPort()->justSetOffsetAndFactor(ox, oy, factorX);
        }
    }
}

void RS_FilterDXFRW::addUCS(const DRW_UCS &data) {
    RS_DEBUG->print("RS_FilterDXF::addUCS");
    RS_DEBUG->print("  adding ucs: %s", data.name.c_str());
    RS_DEBUG->print("RS_FilterDXF::addUCS: creating ucs");

    QString name = QString::fromUtf8(data.name.c_str());
    if (!name.isEmpty() && m_graphic->findNamedUCS(name) != nullptr) {
        return;
    }

    auto* ucs = new LC_UCS(name);
    auto origin = RS_Vector(data.origin.x, data.origin.y, data.origin.z);
    ucs->setOrigin(origin);

    ucs->setElevation(data.elevation);
    ucs->setOrthoType(data.orthoType);

    auto orthoOrigin = RS_Vector(data.orthoOrigin.x, data.orthoOrigin.y, data.orthoOrigin.z);
    ucs->setOrthoOrigin(orthoOrigin);

    auto xAxis = RS_Vector(data.xAxisDirection.x, data.xAxisDirection.y, data.xAxisDirection.z);
    ucs->setXAxis(xAxis);

    auto yAxis = RS_Vector(data.yAxisDirection.x, data.yAxisDirection.y, data.yAxisDirection.z);
    ucs->setYAxis(yAxis);

    RS_DEBUG->print("RS_FilterDXF::addUCS: add ucs to graphic");
    m_graphic->addUCS(ucs);
    RS_DEBUG->print("RS_FilterDXF::addUCS: OK");
}

void RS_FilterDXFRW::addView(const DRW_View &data) {
    RS_DEBUG->print("RS_FilterDXF::addView");
    RS_DEBUG->print("  adding view: %s", data.name.c_str());
    RS_DEBUG->print("RS_FilterDXF::addView: creating view");

    QString name = QString::fromUtf8(data.name.c_str());
    if (!name.isEmpty() && m_graphic->findNamedView(name) != nullptr) {
        return;
    }
    auto* view = new LC_View(name);
    auto center = RS_Vector(data.center.x, data.center.y, data.center.z);
    view->setCenter(center);

    auto size = RS_Vector(data.size.x, data.size.y, data.size.z);
    view->setSize(size);

    auto targetPoint = RS_Vector(data.targetPoint.x, data.targetPoint.y, data.targetPoint.z);
    view->setTargetPoint(targetPoint);

    auto viewDirection = RS_Vector(data.viewDirectionFromTarget.x, data.viewDirectionFromTarget.y, data.viewDirectionFromTarget.z);
    view->setViewDirection(viewDirection);

    view->setLensLen(data.lensLen);
    view->setCameraPlottable(data.cameraPlottable);

    view->setRenderMode(data.renderMode);
    view->setBackClippingPlaneOffset(data.backClippingPlaneOffset);
    view->setFrontClippingPlaneOffset(data.frontClippingPlaneOffset);
    view->setTwistAngle(data.twistAngle);
    view->setFlags(data.flags); // todo - review, use differ properties?
    view->setViewMode(data.viewMode); // todo - probably it might be simpler than long?

    if (data.hasUCS){
        auto ucs = new LC_UCS();

        auto ucsOrigin = RS_Vector(data.ucsOrigin.x, data.ucsOrigin.y, data.ucsOrigin.z);
        auto ucsXAxis = RS_Vector(data.ucsXAxis.x, data.ucsXAxis.y, data.ucsXAxis.z);
        auto ucsYAxis = RS_Vector(data.ucsYAxis.x, data.ucsYAxis.y, data.ucsYAxis.z);
        ucs->setOrthoOrigin(RS_Vector(false));
        ucs->setOrigin(ucsOrigin);
        ucs->setXAxis(ucsXAxis);
        ucs->setYAxis(ucsYAxis);
        ucs->setElevation(data.ucsElevation);
        ucs->setOrthoType(data.ucsOrthoType);
        view->setUCS(ucs);
    }

    RS_DEBUG->print("RS_FilterDXF::addView: set pen");

    RS_DEBUG->print("RS_FilterDXF::addView: add view to graphic");
    m_graphic->addNamedView(view);
    RS_DEBUG->print("RS_FilterDXF::addView: OK");
}

/**
 * Implementation of the method which handles blocks.
 *
 * @todo Adding blocks to blocks (stack for m_currentContainer)
 */
void RS_FilterDXFRW::addBlock(const DRW_Block& data) {
    RS_DEBUG->print("RS_FilterDXF::addBlock");
    RS_DEBUG->print("  adding block: %s", data.name.c_str());
/*TODO correct handle of model-space*/

    QString name = QString::fromUtf8(data.name.c_str());
    QString mid = name.mid(1,11);
// Prevent special blocks (paper_space, model_space) from being added:
    if (mid.toLower() != "paper_space" && mid.toLower() != "model_space") {
            RS_Vector bp(data.basePoint.x, data.basePoint.y);
            auto block = new RS_Block(m_graphic, RS_BlockData(name, bp, false ));
            //block->setFlags(flags);

            if (m_graphic->addBlock(block)) {
                m_currentContainer = block;
                m_blockHash.insert(data.parentHandle, m_currentContainer);
            } else
                m_blockHash.insert(data.parentHandle, m_dummyContainer);
    } else {
        if (mid.toLower() == "model_space") {
            m_blockHash.insert(data.parentHandle, m_graphic);
        } else {
            m_blockHash.insert(data.parentHandle, m_dummyContainer);
        }
    }
}

void RS_FilterDXFRW::setBlock(const int handle){
    if (m_blockHash.contains(handle)) {
        m_currentContainer = m_blockHash.value(handle);
    } else {
        m_currentContainer = m_graphic;
    }
}

/**
 * Implementation of the method which closes blocks.
 */
void RS_FilterDXFRW::endBlock() {
    if (m_currentContainer->rtti() == RS2::EntityBlock) {
        auto bk = static_cast<RS_Block*>(m_currentContainer);
        //remove unnamed blocks *D only if version != R12
        if (m_version!=1009) {
            if (bk->getName().startsWith("*D") ) {
                m_graphic->removeBlock(bk);
            }
        }
    }
    m_currentContainer = m_graphic;
}

/**
 * Implementation of the method which handles point entities.
 */
void RS_FilterDXFRW::addPoint(const DRW_Point& data) {
    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_Point* entity = new RS_Point(m_currentContainer,RS_PointData(v));
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addLine(const DRW_Line& data) {
    RS_DEBUG->print("RS_FilterDXF::addLine");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addLine: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addLine: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addLine: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addLine: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
	}

    RS_DEBUG->print("RS_FilterDXF::addLine: OK");
}

/**
 * Implementation of the method which handles ray entities.
 */
void RS_FilterDXFRW::addRay(const DRW_Ray& data) {
    RS_DEBUG->print("RS_FilterDXF::addRay");

	RS_Vector v1{data.basePoint.x, data.basePoint.y};
	RS_Vector v2{data.basePoint.x+data.secPoint.x,
				data.basePoint.y+data.secPoint.y};

    RS_DEBUG->print("RS_FilterDXF::addRay: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addRay: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addRay: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addRay: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
    }

    RS_DEBUG->print("RS_FilterDXF::addRay: OK");
}

/**
 * Implementation of the method which handles line entities.
 */
void RS_FilterDXFRW::addXline(const DRW_Xline& data) {
    RS_DEBUG->print("RS_FilterDXF::addXline");

    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.basePoint.x+data.secPoint.x, data.basePoint.y+data.secPoint.y);

    RS_DEBUG->print("RS_FilterDXF::addXline: create line");

    if (!m_currentContainer) {
		RS_DEBUG->print("RS_FilterDXF::addXline: currentContainer is nullptr");
    }

    auto entity = new RS_Line{m_currentContainer, {v1, v2}};
    RS_DEBUG->print("RS_FilterDXF::addXline: set attributes");
    setEntityAttributes(entity, &data);

    RS_DEBUG->print("RS_FilterDXF::addXline: add entity");

    if (m_currentContainer) {
        m_currentContainer->addEntity(entity);
	}

    RS_DEBUG->print("RS_FilterDXF::addXline: OK");
}

/**
 * Implementation of the method which handles circle entities.
 */
void RS_FilterDXFRW::addCircle(const DRW_Circle& data) {
    RS_DEBUG->print("RS_FilterDXF::addCircle");

	RS_Vector v{data.basePoint.x, data.basePoint.y};
    auto entity = new RS_Circle(m_currentContainer, {v, data.radious});
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles arc entities.
 *
 * @param angle1 Start angle in deg (!)
 * @param angle2 End angle in deg (!)
 */
void RS_FilterDXFRW::addArc(const DRW_Arc& data) {
    RS_DEBUG->print("RS_FilterDXF::addArc");
    RS_Vector v(data.basePoint.x, data.basePoint.y);
    RS_ArcData d(v, data.radious,
                 data.staangle,
                 data.endangle,
                 false);
    RS_Arc* entity = new RS_Arc(m_currentContainer, d);
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles ellipse entities.
 *
 * @param angle1 Start angle in rad (!)
 * @param angle2 End angle in rad (!)
 */
void RS_FilterDXFRW::addEllipse(const DRW_Ellipse& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addEllipse");

	RS_Vector v1(data.basePoint.x, data.basePoint.y);
	RS_Vector v2(data.secPoint.x, data.secPoint.y);
	double ang2 = data.endparam;
	if (fabs(ang2 - 2.*M_PI) < RS_TOLERANCE && fabs(data.staparam) < RS_TOLERANCE) {
	    ang2 = 0.;
	}
    auto entity = new RS_Ellipse{m_currentContainer, {v1, v2,data.ratio,
										data.staparam, ang2, false}};
    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles trace entities.
 */
void RS_FilterDXFRW::addTrace(const DRW_Trace& data) {
    RS_Solid* entity;
	RS_Vector v1{data.basePoint.x, data.basePoint.y};
	RS_Vector v2{data.secPoint.x, data.secPoint.y};
	RS_Vector v3{data.thirdPoint.x, data.thirdPoint.y};
	RS_Vector v4{data.fourPoint.x, data.fourPoint.y};
    if (v3 == v4) {
        entity = new RS_Solid(m_currentContainer, RS_SolidData(v1, v2, v3));
    }
    else {
        entity = new RS_Solid(m_currentContainer, RS_SolidData(v1, v2, v3,v4));
    }

    setEntityAttributes(entity, &data);
    m_currentContainer->addEntity(entity);
}

void RS_FilterDXFRW::addTolerance(const DRW_Tolerance& data) {
    RS_Vector insertionPoint{data.insertionPoint.x, data.insertionPoint.y};
    RS_Vector axisDirectionVector{data.xAxisDirectionVector.x, data.xAxisDirectionVector.y};

    QString text = toNativeString(QString::fromUtf8(data.text.c_str()));

    QString sty = QString::fromUtf8(data.dimStyleName.c_str());
    if (sty.isEmpty()) {
        sty = m_dimStyle;
    }

    LC_ToleranceData tolData = LC_ToleranceData(insertionPoint, axisDirectionVector,
                                               text, sty);

    auto entity = new LC_Tolerance{m_currentContainer, tolData};
    setEntityAttributes(entity, &data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles solid entities.
 */
void RS_FilterDXFRW::addSolid(const DRW_Solid& data) {
    addTrace(data);
}

/**
 * Implementation of the method which handles lightweight polyline entities.
 */
void RS_FilterDXFRW::addLWPolyline(const DRW_LWPolyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addLWPolyline");
    if (data.vertlist.empty()) {
        return;
    }
    RS_PolylineData d(RS_Vector{},
                      RS_Vector{},
                      data.flags&0x1);
    auto polyline = new RS_Polyline(m_currentContainer, d);
    setEntityAttributes(polyline, &data);

    std::vector<std::pair<RS_Vector, double> > verList;
    for (auto const& v: data.vertlist) {
        verList.emplace_back(std::make_pair(RS_Vector{v->x, v->y}, v->bulge));
    }

    polyline->appendVertexs(verList);
    m_currentContainer->addEntity(polyline);
}

/**
 * Implementation of the method which handles polyline entities.
 */
void RS_FilterDXFRW::addPolyline(const DRW_Polyline& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addPolyline");
    if (data.flags & 0x10) {
        // the polyline is a polygon mesh
        int M = data.vertexcount;
        int N = data.facecount;
        if (M <= 0 || N <= 0 || data.vertlist.size() != static_cast<size_t>(M * N)) {
            return; // invalid mesh
        }
        if (data.curvetype != 0) {
            return; // smooth surfaces not handled
        }
        bool closedM = (data.flags & 0x1);  // closed in M direction
        bool closedN = (data.flags & 0x20); // closed in N direction

        // Add row polylines (along N direction)
        for (int i = 0; i < M; i++) {
            RS_PolylineData pd(RS_Vector(), RS_Vector(), closedN);
            auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
            setEntityAttributes(pl.get(), &data);
            for (int j = 0; j < N; j++) {
                auto v = data.vertlist.at(i * N + j);
                RS_Vector pos(v->basePoint.x, v->basePoint.y);
                pl->addVertex(pos, 0.0, false);
            }
            m_currentContainer->addEntity(pl.release());
        }

        // Add column polylines (along M direction)
        for (int j = 0; j < N; j++) {
            RS_PolylineData pd(RS_Vector(), RS_Vector(), closedM);
            auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
            setEntityAttributes(pl.get(), &data);
            for (int i = 0; i < M; i++) {
                auto v = data.vertlist.at(i * N + j);
                RS_Vector pos(v->basePoint.x, v->basePoint.y);
                pl->addVertex(pos, 0.0, false);
            }
            m_currentContainer->addEntity(pl.release());
        }
        return;
    }

    if (data.flags & 0x40) {
        // the polyline is a polyface mesh
        std::vector<RS_Vector> vertices;
        for (const std::shared_ptr<DRW_Vertex>& v : data.vertlist) {
            if ((v->flags & 0x40) == 0) { // vertex
                vertices.emplace_back(v->basePoint.x, v->basePoint.y);
            }
        }
        // add faces as closed polylines
        for (const std::shared_ptr<DRW_Vertex>& f : data.vertlist) {
            if ((f->flags & 0x40) != 0) { // face
                std::vector<int> indices = {{f->vindex1, f->vindex2, f->vindex3, f->vindex4}};
                int num_points = (f->vindex4 == 0) ? 3 : 4;
                RS_PolylineData pd(RS_Vector(), RS_Vector(), true); // closed
                auto pl = std::make_unique<RS_Polyline>(m_currentContainer, pd);
                setEntityAttributes(pl.get(), &data);
                bool valid = true;
                for (int k = 0; k < num_points; ++k) {
                    int idx = std::abs(indices[k]);
                    if (idx < 1 || idx > static_cast<int>(vertices.size())) {
                        valid = false;
                        break;
                    }
                    pl->addVertex(vertices[idx - 1], 0.0);
                }
                if (valid) {
                    m_currentContainer->addEntity(pl.release());
                }
            }
        }
        return;
    }

    RS_PolylineData pd(RS_Vector{}, RS_Vector{}, data.flags & 0x1);
    auto polyline = std::make_unique<RS_Polyline>(m_currentContainer, pd);
    setEntityAttributes(polyline.get(), &data);

    if (data.vertlist.empty()) {
        m_currentContainer->addEntity(polyline.release());
        return;
    }

    auto vert0 = data.vertlist[0];
    RS_Vector first_pos(vert0->basePoint.x, vert0->basePoint.y);
    polyline->addVertex(first_pos, 0.0, false);
    RS_Vector prev_pos = first_pos;

    bool closed = (data.flags & 0x1) != 0;
    size_t num_segments = closed ? data.vertlist.size() : data.vertlist.size() - 1;

    for (size_t i = 0; i < num_segments; ++i) {
        size_t vert_idx = (i + 1) % data.vertlist.size();
        auto vert = data.vertlist[vert_idx];
        RS_Vector curr_pos(vert->basePoint.x, vert->basePoint.y);

        size_t bulge_idx = i % data.vertlist.size();
        double bulge = data.vertlist[bulge_idx]->bulge;
        const auto& extData = data.vertlist[bulge_idx]->extData;

        bool is_closed_seg = closed && (i == num_segments - 1);
        addPolylineSegment(*polyline, prev_pos, curr_pos, bulge, extData, is_closed_seg);

        prev_pos = curr_pos;
    }

    if (closed) {
        polyline->setFlag(RS2::FlagClosed);
        polyline->setNextBulge(data.vertlist.back()->bulge);
        polyline->getData().endpoint = polyline->getData().startpoint;
    } else {
        polyline->endPolyline();
    }

    m_currentContainer->addEntity(polyline.release());
}

/**
 * Implementation of the method which handles splines.
 */
void RS_FilterDXFRW::addSpline(const DRW_Spline* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addSpline: degree: %d", data->degree);

    // todo - sand - review this, quite a strange logic there... why spline points can't be with degree 3, for example?
    if(data->degree == 2){
        if (data->controllist.size() == 3) { // parabola
            auto toRs = [](const std::shared_ptr<DRW_Coord>& coord) -> RS_Vector {
                return coord ? RS_Vector{coord->x, coord->y} : RS_Vector{};
            };
            LC_ParabolaData d{{toRs(data->controllist.at(0)),
                               toRs(data->controllist.at(1)),
                               toRs(data->controllist.at(2))}};
            auto* parabola = new LC_Parabola(m_currentContainer, d);
            setEntityAttributes(parabola, data);
            parabola->update();
            m_currentContainer->addEntity(parabola);
            return;
        }
        else { // spline points
            LC_SplinePoints *splinePoints;
            LC_SplinePointsData d(((data->flags & 0x1) == 0x1), data->nfit == 0);
            splinePoints = new LC_SplinePoints(m_currentContainer, d);
            setEntityAttributes(splinePoints, data);
            m_currentContainer->addEntity(splinePoints);

            for (auto const &vert: data->controllist) {
                splinePoints->addControlPoint({vert->x, vert->y});
            }

            for (auto const &vert: data->fitlist) {
                splinePoints->addPoint({vert->x, vert->y});
            }

            splinePoints->update();
            return;
        }
    }

    // ordinary spline
    // bit coded: 1: closed; 2: periodic; 4: rational; 8: planar; 16:linear
    const bool isClosed = (data->flags & 0x2);
    RS_Spline* spline = nullptr;
    if (data->degree >= 1 && data->degree <= 3) {
        RS_SplineData d(data->degree, ((data->flags&0x1)==0x1));
        if (data->knotslist.size()) {
            double tolknot {(0 >= data->tolknot) ? 1e-7 : data->tolknot};
            for (auto const& k : data->knotslist) {
                d.knotslist.push_back( RS_Math::round(k, tolknot));
            }
        }
        // Currently all open splines are clamped at start/end points
        // Closed/periodic are initially read as open, and control point wrapping
        // will be added with extended knots
        d.type = isClosed ? RS_SplineData::SplineType::Standard : RS_SplineData::SplineType::ClampedOpen;
        spline = new RS_Spline(m_currentContainer, d);
        setEntityAttributes(spline, data);

        m_currentContainer->addEntity(spline);
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FilterDXF::addSpline: Invalid degree for spline: %d. "
                        "Accepted values are 1..3.", data->degree);
        return;
    }
    assert(data->controllist.size() == data->weightlist.size());
    if (data->controllist.size() != data->weightlist.size()) {
        return;
    }
    for (size_t i=0; i < data->controllist.size(); ++i) {
        const std::shared_ptr<DRW_Coord>& vert = data->controllist[i];
        const double weight = data->weightlist[i];
        spline->addControlPointRaw({vert->x, vert->y}, weight);
    }
    if (data->ncontrol== 0 && data->degree != 2){
        std::vector<RS_Vector> fitPoints;
        for (auto const& vert: data->fitlist)
            fitPoints.emplace_back(vert->x, vert->y);
        spline->setFitPoints(fitPoints);
    }

    if (isClosed) {
        // closed: add control point wrapping with extended knots
        spline->setClosed(true);
    }

    spline->update();
}

/**
 * Implementation of the method which handles inserts.
 */
void RS_FilterDXFRW::addInsert(const DRW_Insert& data) {
    RS_DEBUG->print("RS_FilterDXF::addInsert");

    RS_Vector ip(data.basePoint.x, data.basePoint.y);
    RS_Vector sc(data.xscale, data.yscale);
    RS_Vector sp(data.colspace, data.rowspace);

    //cout << "Insert: " << name << " " << ip << " " << cols << "/" << rows << endl;

    RS_InsertData d( QString::fromUtf8(data.name.c_str()),
                    ip, sc, data.angle,
                    data.colcount, data.rowcount,
					sp, nullptr, RS2::NoUpdate);
    RS_Insert* entity = new RS_Insert(m_currentContainer, d);
    setEntityAttributes(entity, &data);
    RS_DEBUG->print("  id: %lu", entity->getId());
//    entity->update();
    m_currentContainer->addEntity(entity);
}

void RS_FilterDXFRW::prepareTextStyleName(QString& sty) const {
    // use default style for the drawing:
    if (sty.isEmpty()) {
        // japanese, cyrillic:
        if (m_codePage=="ANSI_932" || m_codePage=="ANSI_1251") {
            sty = "Unicode";
        } else {
            sty = m_textStyle;
        }
    } else {
        sty = m_fontList.value(sty, sty);
    }
}

/**
 * Implementation of the method which handles
 * multi texts (MTEXT).
 */
void RS_FilterDXFRW::addMText(const DRW_MText& data) {
    RS_DEBUG->print("RS_FilterDXF::addMText: %s", data.text.c_str());

    RS_MTextData::VAlign valign;
    RS_MTextData::HAlign halign;
    RS_MTextData::MTextDrawingDirection dir;
    RS_MTextData::MTextLineSpacingStyle lss;


    if (data.textgen<=3) {
        valign=RS_MTextData::VATop;
    } else if (data.textgen<=6) {
        valign=RS_MTextData::VAMiddle;
    } else {
        valign=RS_MTextData::VABottom;
    }

    if (data.textgen%3==1) {
        halign=RS_MTextData::HALeft;
    } else if (data.textgen%3==2) {
        halign=RS_MTextData::HACenter;
    } else {
        halign=RS_MTextData::HARight;
    }

    if (data.alignH==1) {
        dir = RS_MTextData::LeftToRight;
    } else if (data.alignH==3) {
        dir = RS_MTextData::TopToBottom;
    } else {
        dir = RS_MTextData::ByStyle;
    }

    if (data.alignV==1) {
        lss = RS_MTextData::AtLeast;
    } else {
        lss = RS_MTextData::Exact;
    }
    QString mtext = toNativeString(QString::fromUtf8(data.text.c_str()));
    QString sty = QString::fromUtf8(data.style.c_str());
    prepareTextStyleName(sty);

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(mtext);
    double interlin = data.interlin;
    double angle = data.angle*M_PI/180.;
    RS_Vector ip = RS_Vector(data.basePoint.x, data.basePoint.y);

//Correct bad alignment of older dxflib or libdxfrw < 0.5.4
    if (m_oldMText) {
        interlin = data.interlin*0.96;
        if (valign == RS_MTextData::VABottom) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList tl = mtext.split('\n', Qt::SkipEmptyParts);
#else
            QStringList tl = mtext.split('\n', QString::SkipEmptyParts);
#endif
            if (!tl.isEmpty()) {
                QString txt = tl.at(tl.size()-1);
                RS_TextData d(RS_Vector(0.,0.,0.), RS_Vector(0.,0.,0.),

                              data.height, 1, RS_TextData::VABaseline, RS_TextData::HALeft,
                              RS_TextData::None, txt, sty, 0,
                              RS2::Update);
				auto entity = new RS_Text(nullptr, d);
                double textTail = entity->getMin().y;
                delete entity;
                auto ot = RS_Vector(0.0,textTail).rotate(angle);
                ip.move(ot);
            }
        }
    }

    RS_MTextData d(ip, data.height, data.widthscale,
                  valign, halign,
                  dir, lss,
                  interlin,
                  mtext, sty, angle,
                  RS2::NoUpdate);
    switch (data.alignH) {
        //case DRW_Text::DrawingDirection::LeftToRight:
        default:
            d.drawingDirection = RS_MTextData::MTextDrawingDirection::LeftToRight;
            break;
        case 3:
            d.drawingDirection = RS_MTextData::MTextDrawingDirection::TopToBottom;
            break;
        case 5:
            // FIXME: add style support
            d.drawingDirection = RS_MTextData::MTextDrawingDirection::RightToLeft;
            break;
    }
    auto entity = new RS_MText(m_currentContainer, d);
    setEntityAttributes(entity, &data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * texts (TEXT).
 */
void RS_FilterDXFRW::addText(const DRW_Text& data) {
    RS_DEBUG->print("RS_FilterDXFRW::addText");
    RS_Vector refPoint = RS_Vector(data.basePoint.x, data.basePoint.y);;
    RS_Vector secPoint = RS_Vector(data.secPoint.x, data.secPoint.y);;
    double angle = data.angle;

    if (data.alignV !=0 || data.alignH !=0 ||data.alignH ==DRW_Text::HMiddle){
        if (data.alignH !=DRW_Text::HAligned && data.alignH !=DRW_Text::HFit){
            secPoint = RS_Vector(data.basePoint.x, data.basePoint.y);
            refPoint = RS_Vector(data.secPoint.x, data.secPoint.y);
        }
    }

    RS_TextData::VAlign valign = (RS_TextData::VAlign)data.alignV;
    RS_TextData::HAlign halign = (RS_TextData::HAlign)data.alignH;
    RS_TextData::TextGeneration dir;
    QString sty = QString::fromUtf8(data.style.c_str());

    if (data.textgen==2) {
        dir = RS_TextData::Backward;
    } else if (data.textgen==4) {
        dir = RS_TextData::UpsideDown;
    } else {
        dir = RS_TextData::None;
    }

    QString text = toNativeString(QString::fromUtf8(data.text.c_str()));

    prepareTextStyleName(sty);

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(text);

    RS_TextData d(refPoint, secPoint, data.height, data.widthscale,
                  valign, halign, dir,
                  text, sty, angle*M_PI/180,
                  RS2::NoUpdate);
    auto* entity = new RS_Text(m_currentContainer, d);

    setEntityAttributes(entity, &data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * dimensions (DIMENSION).
 */
RS_DimensionData RS_FilterDXFRW::convDimensionData(const  DRW_Dimension* data) {
    DRW_Coord crd = data->getDefPoint();
    RS_Vector defP(crd.x, crd.y);
    crd = data->getTextPoint();
    RS_Vector midP(crd.x, crd.y);
    RS_MTextData::VAlign valign;
    RS_MTextData::HAlign halign;
    RS_MTextData::MTextLineSpacingStyle lss;
    QString sty = QString::fromUtf8(data->getStyle().c_str());

    QString t; //= data.text;

    // middlepoint of text can be 0/0 which is considered to be invalid (!):
    //  0/0 because older QCad versions save the middle of the text as 0/0
    //  although they didn't support saving of the middle of the text.
    if (fabs(crd.x)<1.0e-6 && fabs(crd.y)<1.0e-6) {
        midP = RS_Vector(false);
    }

    if (data->getAlign()<=3) {
        valign=RS_MTextData::VATop;
    } else if (data->getAlign()<=6) {
        valign=RS_MTextData::VAMiddle;
    } else {
        valign=RS_MTextData::VABottom;
    }

    if (data->getAlign()%3==1) {
        halign=RS_MTextData::HALeft;
    } else if (data->getAlign()%3==2) {
        halign=RS_MTextData::HACenter;
    } else {
        halign=RS_MTextData::HARight;
    }

    if (data->getTextLineStyle()==1) {
        lss = RS_MTextData::AtLeast;
    } else {
        lss = RS_MTextData::Exact;
    }

    t = toNativeString(QString::fromUtf8( data->getText().c_str() ));

    if (sty.isEmpty()) {
        sty = m_dimStyle;
    }

    RS_DEBUG->print("Text as unicode:");
    RS_DEBUG->printUnicode(t);

    bool customTextLocation = data->type >= 128;

    LC_DimStyle*  dimStyleOverride =  nullptr;

    if (!data->extData.empty()) {
        LC_ExtEntityData* extData = extractEntityExtData(data->extData);
        if (extData != nullptr) {
            dimStyleOverride = parseDimStyleOverride(extData);
            delete extData;
        }
    }

    // data needed to add the actual dimension entity
    return RS_DimensionData(defP, midP, valign, halign, lss, data->getTextLineFactor(), t, sty,
        data->getDir(), data->getHDir(), !customTextLocation, dimStyleOverride,
        data->getFlipArrow1(), data->getFlipArrow2());
}

void RS_FilterDXFRW::fillEntityExtData(std::vector<std::shared_ptr<DRW_Variant>> &extData, LC_ExtEntityData* entityData) {
    auto appDatas = entityData->getAppData();
    for (auto appData: *appDatas) {
        extData.push_back(std::make_shared<DRW_Variant>(1001, appData->getName().toStdString())); // application name
        auto groups = appData->getGroups();
        for (auto group: *groups) {
            extData.push_back(std::make_shared<DRW_Variant>(1000, group->getName().toStdString())); // group name
            extData.push_back(std::make_shared<DRW_Variant>(1002, "{")); // start

            auto tagsList = group->getTagsList();

            for (auto tag: *tagsList) {
                if (tag->isAtomic ()) {
                    // fixme - just plain list of tags within the group, no nesting!
                    auto variable = tag->var();
                    int code = variable->getCode();
                    extData.push_back(std::make_shared<DRW_Variant>(1070, code)); // code of variable
                    auto varType = variable->getType();
                    switch (varType) {
                        case RS2::VariableInt: {
                            extData.push_back(std::make_shared<DRW_Variant>(1070, variable->getInt())); // int value
                            break;
                        }
                        case RS2::VariableDouble: {
                            extData.push_back(std::make_shared<DRW_Variant>(1040, variable->getDouble())); // double value
                            break;
                        }
                        case RS2::VariableString: {
                            // int code = 1003;
                            // if (tag->isRef()) {
                                // code = 1005;
                            // }
                            // extData.push_back(std::make_shared<DRW_Variant>(code, variable->getString().toStdString())); // string value
                            extData.push_back(std::make_shared<DRW_Variant>((tag->isRef() ? 1005 : 1003), variable->getString().toStdString())); // string value
                            break;
                        }
                        case RS2::VariableVector: {
                            RS_Vector vec = variable->getVector();
                            extData.push_back(std::make_shared<DRW_Variant>(1010, vec.x)); // vector value
                            extData.push_back(std::make_shared<DRW_Variant>(1011, vec.y)); // vector value
                            extData.push_back(std::make_shared<DRW_Variant>(1012, vec.z)); // vector value
                            break;
                        }
                        case RS2::VariableVoid: {
                            break;
                        }
                    }
                }
            }
            extData.push_back(std::make_shared<DRW_Variant>(1002, "}")); // end
        }
    }
}

// this method is quite generic and may be used for parsing any entity's ext data
// fixme & todo - sand - nesting of tags is not supported so far in code, yet it's supported by DXF !!!!
LC_ExtEntityData* RS_FilterDXFRW::extractEntityExtData(const std::vector<std::shared_ptr<DRW_Variant>> &extData) {
    auto* result = new LC_ExtEntityData();
    LC_ExtDataAppData* currentAppData = nullptr;
    LC_ExtDataGroup* currentGroup = nullptr;

    int currentValType = -1;
    std::stack<LC_ExtDataTag*> tagStack;
    bool expectType = false;
    int listLevel = 0;
    bool inTagsList = false;
    for (auto v: extData) {
        int code = v->code();
        switch (code) {
            case 1001: { // application name
                QString applicationName = v->c_str();
                currentAppData = result->addAppData(applicationName);
                break;
            }
            case 1000: { // group name
                QString groupName = v->c_str();
                currentGroup = currentAppData->addGroup(groupName);
                break;
            }
            case 1002: { // control braces
                QString ctrlString = v->c_str();
                if (ctrlString == "{") { // fixme - sand - add support of lists nesting!!!
                    listLevel ++;
                    inTagsList = true;
                    expectType = false; // for later "not", as actually we do expect it
                }
                else { // end of list
                    listLevel --;
                    inTagsList = false;
                }
                break;
            }
            case 1003:
            case 1004:{
                QString val = v->c_str();
                if (currentGroup != nullptr) {
                    currentGroup->add(currentValType, val);
                }
                break;
            }
            case 1005:{
                QString val = v->c_str();
                if (currentGroup != nullptr) {
                    currentGroup->addRef(currentValType, val);
                }
                break;
            }
            case 1010:
            case 1011:
            case 1012:
            case 1013: {
                if (currentGroup != nullptr) {
                    auto coord = v->coord(); // fixme - sand - review how actually coordinate is parsed, and why it's on several codes??
                    if (coord != nullptr) {
                        currentGroup->add(currentValType, RS_Vector(coord->x, coord->y, coord->z));
                    }
                }
                break;
            }
            case 1070: // integer
            case 1071:{// long
                int val = v->i_val();
                if (expectType) {
                    // code of var
                    currentValType = val;
                }
                else {
                    // int field
                    if (currentGroup != nullptr) {
                        currentGroup->add(currentValType, val);
                    }
                }
                break;
            }
            case 1040: // real
            case 1041: // distance
            case 1042: { // scale factor
                double val = v->d_val();
                if (currentGroup != nullptr) {
                    currentGroup->add(currentValType, val);
                }
                break;
            }
            default:
                break;
        }
        if (inTagsList) {
            expectType = !expectType;
        }
    }
    return result;
}

bool RS_FilterDXFRW::shouldGenerateExtEntityData(RS_Dimension* entity) {
    // todo - so far, we support only dimension style override as extension data.
    // however, that's logic may be expanded later and store, for example,
    // something like entity-specific meta information or so.
    return entity->getDimStyleOverride() != nullptr;
}

QString RS_FilterDXFRW::toHexStr(int n){
    return QString::number(n, 16).toUpper();
}

void RS_FilterDXFRW::addDimStyleOverrideToExtendedData(LC_ExtEntityData* extEntityData, LC_DimStyle* styleOverride) {
    if (styleOverride == nullptr) {
        return;
    }
    auto appData = extEntityData->addAppData("ACAD");
    auto group = appData->addGroup("DSTYLE");

    auto arrowhead = styleOverride->arrowhead();
    auto linearFormat = styleOverride->linearFormat();
    auto extensionLine = styleOverride->extensionLine();
    auto dimensionLine = styleOverride->dimensionLine();
    auto text = styleOverride->text();
    auto tolerance = styleOverride->latteralTolerance();
    auto zerosSuppression = styleOverride->zerosSuppression();
    auto scaling = styleOverride->scaling();
    auto roundoff = styleOverride->roundOff();
    auto radial = styleOverride->radial();
    auto angularFormat = styleOverride->angularFormat();
    auto fractions = styleOverride->fractions();
    auto leader = styleOverride->leader();
    auto arc = styleOverride->arc();

    auto savedModificationMode = linearFormat->getModifyCheckMode();
    // here we're interested only in actually modified fields
    styleOverride->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);

    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMPOST)) { // $DIMPOST
        group->add(3, linearFormat->prefixOrSuffix());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMAPOST)) { // $DIMAPOST
        group->add(4, linearFormat->altPrefixOrSuffix());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) { // $DIMBLK
        // fixme - restore after test!
        // group->add(5, arrowhead->sameBlockName());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) { // $DIMBLK1
        // fixme - restore after test!
        // group->add(6, arrowhead->arrowHeadBlockNameFirst());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) { // $DIMBLK2
        // fixme - restore after test!
        // group->add(7, arrowhead->arrowHeadBlockNameSecond());
    }
    if (scaling->checkModifyState(LC_DimStyle::Scaling::$DIMSCALE)) { // $DIMSCALE
        group->add(40, scaling->scale());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMASZ)) { // $DIMASZ
        group->add(41, arrowhead->size());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXO)) { // $DIMEXO
        group->add(42, extensionLine->distanceFromOriginPoint());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLI)) {// $DIMDLI
        group->add(43, dimensionLine->baseLineDimLinesSpacing());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXE)) { // $DIMEXE
        group->add(44, extensionLine->distanceBeyondDimLine());
    }
    if (roundoff->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMRND)) { // $DIMRND
        group->add(45, roundoff->roundTo());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLE)) {// $DIMDLE
        group->add(46, dimensionLine->distanceBeyondExtLinesForObliqueStroke());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTP)) {// $DIMDTP
        group->add(47, tolerance->upperToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {// $DIMDTM
        group->add(48, tolerance->lowerToleranceLimit());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXL)) { // $DIMFXL
        group->add(49, extensionLine->fixedLength());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXT)) { // $DIMTXT
        group->add(140, text->height());
    }
    if (radial->checkModifyState(LC_DimStyle::Radial::$DIMCEN)) { // $DIMCEN
        group->add(141, radial->centerCenterMarkOrLineSize());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMTSZ)) { // $DIMTSZ
        group->add(142, arrowhead->tickSize());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTF)) { // $DIMALTF
        group->add(143, linearFormat->altUnitsMultiplier());
    }
    if (scaling->checkModifyState(LC_DimStyle::Scaling::$DIMLFAC)) { // $DIMLFAC
        group->add(144, scaling->linearFactor());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTVP)) { // $DIMTVP
        group->add(145, text->verticalDistanceToDimLine());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTFAC)) {// $DIMTFAC
        group->add(146, tolerance->heightScaleFactorToDimText());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMGAP)) {// $DIMGAP
        group->add(147, dimensionLine->lineGap());
    }
    if (roundoff->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMALTRND)) { // $DIMALTRND
        group->add(148, roundoff->altRoundTo());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILL)) { // $DIMTFILL
        group->add(69, text->backgroundFillMode());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILLCLR)) { // $DIMTFILLCLR
        int colorRgb;
        int colorNumber = colorToNumber(text->explicitBackgroundFillColor(), &colorRgb);
        group->add(70, colorNumber);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOL)) {// $DIMTOL
        group->add(71, tolerance->isAppendTolerancesToDimText() ? 1 : 0);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMLIM)) {// $DIMLIM
        group->add(72, tolerance->isLimitsGeneratedAsDefaultText() ? 1 : 0);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIH)) { // $DIMTIH
        group->add(73, text->orientationInside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTOH)) { // $DIMTOH
        group->add(74, text->orientationOutside());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE1)) { // $DIMSE1
        group->add(75, extensionLine->suppressFirstLine());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE2)) { // $DIMSE2
        group->add(76, extensionLine->suppressSecondLine());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTAD)) { // $DIMTAD
        group->add(77, text->verticalPositioning());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMZIN)) { // $DIMZIN
        group->add(78, zerosSuppression->linearRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMAZIN)) { // $DIMAZIN
        group->add(79, zerosSuppression->angularRaw());
    }
    if (arc->checkModifyState(LC_DimStyle::Arc::$DIMARCSYM)) { // $DIMARCSYM
        group->add(90, arc->arcSymbolPosition());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALT)) { // $DIMALT
        group->add(170, linearFormat->alternateUnits());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTD)) { // $DIMALTD
        group->add(171, linearFormat->altDecimalPlaces());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMTOFL)) {// $DIMTOFL
        group->add(172, dimensionLine->drawPolicyForOutsideText());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH)) { // $DIMSAH
        group->add(173, arrowhead->isUseSeparateArrowHeads()); // fixme - check value
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIX)) { // $DIMTIX
        group->add(174, text->extLinesRelativePlacement());
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSOXD)) { // $DIMSOXD
        group->add(175, arrowhead->suppression()); // fixme - check value
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMCLRD)) {// $DIMCLRD
        int colorRgb;
        int colorNumber = colorToNumber(dimensionLine->color(), &colorRgb);
        group->add(176, colorNumber);
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMCLRE)) {// $DIMCLRE
        int colorRgb;
        int color = colorToNumber(extensionLine->color(), &colorRgb);
        group->add(177, color);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMCLRT)) {// $DIMCLRT
        int colorRgb;
        int colorNumber = colorToNumber(text->color(), &colorRgb);
        group->add(178, colorNumber);
    }
    if (angularFormat->checkModifyState(LC_DimStyle::AngularFormat::$DIMADEC)) { // $DIMADEC
        group->add(179, angularFormat->decimalPlaces());
    }
    // case 270: // fixme - sand - obsolete DIMUNIT
    // result->linearFormat()->setUDecimalPlaces(var->getInt());
    // dimunit = reader->getInt32();
    // add("$DIMUNIT", code, dimunit);
    // break;
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMDEC)) { // $DIMDEC
        group->add(271, linearFormat->decimalPlaces());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTDEC)) {// $DIMTDEC
        group->add(272, tolerance->decimalPlaces());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTU)) { // $DIMALTU
        group->add(273, linearFormat->altFormat());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMALTTD)) {// $DIMALTTD
        group->add(274, tolerance->decimalPlacesAltDim());
    }
    if (angularFormat->checkModifyState(LC_DimStyle::AngularFormat::$DIMAUNIT)) { // $DIMAUNIT
        group->add(275, angularFormat->format());
    }
    if (fractions->checkModifyState(LC_DimStyle::Fractions::$DIMFRAC)) { // $DIMFRAC
        group->add(276, fractions->style());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMLUNIT)) { // $DIMLUNIT
        group->add(277, linearFormat->formatRaw());
    }
    if (linearFormat->checkModifyState(LC_DimStyle::LinearFormat::$DIMSEP)) { // $DIMDSEP
        group->add(278, linearFormat->decimalFormatSeparatorChar());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTMOVE)) { // $DIMTMOVE
        group->add(279, text->positionMovementPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMJUST)) { // $DIMJUST
        group->add(280, text->horizontalPositioning());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD1)) {// $DIMSD1
        group->add(281, dimensionLine->suppressFirstLine());
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD2)) {// $DIMSD2
        group->add(282, dimensionLine->suppressSecondLine());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOLJ)) {// $DIMTOLJ
        group->add(283, tolerance->verticalJustification());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMTZIN)) { // $DIMTZIN
        group->add(284, zerosSuppression->toleranceRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTZ)) { // $DIMALTZ
        group->add(285, zerosSuppression->altLinearRaw());
    }
    if (zerosSuppression->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTTZ)) { // $DIMALTTZ
        group->add(286, zerosSuppression->altToleranceRaw());
    }
    //case 287: // fixme - DIMFIT
    // dimfit = reader->getInt32();
    // add("$DIMFIT", code, dimfit);
    if (text->checkModifyState(LC_DimStyle::Text::$DIMUPT)) { // $DIMUPT
        group->add(288, text->cursorControlPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMATFIT)) { // $DIMATFIT
        group->add(289, text->unsufficientSpacePolicy());
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXLON)) { // $DIMFXLON
        group->add(290, extensionLine->hasFixedLength() ? 1 : 0);  // fixme - check
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXTDIRECTION)) { // $DIMTXTDIRECTION
        group->add(292, text->readingDirection());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXSTY)) { // $DIMTXSTY
        group->add(340, text->style()); // fixme - ref to style?
    }
    if (leader->checkModifyState(LC_DimStyle::Leader::$DIMLDRBLK)) { //DIMLDRBLK
        auto blockName = leader->arrowBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                group->addRef(341, toHexStr(blkHandle));
            }
        }
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) { //$DIMBLK
        auto blockName = arrowhead->sameBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                group->addRef(342, toHexStr(blkHandle));
            }
        }
    }
    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) { //$DIMBLK1
        auto blockName = arrowhead->arrowHeadBlockNameFirst();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                group->addRef(343, toHexStr(blkHandle));
            }
        }
    }

    if (arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) { //$DIMBLK2
        auto blockName = arrowhead->arrowHeadBlockNameSecond();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                group->addRef(344, toHexStr(blkHandle));
            }
        }
    }
/*
                    // case 345: // codes///
                    // fixme - may this code be used for DIMLDRBLK?
                    //      dimblk2 = reader->getUtf8String();
                    //      add("$DIMBLK2", code, dimblk2);
                    //      break;
     */

    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLTYPE)) { // $DIMLTYPE
        int lineTypeHandle = findLineTypeHandleToWrite(dimensionLine->lineTypeName());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(345, handleStr);
        }
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX1)) { // $DIMLTEX1
        int lineTypeHandle = findLineTypeHandleToWrite(extensionLine->lineTypeFirstRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(347, handleStr);
        }
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX2)) { // $DIMLTEX2
        int lineTypeHandle = findLineTypeHandleToWrite(extensionLine->lineTypeSecondRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            group->addRef(348, handleStr);
        }
    }
    if (dimensionLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLWD)) { // $DIMLWD
        auto lineWidth = dimensionLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        group->add(371, lw);
    }
    if (extensionLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLWE)) { // $DIMLWE
        auto lineWidth = extensionLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        group->add(372, lw);
    }

    styleOverride->setModifyCheckMode(savedModificationMode);
}

LC_DimStyle* RS_FilterDXFRW::parseDimStyleOverride(LC_ExtEntityData* extEntityData) const {
    if (extEntityData != nullptr) {
        auto dimStyleGroup = extEntityData->getGroupByName("ACAD", "DSTYLE");
        if (dimStyleGroup != nullptr) {
            auto dimStyleVariables = dimStyleGroup->getTagsList();
            if (dimStyleVariables->empty()) {
                return nullptr;
            }
            auto* result = new LC_DimStyle();
            auto arrowhead = result->arrowhead();
            auto linearFormat = result->linearFormat();
            auto extensionLine = result->extensionLine();
            auto dimensionLine = result->dimensionLine();
            auto text = result->text();
            auto tolerance = result->latteralTolerance();
            auto zerosSuppression = result->zerosSuppression();
            auto arc = result->arc();

            result->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
            auto count = dimStyleVariables->size();
            for (size_t i = 0; i < count; i++) {
                LC_ExtDataTag* tag = dimStyleVariables->at(i);
                RS_Variable* var = tag->var();
                int code = var->getCode();

                switch (code) {
                    case 105:
                        // handle = reader->getHandleString();
                        break;
                    case 3: // "$DIMPOST"
                        linearFormat->setPrefixOrSuffix(var->getString());
                        break;
                    case 4: // "$DIMAPOST"
                        linearFormat->setAltPrefixOrSuffix(var->getString());
                        break;
                    case 5: // $DIMBLK
                        arrowhead->setSameBlockName(var->getString());
                        break;
                    case 6: // "$DIMBLK1"
                        arrowhead->setArrowHeadBlockNameFirst(var->getString());
                        break;
                    case 7: // "$DIMBLK2"
                        arrowhead->setArrowHeadBlockNameSecond(var->getString());
                        break;
                    case 40: // "$DIMSCALE"
                        result->scaling()->setScale(var->getDouble());
                        break;
                    case 41: // "$DIMASZ"
                        arrowhead->setSize(var->getDouble());
                        break;
                    case 42: // "$DIMEXO"
                        extensionLine->setDistanceFromOriginPoint(var->getDouble());
                        break;
                    case 43: //"$DIMDLI"
                        dimensionLine->setBaselineDimLinesSpacing(var->getDouble());
                        break;
                    case 44: //"$DIMEXE"
                        extensionLine->setDistanceBeyondDimLine(var->getDouble());
                        break;
                    case 45: //$DIMRND
                        result->roundOff()->setRoundToValue(var->getDouble());
                        break;
                    case 46: //$DIMDLE
                        dimensionLine->setDistanceBeyondExtLinesForObliqueStroke(var->getDouble());
                        break;
                    case 47: //"$DIMTP"
                        tolerance->setUpperToleranceLimit(var->getDouble());
                        break;
                    case 48: //"$DIMTM"
                        tolerance->setLowerToleranceLimit(var->getDouble());
                        break;
                    case 49: //"$DIMFXL"
                        extensionLine->setFixedLength(var->getDouble());
                        break;
                    case 140:// "$DIMTXT"
                        text->setHeight(var->getDouble());
                        break;
                    case 141: // "$DIMCEN"
                        result->radial()->setCenterMarkOrLineSize(var->getDouble());
                        break;
                    case 142: //"$DIMTSZ"
                        arrowhead->setTickSize(var->getDouble());
                        break;
                    case 143: //"$DIMALTF"
                        linearFormat->setAltUnitsMultiplier(var->getDouble());
                        break;
                    case 144: //"$DIMLFAC"
                        result->scaling()->setLinearFactor(var->getDouble());
                        break;
                    case 145: //"$DIMTVP"
                        text->setVerticalDistanceToDimLine(var->getDouble());
                        break;
                    case 146: //"$DIMTFAC"
                        tolerance->setHeightScaleFactorToDimText(var->getDouble());
                        break;
                    case 147: //"$DIMGAP"
                        dimensionLine->setLineGap(var->getDouble());
                        break;
                    case 148: //"$DIMALTRND"
                        result->roundOff()->setAltRoundToValue(var->getDouble());
                        break;
                    case 69: // "$DIMTFILL"
                        text->setBackgroundFillModeRaw(var->getInt());
                        break;
                    case 70: {
                        //"$DIMTFILLCLR"
                        RS_Color fillClr = numberToColor(var->getInt());
                        text->setExplicitBackgroundFillColor(fillClr);
                        break;
                    }
                    case 71: // "$DIMTOL"
                        tolerance->setAppendTolerancesToDimText(var->getInt() == 1); // fixme - review
                        break;
                    case 72: //"$DIMLIM"
                        tolerance->setLimitsAreGeneratedAsDefaultText(var->getInt() == 1); // fixme - review
                        break;
                    case 73: //"$DIMTIH"
                        text->setOrientationInsideRaw(var->getInt());
                        break;
                    case 74: //"$DIMTOH"
                        text->setOrientationOutsideRaw(var->getInt());
                        break;
                    case 75: //"$DIMSE1"
                        extensionLine->setSuppressFirstRaw(var->getInt());
                        break;
                    case 76: //"$DIMSE2"
                        extensionLine->setSuppressSecondRaw(var->getInt());
                        break;
                    case 77: //"$DIMTAD"
                        text->setVerticalPositioningRaw(var->getInt());
                        break;
                    case 78: // "$DIMZIN"
                        zerosSuppression->setLinearRaw(var->getInt());
                        break;
                    case 79: //"$DIMAZIN"
                        zerosSuppression->setAngularRaw(var->getInt());
                        break;
                    case 170: //"$DIMALT"
                        linearFormat->setAlternateUnitsRaw(var->getInt());
                        break;
                    case 171: //"$DIMALTD"
                        linearFormat->setAltDecimalPlaces(var->getInt());
                        break;
                    case 172: //"$DIMTOFL"
                        dimensionLine->setDrawPolicyForOutsideTextRaw(var->getInt());
                        break;
                    case 173: //"$DIMSAH"
                        arrowhead->setUseSeparateArrowHeads(var->getInt()); // fixme - check value
                        break;
                    case 174: // "$DIMTIX"
                        text->setExtLinesRelativePlacementRaw(var->getInt());
                        break;
                    case 175: //"$DIMSOXD"
                        arrowhead->setSuppressionsRaw(var->getInt()); // fixme - check value
                        break;
                    case 176: { //"$DIMCLRD"
                        RS_Color color = numberToColor(var->getInt());
                        dimensionLine->setColor(color);
                        break;
                    }
                    case 177: { //"$DIMCLRE"
                        RS_Color color = numberToColor(var->getInt());
                        extensionLine->setColor(color);
                        break;
                    }
                    case 178: { //"$DIMCLRT"
                        RS_Color color = numberToColor(var->getInt());
                        text->setColor(color);
                        break;
                    }
                    case 179: //"$DIMADEC"
                        result->angularFormat()->setDecimalPlaces(var->getInt());
                        break;
                    case 270: // fixme - sand - obsolete DIMUNIT
                        // result->linearFormat()->setUDecimalPlaces(var->getInt());
                        // dimunit = reader->getInt32();
                        // add("$DIMUNIT", code, dimunit);
                        break;
                    case 271: //"$DIMDEC"
                        linearFormat->setDecimalPlaces(var->getInt());
                        break;
                    case 272://"$DIMTDEC"
                        tolerance->setDecimalPlaces(var->getInt());
                        break;
                    case 273: //"$DIMALTU"
                        linearFormat->setAltFormatRaw(var->getInt());
                        break;
                    case 274: // "$DIMALTTD"
                        tolerance->setDecimalPlacesAltDim(var->getInt());
                        break;
                    case 275: //"$DIMAUNIT"
                        result->angularFormat()->setFormatRaw(var->getInt());
                        break;
                    case 276: // "$DIMFRAC"
                        result->fractions()->setStyleRaw(var->getInt());
                        break;
                    case 277: // "$DIMLUNIT"
                        linearFormat->setFormatRaw(var->getInt());
                        break;
                    case 278: //"$DIMDSEP"
                        linearFormat->setDecimalFormatSeparatorChar(var->getInt());
                        break;
                    case 279: // "$DIMTMOVE"
                        text->setPositionMovementPolicyRaw(var->getInt());
                        break;
                    case 280: // "$DIMJUST"
                        text->setHorizontalPositioningRaw(var->getInt());
                        break;
                    case 281: // "$DIMSD1"
                        dimensionLine->setSuppressFirstLineRaw(var->getInt());
                        break;
                    case 282: // "$DIMSD2"
                        dimensionLine->setSuppressSecondLineRaw(var->getInt());
                        break;
                    case 283: // "$DIMTOLJ"
                        tolerance->setVerticalJustificationRaw(var->getInt());
                        break;
                    case 284:// "$DIMTZIN"
                        zerosSuppression->setToleranceRaw(var->getInt());
                        break;
                    case 285: // "$DIMALTZ"
                        zerosSuppression->setAltLinearRaw(var->getInt());
                        break;
                    case 286: //"$DIMALTTZ"
                        zerosSuppression->setAltToleranceRaw(var->getInt());
                        break;
                    case 287: // fixme - DIMFIT
                        // dimfit = reader->getInt32();
                        // add("$DIMFIT", code, dimfit);
                        break;
                    case 288: // "$DIMUPT"
                        text->setCursorControlPolicyRaw(var->getInt());
                        break;
                    case 289: //"$DIMATFIT"
                        text->setUnsufficientSpacePolicyRaw(var->getInt());
                        break;
                    case 290: // "$DIMFXLON"
                        extensionLine->setHasFixedLength(var->getInt()); // fixme - check
                        break;
                    case 292: // "$DIMTXTDIRECTION"
                        text->setReadingDirectionRaw(var->getInt());
                        break;
                    case 340: // "$DIMTXSTY"
                    {
                        auto dimtxsty = var->getString();
                        prepareTextStyleName(dimtxsty);
                        text->setStyle(dimtxsty); // fixme - ref to style?
                        // fixme - ref to style?
                        break;
                    }
                    case 341: {
                        // "_$DIMLDRBLK"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                result->leader()->setArrowBlockName(blockName);
                            }
                        }
                        else { // the string is not handle, but a direct name of the block.
                            // fixme - DIMLDRBLK reading!
                            // This is workaround for referring leader by name, not by block ref...
                            // however, it may be not ACad compatible..
                            // if (LC_DimArrowRegistry::isStandardBlockName(refHandleStr)) {
                                // result->leader()->setArrowBlockName(refHandleStr);
                            // }
                        }
                        break;
                    }
                    case 342: {// "_$DIMBLK"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setSameBlockName(blockName);
                            }
                        }
                        break;
                    }
                    case 343: {
                        // "_$DIMBLK1"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setArrowHeadBlockNameFirst(blockName);
                            }
                        }
                        break;
                    }
                    case 344: {
                        // "_$DIMBLK2"
                        QString refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        if (ok) {
                            QString blockName;
                            ok = resolveBlockNameByHandle(refHandle, blockName);
                            if (ok) {
                                arrowhead->setArrowHeadBlockNameSecond(blockName);
                            }
                        }
                        break;
                    }
                    // case 345: // codes///
                    // fixme - may this code be used for DIMLDRBLK?
                    //      dimblk2 = reader->getUtf8String();
                    //      add("$DIMBLK2", code, dimblk2);
                    //      break;
                    case 345: {
                        // "$DIMLTYPE"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            dimensionLine->setLineType(name);
                        }
                        break;
                    }
                    case 347: { // "$DIMLTEX1"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            extensionLine->setLineTypeFirst(name);
                        }
                        break;
                    }
                    case 348: { //"$DIMLTEX2"
                        auto refHandleStr = var->getString();
                        bool ok;
                        int refHandle = refHandleStr.toInt(&ok, 16);
                        auto lineTypeName = m_dxfR->getReadingContext()->resolveLineTypeName(refHandle);
                        if (!lineTypeName.empty()) {
                            QString name = QString::fromStdString(lineTypeName);
                            extensionLine->setLineTypeSecond(name);
                        }
                        break;
                    }
                    case 371: // "$DIMLWD"
                        dimensionLine->setLineWidthRaw(var->getInt());
                        break;
                    case 372: //"$DIMLWE"
                        extensionLine->setLineWidthRaw(var->getInt());
                        break;
                    case 90: //"$DIMARCSYM"
                        arc->setArcSymbolPositionRaw(var->getInt());
                        break;
                    default:
                        break;
                }
            }

            // workaround for AutoCAD - it seems that later versions ignores DIMSAH and consider blocks for arrowhead to be
            // different if individual arrows are different. So we set the flag manually
            // fixme - check whether given condition is enough for properly setting the flag
            result->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);
            if (!arrowhead->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH) ) {
                if(arrowhead->sameBlockName().isEmpty() && (arrowhead->arrowHeadBlockNameFirst() != arrowhead->arrowHeadBlockNameSecond())) {
                    arrowhead->setUseSeparateArrowHeads(true);
                }
            }

            return result;
        }
    }
    return nullptr;
}

/**
 * Implementation of the method which handles
 * aligned dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAlign(const DRW_DimAligned *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAligned");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector ext1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector ext2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimAlignedData d(ext1, ext2);
    auto* entity = new RS_DimAligned(m_currentContainer,dimensionData, d);
    setEntityAttributes(entity, data);
    entity->updateDimPoint();
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * linear dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimLinear(const DRW_DimLinear *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLinear");

    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector dxt1(data->getDef1Point().x, data->getDef1Point().y);
    RS_Vector dxt2(data->getDef2Point().x, data->getDef2Point().y);

    RS_DimLinearData d(dxt1, dxt2,
                       RS_Math::deg2rad(data->getAngle()), RS_Math::deg2rad(data->getOblique()));

    auto entity = new RS_DimLinear(m_currentContainer,dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * radial dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimRadial(const DRW_DimRadial* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimRadial");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(data->getDiameterPoint().x, data->getDiameterPoint().y);

    RS_DimRadialData d(dp, data->getLeaderLength());
    auto entity = new RS_DimRadial(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * diametric dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimDiametric(const DRW_DimDiametric* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimDiametric");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp(data->getDiameter1Point().x, data->getDiameter1Point().y);

    RS_DimDiametricData d(dp, data->getLeaderLength());
    auto entity = new RS_DimDiametric(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular(const DRW_DimAngular* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine1().x, data->getFirstLine1().y);
    RS_Vector dp2(data->getFirstLine2().x, data->getFirstLine2().y);
    RS_Vector dp3(data->getSecondLine1().x, data->getSecondLine1().y);
    RS_Vector dp4(data->getDimPoint().x, data->getDimPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    auto entity = new RS_DimAngular(m_currentContainer,dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles
 * angular dimensions (DIMENSION).
 */
void RS_FilterDXFRW::addDimAngular3P(const DRW_DimAngular3p* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimAngular3P");

    RS_DimensionData dimensionData = convDimensionData(data);
    RS_Vector dp1(data->getFirstLine().x, data->getFirstLine().y);
    RS_Vector dp2(data->getSecondLine().x, data->getSecondLine().y);
    RS_Vector dp3(data->getVertexPoint().x, data->getVertexPoint().y);
	RS_Vector dp4 = dimensionData.definitionPoint;
	dimensionData.definitionPoint = RS_Vector(data->getVertexPoint().x, data->getVertexPoint().y);

    RS_DimAngularData d(dp1, dp2, dp3, dp4);

    auto entity = new RS_DimAngular(m_currentContainer, dimensionData, d);

    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

void RS_FilterDXFRW::addDimOrdinate(const DRW_DimOrdinate* data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimOrdinate(const DL_DimensionData&, const DL_DimOrdinateData&) not yet implemented");
    RS_DimensionData dimensionData = convDimensionData(data);

    RS_Vector featurePoint{data->getFirstLine().x, data->getFirstLine().y};
    RS_Vector leaderEndPoint{data->getSecondLine().x, data->getSecondLine().y};

    bool ordinateTypeForX = false;
    int type = data->type;
    if (type & 64) {
        ordinateTypeForX = true;
    }
    LC_DimOrdinateData d(featurePoint, leaderEndPoint, ordinateTypeForX);
    auto* entity = new LC_DimOrdinate(m_currentContainer, dimensionData, d);
    setEntityAttributes(entity, data);
    entity->update();
    m_currentContainer->addEntity(entity);
}

/**
 * Implementation of the method which handles leader entities.
 */
void RS_FilterDXFRW::addLeader(const DRW_Leader *data) {
    RS_DEBUG->print("RS_FilterDXFRW::addDimLeader");
    RS_LeaderData d(data->arrow!=0, QString::fromUtf8(data->style.c_str()));
    auto leader = new RS_Leader(m_currentContainer, d);
    setEntityAttributes(leader, data);

	for (auto const& vert: data->vertexlist) {
	    leader->addVertex({vert->x, vert->y});
	}

    leader->update();
    m_currentContainer->addEntity(leader);
}

/**
 * Implementation of the method which handles hatch entities.
 */
void RS_FilterDXFRW::addHatch(const DRW_Hatch *data) {
    RS_DEBUG->print("RS_FilterDXF::addHatch()");
    RS_EntityContainer* hatchLoop;
    auto hatch = new RS_Hatch(m_currentContainer,
                         RS_HatchData(data->solid, data->scale, data->angle,
                                      QString::fromUtf8(data->name.c_str())));
    setEntityAttributes(hatch, data);
    m_currentContainer->appendEntity(hatch);

    for (unsigned int i=0; i < data->looplist.size(); i++) {
        auto& loop = data->looplist.at(i);
        if ((loop->type & 32) == 32) {
            continue;
        }
        hatchLoop = new RS_EntityContainer(hatch);
        hatchLoop->setLayer(nullptr);
        hatch->addEntity(hatchLoop);

        RS_Entity* e = nullptr;
        if ((loop->type & 2) == 2){   //polyline, convert to lines & arcs
            DRW_LWPolyline* pline = static_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
            RS_Polyline polyline{nullptr,
                                 RS_PolylineData(RS_Vector(false), RS_Vector(false), pline->flags)};
            for (auto const& vert: pline->vertlist) {
                polyline.addVertex(RS_Vector{vert->x, vert->y}, vert->bulge);
            }

            for(RS_Entity* e: lc::LC_ContainerTraverser{polyline, RS2::ResolveNone}.entities()) {
         //   for (RS_Entity* e=polyline.firstEntity(); e; e=polyline.nextEntity()) {
                RS_Entity* tmp = e->clone();
                tmp->reparent(hatchLoop);
                tmp->setLayer(nullptr);
                hatchLoop->addEntity(tmp);
            }

        } else {
            for (unsigned int j=0; j<loop->objlist.size(); j++) {
                e = nullptr;
                auto& ent = loop->objlist.at(j);
                switch (ent->eType) {
                    case DRW::LINE: {
                        DRW_Line *e2 = static_cast<DRW_Line*>(ent.get());
                        e = new RS_Line{hatchLoop,
                                        {{e2->basePoint.x, e2->basePoint.y},
                                         {e2->secPoint.x, e2->secPoint.y}}};
                        break;
                    }
                    case DRW::ARC: {
                        DRW_Arc *e2 = static_cast<DRW_Arc*>(ent.get());
                        if (e2->isccw && e2->staangle<1.0e-6 && e2->endangle>RS_Math::deg2rad(360)-1.0e-6) {
                            e = new RS_Circle(hatchLoop,
                                              {{e2->basePoint.x, e2->basePoint.y},
                                               e2->radious});
                        } else {

                            if (e2->isccw) {
                                e = new RS_Arc(hatchLoop,
                                               RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                          RS_Math::correctAngle(e2->staangle),
                                                          RS_Math::correctAngle(e2->endangle),
                                                          false));
                            } else {
                                e = new RS_Arc(hatchLoop,
                                               RS_ArcData(RS_Vector(e2->basePoint.x, e2->basePoint.y), e2->radious,
                                                          RS_Math::correctAngle(2*M_PI-e2->staangle),
                                                          RS_Math::correctAngle(2*M_PI-e2->endangle),
                                                          true));
                            }
                        }
                        break;
                    }
                    case DRW::ELLIPSE: {
                        DRW_Ellipse *e2 = static_cast<DRW_Ellipse*>(ent.get());
                        double ang1 = e2->staparam;
                        double ang2 = e2->endparam;
                        if ( fabs(ang2 - 2.*M_PI) < 1.0e-10 && fabs(ang1) < 1.0e-10 ) {
                            ang2 = 0.0;
                        }
                        else { //convert angle to parameter
                            ang1 = atan(tan(ang1)/e2->ratio);
                            ang2 = atan(tan(ang2)/e2->ratio);
                            if (ang1 < 0) {
                                //quadrant 2 & 4
                                ang1 +=M_PI;
                                if (e2->staparam > M_PI) {
                                    //quadrant 4
                                    ang1 += M_PI;
                                }
                            } else if (e2->staparam > M_PI){//3 quadrant
                                ang1 +=M_PI;
                            }
                            if (ang2 < 0){//quadrant 2 & 4
                                ang2 +=M_PI;
                                if (e2->endparam > M_PI) {
                                    //quadrant 4
                                    ang2 +=M_PI;
                                }
                            } else if (e2->endparam > M_PI){//3 quadrant
                                ang2 +=M_PI;
                            }
                        }
                        e = new RS_Ellipse{hatchLoop,
                                           {{e2->basePoint.x, e2->basePoint.y},
                                            {e2->secPoint.x, e2->secPoint.y},
                                            e2->ratio, ang1, ang2, !e2->isccw}};
                        break;
                    }
                    default:
                        break;
                }
                if (e) {
                    e->setLayer(nullptr);
                    hatchLoop->addEntity(e);
                }
            }
        }

    }

    RS_DEBUG->print("hatch->update()");
    if (hatch->validate()) {
        hatch->update();
    } else {
        m_graphic->removeEntity(hatch);
        RS_DEBUG->print(RS_Debug::D_ERROR,"RS_FilterDXFRW::endEntity(): updating hatch failed: invalid hatch area");
    }
}

/**
 * Implementation of the method which handles image entities.
 */
void RS_FilterDXFRW::addImage(const DRW_Image *data) {
    RS_DEBUG->print("RS_FilterDXF::addImage");

    RS_Vector ip(data->basePoint.x, data->basePoint.y);
    RS_Vector uv(data->secPoint.x, data->secPoint.y);
    RS_Vector vv(data->vVector.x, data->vVector.y);
    RS_Vector size(data->sizeu, data->sizev);

    auto image = new RS_Image( m_currentContainer,
            RS_ImageData(data->ref, ip, uv, vv, size,
                         QString(""), data->brightness,
                         data->contrast, data->fade));

    setEntityAttributes(image, data);
    m_currentContainer->appendEntity(image);
}

/**
 * Implementation of the method which links image entities to image files.
 */
void RS_FilterDXFRW::linkImage(const DRW_ImageDef *data) {
    RS_DEBUG->print("RS_FilterDXFRW::linkImage");

    int handle = data->handle;
    QString sfile(QString::fromUtf8(data->name.c_str()));
    QFileInfo fiDxf(m_file);
    QFileInfo fiBitmap(sfile);

    // try to find the image file:

    // first: absolute path:
    if (!fiBitmap.exists()) {
        RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(sfile));
        // try relative path:
        QString f1 = fiDxf.absolutePath() + "/" + sfile;
        if (QFileInfo(f1).exists()) {
            sfile = f1;
        } else {
            RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f1));
            // try drawing path:
            QString f2 = fiDxf.absolutePath() + "/" + fiBitmap.fileName();
            if (QFileInfo(f2).exists()) {
                sfile = f2;
            } else {
                RS_DEBUG->print("File %s doesn't exist.", (const char*)QFile::encodeName(f2));
            }
        }
    }

    // Also link images in subcontainers (e.g. inserts):
    for(RS_Entity* e: lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (e->rtti()==RS2::EntityImage) {
            auto img = static_cast<RS_Image*>(e);
            if (img->getHandle()==handle) {
                img->setFile(sfile);
                RS_DEBUG->print("image found: %s", (const char*)QFile::encodeName(img->getFile()));
                img->update();
            }
        }
    }

    // update images in blocks:
    for (unsigned i=0; i<m_graphic->countBlocks(); ++i) {
        RS_Block* b = m_graphic->blockAt(i);
        for(RS_Entity* e: lc::LC_ContainerTraverser{*b, RS2::ResolveNone}.entities()) {
            if (e->rtti()==RS2::EntityImage) {
                auto img = static_cast<RS_Image*>(e);
                if (img->getHandle()==handle) {
                    img->setFile(sfile);
                    RS_DEBUG->print("image in block found: %s",(const char*)QFile::encodeName(img->getFile()));
                    img->update();
                }
            }
        }
    }
    RS_DEBUG->print("linking image: OK");
}

using std::map;
/**
 * Sets the header variables from the DXF file.
 */
void RS_FilterDXFRW::addHeader(const DRW_Header* data){
	RS_Graphic* container = nullptr;
    if (m_currentContainer->rtti()==RS2::EntityGraphic) {
        container = static_cast<RS_Graphic*>(m_currentContainer);
    } else {
        return;
    }

    for (auto it = data->vars.begin() ; it != data->vars.end(); ++it ) {
        QString key = QString::fromStdString((*it).first);
        DRW_Variant *var = (*it).second;
        switch (var->type()) {
        case DRW_Variant::COORD:
            container->addVariable(key,
            RS_Vector(var->content.v->x, var->content.v->y, var->content.v->z), var->code());
            break;
        case DRW_Variant::STRING:
            container->addVariable(key, QString::fromUtf8(var->content.s->c_str()), var->code());
            break;
        case DRW_Variant::INTEGER:
            container->addVariable(key, var->content.i, var->code());
            break;
        case DRW_Variant::DOUBLE:
            container->addVariable(key, var->content.d, var->code());
            break;
        default:
            break;
        }
    }

    for (auto it = data->customVars.begin() ; it != data->customVars.end(); ++it ) {
        QString key = QString::fromStdString((*it).first);
        DRW_Variant *var = (*it).second;
        container->addCustomProperty(key, var->c_str());
    }

    m_codePage = m_graphic->getVariableString("$DWGCODEPAGE", "ANSI_1252");
    m_textStyle = m_graphic->getVariableString("$TEXTSTYLE", "Standard");
    m_dimStyle = m_graphic->getVariableString("$DIMSTYLE", "Standard");
    //initialize units vars if not are present in dxf file
    m_graphic->getVariableInt("$LUNITS", 2);
    m_graphic->getVariableInt("$LUPREC", 4);
    m_graphic->getVariableInt("$AUNITS", 0);
    m_graphic->getVariableInt("$AUPREC", 4);

	//initialize points drawing style vars if not present in dxf file
    if (m_graphic->getVariableInt("$PDMODE", -999) < 0) {
        m_graphic->addVariable("$PDMODE", LC_DEFAULTS_PDMode, DXF_FORMAT_GC_VarName);
    }
    if (m_graphic->getVariableDouble("$PDSIZE", -999.9) < -100.0) {
        m_graphic->addVariable("$PDSIZE", LC_DEFAULTS_PDSize, DXF_FORMAT_GC_VarName);
    }
    if (m_graphic->getVariableDouble("$JOINSTYLE", -999.9) < -100.0) {
        m_graphic->addVariable("$JOINSTYLE", 1, DXF_FORMAT_GC_JoinStyle);
    }
    if( m_graphic->getVariableDouble("$ENDCAPS", -999.9) < -100.0) {
        m_graphic->addVariable("$ENDCAPS", 1, DXF_FORMAT_GC_Endcaps);
    }

    QString acadver = m_versionStr = m_graphic->getVariableString("$ACADVER", "");
    acadver.replace(QRegularExpression("[a-zA-Z]"), "");
    bool ok;
    m_version=acadver.toInt(&ok);
    if (!ok) {
        m_version = 1021;
    }

    //detect if dxf lib are a old dxflib or libdxfrw < 0.5.4 (used to correct mtext alignment)
    m_oldMText = false;
    m_isLibDxfRw = false;
    m_libDxfRwVersion = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    auto option = Qt::SkipEmptyParts;
#else
    auto option = QString::SkipEmptyParts;
#endif
    QStringList commentList = QString::fromStdString( data->getComments()).split('\n', option);
    for( auto commentLine: commentList) {

        QStringList commentWords = commentLine.split(' ', option);
        if( 0 < commentWords.size()) {
            if( "dxflib" == commentWords.at(0)) {
                m_oldMText = true;
                break;
            } else if( "dxfrw" == commentWords.at(0)) {
                QStringList libVersionList = commentWords.at(1).split('.', option);
                if( 2 < libVersionList.size()) {
                    m_isLibDxfRw = true;
                    m_libDxfRwVersion = LIBDXFRW_VERSION( libVersionList.at(0).toInt(),
                                                        libVersionList.at(1).toInt(),
                                                        libVersionList.at(2).toInt() );
                    if( m_libDxfRwVersion < LIBDXFRW_VERSION( 0, 5, 4)) {
                        m_oldMText = true;
                    }
                }
                break;
            }
        }
    }
}

/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the DXF file that will be written.
 */
bool RS_FilterDXFRW::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType type) {
    RS_DEBUG->print("RS_FilterDXFDW::fileExport: exporting file '%s'...",(const char*)QFile::encodeName(file));
    RS_DEBUG->print("RS_FilterDXFDW::fileExport: file type '%d'", (int)type);

    this->m_graphic = &g;

    // check if we can write to that directory:
#ifndef Q_OS_WIN

    QString path = QFileInfo(file).absolutePath();
    if (QFileInfo(path).isWritable()==false) {
        RS_DEBUG->print("RS_FilterDXFRW::fileExport: can't write file: "
                        "no permission");
        return false;
    }
    //
#endif

    // set version for DXF filter:
    m_exactColor = false;
    DRW::Version exportVersion;
    if (type==RS2::FormatDXFRW12) {
        exportVersion = DRW::AC1009;
        m_version = 1009;
    } else if (type==RS2::FormatDXFRW14) {
        exportVersion = DRW::AC1014;
        m_version = 1014;
    } else if (type==RS2::FormatDXFRW2000) {
        exportVersion = DRW::AC1015;
        m_version = 1015;
    } else if (type==RS2::FormatDXFRW2004) {
        exportVersion = DRW::AC1018;
        m_version = 1018;
        m_exactColor = true;
    } else if (type==RS2::FormatDXFRW){
        exportVersion = DRW::AC1021;
        m_version = 1021;
        m_exactColor = true;
    } else {
        exportVersion = DRW::AC1032;
        m_version = 1032;
        m_exactColor = true;
    }
    /**
     * fixme - sand - files - RESTORE!!! Under win, encodeName() prevents using unicode file names!!! Due to that, blocks/files may be saved incorrectly if name is localized
     */
    m_dxfW = new dxfRW(QFile::encodeName(file));
    // fixme - sand - save to binary format enabling/disabling!!
    bool binary = false;

//    bool success = m_dxfW->write(this, exportVersion, false); //ascii
    bool success = m_dxfW->write(this, exportVersion, binary); //binary
    delete m_dxfW;

    if (!success) {
        RS_DEBUG->print("RS_FilterDXFDW::fileExport: can't write file");
        return false;
    }
/*RLZ pte*/
/*    RS_DEBUG->print("writing tables...");
    dw->sectionTables();
    // VPORT:
    dxf.writeVPort(*dw);
    dw->tableEnd();

    // VIEW:
    RS_DEBUG->print("writing views...");
    dxf.writeView(*dw);

    // UCS:
    RS_DEBUG->print("writing ucs...");
    dxf.writeUcs(*dw);

    // Appid:
    RS_DEBUG->print("writing appid...");
    dw->tableAppid(1);
    writeAppid(*dw, "ACAD");
    dw->tableEnd();
*/
    return success;
}

/**
 * Prepare unnamed blocks.
 */
void RS_FilterDXFRW::prepareBlocks() {
    RS_Block *blk;
    int dimNum = 0, hatchNum= 0;
    QString prefix, sufix;

    //check for existing *D?? or  *U??
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        prefix = blk->getName().left(2).toUpper();
        sufix = blk->getName().mid(2);
        if (prefix == "*D") {
            if (sufix.toInt() > dimNum) dimNum = sufix.toInt();
        } else if (prefix == "*U") {
            if (sufix.toInt() > hatchNum) hatchNum = sufix.toInt();
        }
    }
    //Add a name to each dimension, in dxfR12 also for hatches
    for(RS_Entity* e: lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (!(e->getFlag(RS2::FlagUndone)) ) {
            switch (e->rtti()) {
            case RS2::EntityDimLinear:
            case RS2::EntityDimOrdinate:
            case RS2::EntityDimAligned:
            case RS2::EntityDimAngular:
            case RS2::EntityDimRadial:
            case RS2::EntityDimDiametric:
            case RS2::EntityDimLeader:
                prefix = "*D" + QString::number(++dimNum);
                m_noNameBlock[e] = prefix;
                break;
            case RS2::EntityHatch:
                if (m_version==1009) {
                    if ( !static_cast<RS_Hatch*>(e)->isSolid() ) {
                        prefix = "*U" + QString::number(++hatchNum);
                        m_noNameBlock[e] = prefix;
                    }
                }
                break;
            default:
                break;
            }//end switch
        }//end if !RS2::FlagUndone
    }
}

/**
 * Writes block records (just the name, not the entities in it).
 */
void RS_FilterDXFRW::writeBlockRecords(){
    //first prepare and send unnamed blocks, the while loop can be omitted for R12
    prepareBlocks();
    QHash<RS_Entity*, QString>::const_iterator it = m_noNameBlock.constBegin();
    while (it != m_noNameBlock.constEnd()) {
        m_dxfW->writeBlockRecord(it.value().toStdString());
        ++it;
    }

    //next send "normal" blocks
    RS_Block *blk;
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        if (!blk->isUndone()){
            RS_DEBUG->print("writing block record: %s", (const char*)blk->getName().toLocal8Bit());
            m_dxfW->writeBlockRecord(blk->getName().toUtf8().data());
        }
    }
}

/**
 * Writes blocks.
 */
void RS_FilterDXFRW::writeBlocks() {
    RS_Block *blk;

    //write unnamed blocks
    QHash<RS_Entity*, QString>::const_iterator it = m_noNameBlock.constBegin();
    while (it != m_noNameBlock.constEnd()) {
        DRW_Block block;
        block.name = it.value().toStdString();
        block.basePoint.x = 0.0;
        block.basePoint.y = 0.0;
        block.basePoint.z = 0.0;
        block.flags = 1;//flag for unnamed block
        m_dxfW->writeBlock(&block);
        RS_EntityContainer *ct = (RS_EntityContainer *)it.key();
        for(RS_Entity* e: lc::LC_ContainerTraverser{*ct, RS2::ResolveNone}.entities()) {
            if ( !(e->getFlag(RS2::FlagUndone)) ) {
                writeEntity(e);
            }
        }
        ++it;
    }

    //next write "normal" blocks
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        if (!blk->isUndone()) {
            RS_DEBUG->print("writing block: %s", (const char*)blk->getName().toLocal8Bit());

            DRW_Block block;
            block.name = blk->getName().toUtf8().data();
            block.basePoint.x = blk->getBasePoint().x;
            block.basePoint.y = blk->getBasePoint().y;
            block.basePoint.z = blk->getBasePoint().z;
            m_dxfW->writeBlock(&block);
            for(RS_Entity* e: lc::LC_ContainerTraverser{*blk, RS2::ResolveNone}.entities()) {
                if ( !(e->getFlag(RS2::FlagUndone)) ) {
                    writeEntity(e);
                }
            }
        }
    }
}

void RS_FilterDXFRW::writeHeader(DRW_Header& data){
    RS_Vector v;
/*TODO $ISOMETRICGRID == $SNAPSTYLE and "GRID on/off" not handled because is part of
 active vport to save is required read/write VPORT table */
    QHash<QString, RS_Variable>vars = m_graphic->getVariableDict();
    QHash<QString, RS_Variable>::iterator it = vars.begin();
    if (!vars.contains ( "$DWGCODEPAGE" )) {
//RLZ: TODO verify this
        m_codePage = RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit());
//        RS_Variable v( QString(RS_SYSTEM->localeToISO(QLocale::system().name().toLocal8Bit())),0 );
        vars.insert(QString("$DWGCODEPAGE"), RS_Variable(m_codePage, 0) );
    }

    while (it != vars.end()) {
        auto value = it.value();
        int code = value.getCode();
        auto key = it.key().toStdString();
        switch (value.getType()) {
            case RS2::VariableInt:
                data.addInt(key, value.getInt(), code);
                break;
            case RS2::VariableDouble:
                data.addDouble(key, value.getDouble(), code);
                break;
            case RS2::VariableString:
                data.addStr(key, value.getString().toUtf8().data(), code);
                break;
            case RS2::VariableVector:
                v = value.getVector();
                data.addCoord(key, DRW_Coord(v.x, v.y, v.z), code);
                break;
            default:
                break;
        }
        ++it;
    }
    v = m_graphic->getMin();
    v = m_graphic->getMax();
    data.addCoord("$EXTMIN", DRW_Coord(v.x, v.y, 0.0), 0);
    data.addCoord("$EXTMAX", DRW_Coord(v.x, v.y, 0.0), 0);

    //when saving a block, there is no active layer. ignore it to avoid crash
    if(m_graphic->getActiveLayer()==0) {
        return;
    }
    data.addStr("$CLAYER", (m_graphic->getActiveLayer()->getName()).toUtf8().data(), 8);

    QHash<QString, RS_Variable> customVars = m_graphic->getCustomProperties();

    QHashIterator<QString,RS_Variable> customVar(customVars);
    while (customVar.hasNext()) {
        customVar.next();
        auto val = customVar.value().getString();
        if (!val.isEmpty()) {
            auto key = customVar.key().toStdString();
            data.customVars[key] = new DRW_Variant(1, val.toStdString());
        }
    }
}

void RS_FilterDXFRW::writeLType(const UTF8STRING& lTypeName, const UTF8STRING& ltDescription, int ltSize,
                                double ltLength, const std::vector<double>& ltPath) {
    DRW_LType ltype;
    ltype.updateValues(lTypeName, ltDescription, ltSize, ltLength, ltPath);
    m_dxfW->writeLineType(&ltype);
}

void RS_FilterDXFRW::writeLTypes(){
    writeLType("CONTINUOUS", "Solid line", 0, 0, {});
    writeLType("ByLayer", "", 0, 0, {});
    writeLType("ByBlock", "", 0, 0, {});
    writeLType("DOT", "Dot . . . . . . . . . . . . . . . . . . . . . .",
                       2, 6.35, {0.0, -6.35});
    writeLType("DOTTINY", "Dot (.15x) .....................................",
                       2, 0.9525, {0.0, -0.9525});
    writeLType("DOT2", "Dot (.5x) .....................................",
                           2, 3.175, {0.0, -3.175});
    writeLType("DOTX2", "Dot (2x) .  .  .  .  .  .  .  .  .  .  .  .  .",
                          2, 12.7, {0.0, -12.7});
    writeLType("DASHED", "Dashed _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                         2, 19.05, {12.7, -6.35});
    writeLType("DASHEDTINY", "Dashed (.15x) _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                        2, 2.8575, {1.905, -0.9525});
    writeLType("DASHED2", "Dashed (.5x) _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _",
                      2, 9.525, {6.35, -3.175});
    writeLType("DASHEDX2", "Dashed (2x) ____  ____  ____  ____  ____  ___",
                      2, 38.1, {25.4, -12.7});
    writeLType("DASHDOT", "Dash dot __ . __ . __ . __ . __ . __ . __ . __",
                     4, 25.4, {12.7, -6.35, 0.0, -6.35});
    writeLType("DASHDOTTINY", "Dash dot (.15x) _._._._._._._._._._._._._._._.",
                    4, 3.81, {1.905, -0.9525, 0.0, -0.9525});
    writeLType("DASHDOT2", "Dash dot (.5x) _._._._._._._._._._._._._._._.",
                    4, 12.7, {6.35, -3.175, 0.0, -3.175});
    writeLType("DASHDOTX2", "Dash dot (2x) ____  .  ____  .  ____  .  ___",
                    4, 50.8, {25.4, -12.7, 0.0, -12.7});
    writeLType("DIVIDE", "Divide ____ . . ____ . . ____ . . ____ . . ____",
                    6, 31.75, {12.7, -6.35, 0.0, -6.35, 0.0, -6.35});
    writeLType("DIVIDETINY", "Divide (.15x) __..__..__..__..__..__..__..__.._",
                    6, 4.7625, {1.905, -0.9525, 0.0, -0.9525, 0.0, -0.9525});
    writeLType("DIVIDE2", "Divide (.5x) __..__..__..__..__..__..__..__.._",
                   6, 15.875, {6.35, -3.175, 0.0, -3.175, 0.0, -3.175});
    writeLType("DIVIDEX2", "Divide (2x) ________  .  .  ________  .  .  _",
                   6, 63.5, {25.4, -12.7, 0.0, -12.7, 0.0, -12.7});
    writeLType("BORDER", "Border __ __ . __ __ . __ __ . __ __ . __ __ .",
                   6, 44.45, {12.7, -6.35, 12.7, -6.35, 0.0, -6.35});
    writeLType("BORDERTINY", "Border (.15x) __.__.__.__.__.__.__.__.__.__.__.",
               6, 6.6675, {1.905, -0.9525, 1.905, -0.9525, 0.0, -0.9525});
    writeLType("BORDER2", "Border (.5x) __.__.__.__.__.__.__.__.__.__.__.",
               6, 22.225, {6.35, -3.175, 6.35, -3.175, 0.0, -3.175});
    writeLType("BORDERX2", "Border (2x) ____  ____  .  ____  ____  .  ___",
              6, 88.9, {25.4, -12.7, 25.4, -12.7, 0.0, -12.7});
    writeLType("CENTER", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____",
              4, 50.8, {31.75, -6.35, 6.35, -6.35});
    writeLType("CENTERTINY", "Center (.15x) ___ _ ___ _ ___ _ ___ _ ___ _ ___",
                4, 7.62, {4.7625, -0.9525, 0.9525, -0.9525});
    writeLType("CENTER2", "Center (.5x) ___ _ ___ _ ___ _ ___ _ ___ _ ___",
                4, 28.575, {19.05, -3.175, 3.175, -3.175});
    writeLType("CENTERX2", "Center (2x) ________  __  ________  __  _____",
               4, 101.6, {63.5, -12.7, 12.7, -12.7});
}

void RS_FilterDXFRW::writeLayers(){
    DRW_Layer lay;
    RS_LayerList* ll = m_graphic->getLayerList();
    int exact_rgb;
    for (unsigned int i = 0; i < ll->count(); i++) {
        lay.reset();
        RS_Layer* l = ll->at(i);
        RS_Pen pen = l->getPen();
        lay.name = l->getName().toUtf8().data();
        lay.color = colorToNumber(pen.getColor(), &exact_rgb);
        lay.color24 = exact_rgb;
        lay.lWeight = widthToNumber(pen.getWidth());
        lay.lineType = lineTypeToName(pen.getLineType()).toStdString();
        lay.flags = l->isFrozen() ? 0x01 : 0x00;
        if (l->isLocked()) {
            lay.flags |=0x04;
        }
        lay.plotF = l->isPrint();
        if( l->isConstruction()) {
            lay.extData.push_back(new DRW_Variant(1001, "LibreCad"));
            lay.extData.push_back(new DRW_Variant(1070, 1));
            // RS_DEBUG->print(RS_Debug::D_WARNING, "RS_FilterDXF::writeLayers: layer %s saved as construction layer", lay.name.c_str());
        }
        m_dxfW->writeLayer(&lay);
    }
}

void RS_FilterDXFRW::writeUCSs() {
    LC_UCSList* vl = m_graphic->getUCSList();
    DRW_UCS ucs;
    for (unsigned int i = 1; i < vl->count(); i++) {
        ucs.reset();

        LC_UCS* u = vl->at(i);
        if (u->isTemporary()){ // temporary ucs without name are not persistent
            continue;
        }
        ucs.name = u->getName().toUtf8().data();
        ucs.origin.x = u->getOrigin().x;
        ucs.origin.y = u->getOrigin().y;
        ucs.origin.z = u->getOrigin().z;

        ucs.xAxisDirection.x = u->getXAxis().x;
        ucs.xAxisDirection.y = u->getXAxis().y;
        ucs.xAxisDirection.z = u->getXAxis().z;

        ucs.yAxisDirection.x = u->getYAxis().x;
        ucs.yAxisDirection.y = u->getYAxis().y;
        ucs.yAxisDirection.z = u->getYAxis().z;

        ucs.orthoOrigin.x = u->getOrthoOrigin().x;
        ucs.orthoOrigin.y = u->getOrthoOrigin().y;
        ucs.orthoOrigin.z = u->getOrthoOrigin().z;

        ucs.orthoType = u->getOrthoType();
        ucs.elevation = u->getElevation();

        m_dxfW->writeUCS(&ucs);
    }
}

void RS_FilterDXFRW::writeViews() {
    LC_ViewList* vl = m_graphic->getViewList();
    DRW_View vie;
    for (unsigned int i = 0; i < vl->count(); i++) {
        vie.reset();
        LC_View* view = vl->at(i);
        vie.name = view->getName().toUtf8().data();
        vie.center.x = view->getCenter().x; 
        vie.center.y = view->getCenter().y; 
        vie.center.z = view->getCenter().z;

        vie.targetPoint.x = view->getTargetPoint().x;
        vie.targetPoint.y = view->getTargetPoint().y;
        vie.targetPoint.z = view->getTargetPoint().z;
        
        vie.size.x = view->getSize().x;
        vie.size.y = view->getSize().y;
        vie.size.z = view->getSize().z;        
        
        vie.frontClippingPlaneOffset = view->getFrontClippingPlaneOffset();
        vie.backClippingPlaneOffset = view->getBackClippingPlaneOffset();
        vie.lensLen = view->getLensLen();
        vie.flags = view->getFlags();
        vie.viewMode = view->getViewMode();
                
        vie.viewDirectionFromTarget.x = view->getViewDirection().x;
        vie.viewDirectionFromTarget.y = view->getViewDirection().y;
        vie.viewDirectionFromTarget.z = view->getViewDirection().z;
        
        vie.cameraPlottable = view->isCameraPlottable();
        vie.renderMode = view->getRenderMode();
        
        vie.twistAngle = view->getTwistAngle();        
        
        if (view->isHasUCS()){
            vie.hasUCS = true;
            LC_UCS *ucs = view->getUCS();
            vie.ucsOrigin.x = ucs->getOrigin().x;
            vie.ucsOrigin.y = ucs->getOrigin().y;
            vie.ucsOrigin.z = ucs->getOrigin().z;
            
            vie.ucsOrthoType = ucs->getOrthoType();
            vie.ucsElevation = ucs->getElevation();

            vie.ucsXAxis.x = ucs->getXAxis().x;
            vie.ucsXAxis.y = ucs->getXAxis().y;
            vie.ucsXAxis.z = ucs->getXAxis().z;

            vie.ucsYAxis.x = ucs->getYAxis().x;
            vie.ucsYAxis.y = ucs->getYAxis().y;
            vie.ucsYAxis.z = ucs->getYAxis().z;

            // fixme - complete - base UCS_ID and Named UCS_ID support. That's might be necessary to support views/UCS
            // created outside of LibreCAD.
            // Return to this after normal support of UCS.
//            vie.namedUCS_ID = ucs.
//            vie.baseUCS_ID = ucs.
        }
        m_dxfW->writeView(&vie);
    }
}

void RS_FilterDXFRW::writeTextstyles(){
    QHash<QString, QString> styles;
    QString sty;
    //Find fonts used by text entities in drawing
    for (RS_Entity* e : lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if (!e->isUndone()) {
            auto rtti = e->rtti();
            switch (rtti) {
                case RS2::EntityMText:
                    sty = static_cast<RS_MText*>(e)->getStyle();
                    break;
                case RS2::EntityText:
                    sty = static_cast<RS_Text*>(e)->getStyle();
                    break;
                default:
                    if (RS2::isDimensionalEntity(rtti)) {
                        auto dim = dynamic_cast<RS_Dimension*>(e);
                        if (dim != nullptr) {
                            auto styleOverride = dim->getDimStyleOverride();
                            if (styleOverride != nullptr) {
                                auto dimTextStyle = styleOverride->text()->style();
                                sty = dimTextStyle;
                            }
                        }
                    }
                    else {
                        sty.clear();
                    }
                    break;
            }
            if (!sty.isEmpty() && !styles.contains(sty)) {
                styles.insert(sty, sty);
            }
        }
    }
    //Find fonts used by text entities in blocks
    RS_Block *blk;
    for (unsigned i = 0; i < m_graphic->countBlocks(); i++) {
        blk = m_graphic->blockAt(i);
        for(RS_Entity* e: lc::LC_ContainerTraverser{*blk, RS2::ResolveNone}.entities()) {
            if (!e->isUndone()) {
                RS2::EntityType rtti = e->rtti();
                switch (rtti) {
                    case RS2::EntityMText:
                        sty = static_cast<RS_MText*>(e)->getStyle();
                        break;
                    case RS2::EntityText:
                        sty = static_cast<RS_Text*>(e)->getStyle();
                        break;
                    default:
                        if (RS2::isDimensionalEntity(rtti)) {
                            auto dim = dynamic_cast<RS_Dimension*>(e);
                            if (dim != nullptr) {
                                auto styleOverride = dim->getDimStyleOverride();
                                if (styleOverride != nullptr) {
                                    auto dimTextStyle = styleOverride->text()->style();
                                    sty = dimTextStyle;
                                }
                            }
                        }
                        else {
                            sty.clear();
                        }

                        break;
                }
                if (!sty.isEmpty() && !styles.contains(sty)) {
                    styles.insert(sty, sty);
                }
            }
        }
    }

    auto dimStyleList = m_graphic->getDimStyleList();

    for (const auto ds: *dimStyleList->getStylesList()) {
       sty = ds->text()->style();
        if (!sty.isEmpty() && !styles.contains(sty)) {
            styles.insert(sty, sty);
        }
    }

    DRW_Textstyle ts;
    QHash<QString, QString>::const_iterator it = styles.constBegin();
     while (it != styles.constEnd()) {
         ts.name = (it.key()).toStdString();
         ts.font = it.value().toStdString();
//         ts.flags;
         m_dxfW->writeTextstyle( &ts );
         ++it;
     }
}

void RS_FilterDXFRW::writeVports(){
    DRW_Vport vp;
    vp.name = "*Active";
    m_graphic->isGridOn()? vp.grid = 1 : vp.grid = 0;
    RS_Vector spacing = m_graphic->getVariableVector("$GRIDUNIT",RS_Vector(0.0,0.0));
    vp.gridBehavior = 3;
    vp.gridSpacing.x = spacing.x;
    vp.gridSpacing.y = spacing.y;
    vp.snapStyle = m_graphic->isIsometricGrid();
    vp.snapIsopair = m_graphic->getIsoView();
    if (vp.snapIsopair > 2) {
        vp.snapIsopair = 0;
    }
    if (fabs(spacing.x) < 1.0e-6) {
        vp.gridBehavior = 7; //auto
        vp.gridSpacing.x = 10;
    }
    if (fabs(spacing.y) < 1.0e-6) {
        vp.gridBehavior = 7; //auto
        vp.gridSpacing.y = 10;
    }
    RS_GraphicView *gv = m_graphic->getGraphicView();
    if (gv) {
        LC_GraphicViewport *viewport = gv->getViewPort();
        RS_Vector fac = viewport->getFactor();
        vp.height = gv->getHeight() / fac.y;
        vp.ratio = (double) gv->getWidth() / (double) gv->getHeight();
        vp.center.x = (gv->getWidth() - viewport->getOffsetX()) / (fac.x * 2.0);
        vp.center.y = (gv->getHeight() - viewport->getOffsetY()) / (fac.y * 2.0);
    }
    m_dxfW->writeVport(&vp);
}

void RS_FilterDXFRW::writeDimstyles(){
    LC_DimStylesList* dimStylesList = m_graphic->getDimStyleList();
    auto stylesList = dimStylesList->getStylesList();
    for (auto ds: *stylesList) {
        DRW_Dimstyle dst;
        prepareDRWDimStyle(dst, ds);
        m_dxfW->writeDimstyle(&dst);
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleZerosSuppression(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto zeros = ds->zerosSuppression();
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMZIN)) {
        d.add("$DIMZIN", 78, zeros->linearRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMAZIN)) {
        d.add("$DIMAZIN", 79, zeros->angularRaw());
    }

    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMTZIN)) {
        d.add("$DIMTZIN", 284, zeros->toleranceRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTZ)) {
        d.add("$DIMALTZ", 285, zeros->altLinearRaw());
    }
    if (zeros->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTTZ)) {
        d.add("$DIMALTTZ", 286, zeros->altToleranceRaw());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleArrows(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto arrow = ds->arrowhead();
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) {
        QString blockName = arrow->sameBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                d.add("_$DIMBLK", 342, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK", 5, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) {
        QString blockName = arrow->arrowHeadBlockNameFirst();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                d.add("_$DIMBLK1", 343, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK1", 6, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) {
        QString blockName = arrow->arrowHeadBlockNameSecond();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                d.add("_$DIMBLK2", 344, toHexStr(blkHandle).toStdString());
            }
            d.add("$DIMBLK2", 7, blkName);
        }
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMASZ)) {
        d.add("$DIMASZ", 41, arrow->size());
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMTSZ)) {
        d.add("$DIMTSZ", 142, arrow->tickSize());
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH)) {
        d.add("$DIMSAH", 173, arrow->isUseSeparateArrowHeads()? 1 : 0);
    }
    if (arrow->checkModifyState(LC_DimStyle::Arrowhead::$DIMSOXD)) {
        d.add("$DIMSOXD", 175, arrow->suppression());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleScaling(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto scale = ds->scaling();
    if (scale->checkModifyState(LC_DimStyle::Scaling::$DIMSCALE)) {
        d.add("$DIMSCALE", 40, scale->scale());
    }
    if (scale->checkModifyState(LC_DimStyle::Scaling::$DIMLFAC)) {
        d.add("$DIMLFAC", 144, scale->linearFactor());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleExtLine(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto extLine = ds->extensionLine();
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXO)) {
        d.add("$DIMEXO", 42, extLine->distanceFromOriginPoint());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXE)) {
        d.add("$DIMEXE", 44, extLine->distanceBeyondDimLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXL)) {
        d.add("$DIMFXL", 49, extLine->fixedLength());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXLON)) {
        d.add("$DIMFXLON", 290, extLine->hasFixedLength() ? 1: 0);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLWE)) {
        auto lineWidth = extLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        d.add("$DIMLWE", 372, lw);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMCLRE)) {
        auto lineColor = extLine->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRE", 177, colNum);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE1)) {
        d.add("$DIMSE1", 75, extLine->suppressFirstLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE2)) {
        d.add("$DIMSE2", 76, extLine->suppressSecondLine());
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX1)) {
        int lineTypeHandle = findLineTypeHandleToWrite(extLine->lineTypeFirstRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTEX1", 347, handleStr.toStdString());
        }
        // auto lineType = extLine->lineTypeFirstRaw().toStdString();
        // d.add("$DIMLTEX1", 347, lineType);
    }
    if (extLine->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX2)) {
        int lineTypeHandle = findLineTypeHandleToWrite(extLine->lineTypeSecondRaw());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTEX2", 348, handleStr.toStdString());
        }
        // auto lineType = extLine->lineTypeFirstRaw().toStdString();
        // d.add("$DIMLTEX2", 348, lineType);
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleDimLine(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto dimLine = ds->dimensionLine();
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLWD)) {
        auto lineWidth = dimLine->lineWidth();
        int lw = RS2::lineWidth2dxfInt(lineWidth);
        d.add("$DIMLWD", 371, lw);
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLE)) {
        d.add("$DIMDLE", 46, dimLine->distanceBeyondExtLinesForObliqueStroke());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLI)) {
        d.add("$DIMDLI", 43, dimLine->baseLineDimLinesSpacing());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMGAP)) {
        d.add("$DIMGAP", 147, dimLine->lineGap());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMCLRD)) {
        auto lineColor = dimLine->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRD", 176, colNum);
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD1)) {
        d.add("$DIMSD1", 281, dimLine->suppressFirstLine());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD2)) {
        d.add("$DIMSD2", 281, dimLine->suppressSecondLine());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMTOFL)) {
        d.add("$DIMTOFL", 172, dimLine->drawPolicyForOutsideText());
    }
    if (dimLine->checkModifyState(LC_DimStyle::DimensionLine::$DIMLTYPE)) {
        // auto value = dimLine->lineType();
        // auto lineType = dimLine->lineTypeName().toStdString();
        int lineTypeHandle = findLineTypeHandleToWrite(dimLine->lineTypeName());
        if (lineTypeHandle > 0) {
            auto handleStr = toHexStr(lineTypeHandle);
            d.add("$DIMLTYPE", 345, handleStr.toStdString());
        }
    }
}

int RS_FilterDXFRW::findLineTypeHandleToWrite(const QString& name) const {
    std::string lineName = name.toUpper().toStdString();
    for (auto p: m_dxfW->getWritingContext()->lineTypesMap) {
        if (p.first.compare(lineName) == 0) {
            return p.second;
        }
    }
    return -1;
}

void RS_FilterDXFRW::prepareDRWDimStyleText(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto text = ds->text();
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXT)) {
        d.add("$DIMTXT", 140, text->height());
    }

    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXSTY)) {
        auto styleName = text->style().toStdString();
        int styleHandle = m_dxfW->getTextStyleHandle(styleName);
        if(styleHandle > 0) {
            d.add("$DIMTXSTY", 340, toHexStr(styleHandle).toStdString());
        }
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTOH)) {
        d.add("$DIMTOH", 74, text->orientationOutside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIH)) {
        d.add("$DIMTIH", 73, text->orientationInside());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMJUST)) {
        d.add("$DIMJUST", 280, text->horizontalPositioning());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMCLRT)) {
        auto lineColor = text->color();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMCLRT", 178, colNum);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTAD)) {
        d.add("$DIMTAD", 77, text->verticalPositioning());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTIX)) {
        d.add("$DIMTIX", 174, text->extLinesRelativePlacement());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILL)) {
        d.add("$DIMTFILL", 69, text->backgroundFillMode());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTFILLCLR)) {
        auto lineColor = text->explicitBackgroundFillColor();
        int colRGB;
        int colNum = colorToNumber(lineColor, &colRGB);
        d.add("$DIMTFILLCLR", 70, colNum);
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTXTDIRECTION)) {
        d.add("$DIMTXTDIRECTION", 292, text->readingDirection());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTVP)) {
        d.add("$DIMTVP", 145, text->verticalDistanceToDimLine());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMUPT)) {
        d.add("$DIMUPT", 288, text->cursorControlPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMTMOVE)) {
        d.add("$DIMTMOVE", 279, text->positionMovementPolicy());
    }
    if (text->checkModifyState(LC_DimStyle::Text::$DIMATFIT)) {
        d.add("$DIMATFIT", 289, text->unsufficientSpacePolicy());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleLinearFormat(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto linear = ds->linearFormat();
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMLUNIT)) {
        d.add("$DIMLUNIT", 277, linear->formatRaw());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMSEP)) {
        d.add("$DIMDSEP", 278, linear->decimalFormatSeparatorChar());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMDEC)) {
        d.add("$DIMDEC", 271, linear->decimalPlaces());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMPOST)) {
        d.add("$DIMPOST", 3, linear->prefixOrSuffix().toStdString());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALT)) {
        d.add("$DIMALT", 170, linear->alternateUnits());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTU)) {
        d.add("$DIMALTU", 273, linear->altFormatRaw());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTD)) {
        d.add("$DIMALTD", 171, linear->altDecimalPlaces());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTF)) {
        d.add("$DIMALTF", 143, linear->altUnitsMultiplier());
    }
    if (linear->checkModifyState(LC_DimStyle::LinearFormat::$DIMAPOST)) {
        d.add("$DIMAPOST", 4, linear->altPrefixOrSuffix().toStdString());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleFractions(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto fraction = ds->fractions();
    if (fraction->checkModifyState(LC_DimStyle::Fractions::$DIMFRAC)) {
        d.add("$DIMFRAC", 276, fraction->style());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleAngularFormat(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto angular = ds->angularFormat();
    if (angular->checkModifyState(LC_DimStyle::AngularFormat::$DIMAUNIT)) {
        d.add("$DIMAUNIT", 275, angular->format());
    }
    if (angular->checkModifyState(LC_DimStyle::AngularFormat::$DIMADEC)) {
        d.add("$DIMADEC", 179, angular->decimalPlaces());
    }

    auto round = ds->roundOff();
    if (round->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMRND)) {
        d.add("$DIMRND", 45, round->roundTo());
    }
    if (round->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMALTRND)) {
        d.add("$DIMALTRND", 148, round->altRoundTo());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleRadial(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto radial = ds->radial();
    if (radial->checkModifyState(LC_DimStyle::Radial::$DIMCEN)) {
        d.add("$DIMCEN", 141, radial->centerCenterMarkOrLineSize());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleTolerance(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto tolerance = ds->latteralTolerance();
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTDEC)) {
        d.add("$DIMTDEC", 272, tolerance->decimalPlaces());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMALTTD)) {
        d.add("$DIMALTTD", 274, tolerance->decimalPlacesAltDim());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOL)) {
        d.add("$DIMTOL", 71, tolerance->isAppendTolerancesToDimText() ? 1 : 0);
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOLJ)) {
        d.add("$DIMTOLJ", 283, tolerance->verticalJustification());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {
        d.add("$DIMTM", 48, tolerance->lowerToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {
        d.add("$DIMTP", 47, tolerance->upperToleranceLimit());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTFAC)) {
        d.add("$DIMTFAC", 146, tolerance->heightScaleFactorToDimText());
    }
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMLIM)) {
        d.add("$DIMLIM", 72, tolerance->isLimitsGeneratedAsDefaultText() ? 1 : 0);
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleArc(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto arc = ds->arc();
    if (arc->checkModifyState(LC_DimStyle::Arc::$DIMARCSYM)) {
        d.add("$DIMARCSYM", 90, arc->arcSymbolPosition());
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleLeader(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto leader = ds->leader();
    if (leader->checkModifyState(LC_DimStyle::Leader::$DIMLDRBLK)) {
        QString blockName = leader->arrowBlockName();
        if (!blockName.isEmpty()) {
            auto blkName = blockName.toStdString();
            int blkHandle = m_dxfW->getBlockRecordHandleToWrite(blkName);
            if(blkHandle > 0) {
                d.add("_$DIMLDRBLK", 341, toHexStr(blkHandle).toStdString());
            }
        }
    }
}

void RS_FilterDXFRW::prepareDRWDimStyleExtData(DRW_Dimstyle& d, LC_DimStyle* ds) {
    auto tolerance = ds->latteralTolerance();
    if (tolerance->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTALN)) {
        d.extData.push_back(new DRW_Variant(1001, "ACAD_DSTYLE_DIMTALN"));
        d.extData.push_back(new DRW_Variant(1070, 392));
        d.extData.push_back(new DRW_Variant(1070, tolerance->adjustment()));
    }
    // todo - add support of ACAD_DIMSTYLE_DIMBREAK
    // todo - add support of ACAD_DIMSTYLE_DIMJAG
}

void RS_FilterDXFRW::prepareDRWDimStyle(DRW_Dimstyle &d, LC_DimStyle* ds) {
    d.name = ds->getName().toStdString();

    auto savedMode = ds->getModifyCheckMode();
    if (ds->isBaseStyle()){
        // base styles are written completely, regardless of fields modification state!
        ds->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
    }

    // in AutoCAD, dimstyle that is specific for the type of dimension will contain only
    // values that are really overriden for the type. Other properties will be
    // extracted from the base style.
    // therefore, have need to check for properties that are modified and write only
    // one that has modified flag set...

    prepareDRWDimStyleArrows(d, ds);
    prepareDRWDimStyleScaling(d, ds);
    prepareDRWDimStyleExtLine(d, ds);
    prepareDRWDimStyleDimLine(d, ds);
    prepareDRWDimStyleText(d, ds);
    prepareDRWDimStyleZerosSuppression(d, ds);
    prepareDRWDimStyleLinearFormat(d, ds);
    prepareDRWDimStyleAngularFormat(d, ds);
    prepareDRWDimStyleFractions(d, ds);
    prepareDRWDimStyleRadial(d, ds);
    prepareDRWDimStyleTolerance(d, ds);
    prepareDRWDimStyleArc(d, ds);
    prepareDRWDimStyleLeader(d, ds);
    prepareDRWDimStyleExtData(d, ds);
    // result->setDimunit(s.dimunit); // $DIMLUNIT

    ds->setModifyCheckMode(savedMode); // restore modification flags

    // fixme - return to this later, move to MLeaderStyle
    // auto mleader = ds->mleader();
    // var = s.get("$MLEADERSCALE");
    // if (var != nullptr) {
    //     mleaderStyle->setScale(var->d_val());
    // }
}

void RS_FilterDXFRW::writeObjects() {
    /* PLOTSETTINGS */
    DRW_PlotSettings ps;
    QString horizXvert = QString("%1x%2").arg(m_graphic->getPagesNumHoriz())
                                         .arg(m_graphic->getPagesNumVert());
    ps.plotViewName = horizXvert.toStdString();
    ps.marginLeft = m_graphic->getMarginLeft();
    ps.marginTop = m_graphic->getMarginTop();
    ps.marginRight = m_graphic->getMarginRight();
    ps.marginBottom = m_graphic->getMarginBottom();
    m_dxfW->writePlotSettings(&ps);
}

void RS_FilterDXFRW::writeAppId(){
    DRW_AppId ai;
    ai.name ="LibreCad";
    m_dxfW->writeAppId(&ai);

    ai.name ="ACAD_DSTYLE_DIMTALN";
    m_dxfW->writeAppId(&ai);

    ai.name ="ACAD_DSTYLE_DIMJAG_POSITION";
    m_dxfW->writeAppId(&ai);

    ai.name ="ACAD_DSTYLE_DIMJAG";
    m_dxfW->writeAppId(&ai);

    // ACAD_DSTYLE_DIMJAG
    // fixme - sand - probably we can add version there, check format
}

void RS_FilterDXFRW::writeEntities(){
    for(RS_Entity* e: lc::LC_ContainerTraverser{*m_graphic, RS2::ResolveNone}.entities()) {
        if ( !(e->getFlag(RS2::FlagUndone)) ) {
            writeEntity(e);
        }
    }
}

void RS_FilterDXFRW::writeEntity(RS_Entity* e){
    switch (e->rtti()) {
    case RS2::EntityPoint:
        writePoint(static_cast<RS_Point*>(e));
        break;
    case RS2::EntityLine:
        writeLine(static_cast<RS_Line*>(e));
        break;
    case RS2::EntityCircle:
        writeCircle(static_cast<RS_Circle*>(e));
        break;
    case RS2::EntityArc:
        writeArc(static_cast<RS_Arc*>(e));
        break;
    case RS2::EntitySolid:
        writeSolid(static_cast<RS_Solid*>(e));
        break;
    case RS2::EntityEllipse:
        writeEllipse(static_cast<RS_Ellipse*>(e));
        break;
    case RS2::EntityPolyline:
        writeLWPolyline(static_cast<RS_Polyline*>(e));
        break;
    case RS2::EntitySpline:
        writeSpline(static_cast<RS_Spline*>(e));
        break;
    case RS2::EntitySplinePoints:
    case RS2::EntityParabola:
        writeSplinePoints(static_cast<LC_SplinePoints*>(e));
        break;
//    case RS2::EntityVertex:
//        break;
    case RS2::EntityInsert:
        writeInsert(static_cast<RS_Insert*>(e));
        break;
    case RS2::EntityMText:
        writeMText(static_cast<RS_MText*>(e));
        break;
    case RS2::EntityText:
        writeText(static_cast<RS_Text*>(e));
        break;
    case RS2::EntityDimLinear:
    case RS2::EntityDimOrdinate:
    case RS2::EntityDimAligned:
    case RS2::EntityDimAngular:
    case RS2::EntityDimRadial:
    case RS2::EntityDimDiametric:
        writeDimension(static_cast<RS_Dimension*>(e));
        break;
    case RS2::EntityDimLeader:
        writeLeader(static_cast<RS_Leader*>(e));
        break;
    case RS2::EntityHatch:
        writeHatch(static_cast<RS_Hatch*>(e));
        break;
    case RS2::EntityImage:
        writeImage(static_cast<RS_Image*>(e));
        break;
    default:
        break;
    }
}

/**
 * Writes the given Point entity to the file.
 */
void RS_FilterDXFRW::writePoint(RS_Point* p) {
    DRW_Point point;
    getEntityAttributes(&point, p);
    point.basePoint.x = p->getStartpoint().x;
    point.basePoint.y = p->getStartpoint().y;
    m_dxfW->writePoint(&point);
}

/**
 * Writes the given Line( entity to the file.
 */
void RS_FilterDXFRW::writeLine(RS_Line* l) {
    DRW_Line line;
    getEntityAttributes(&line, l);
    line.basePoint.x = l->getStartpoint().x;
    line.basePoint.y = l->getStartpoint().y;
    line.secPoint.x = l->getEndpoint().x;
    line.secPoint.y = l->getEndpoint().y;
    m_dxfW->writeLine(&line);
}

/**
 * Writes the given circle entity to the file.
 */
void RS_FilterDXFRW::writeCircle(RS_Circle* c) {
    DRW_Circle circle;
    getEntityAttributes(&circle, c);
    circle.basePoint.x = c->getCenter().x;
    circle.basePoint.y = c->getCenter().y;
    circle.radious = c->getRadius();
    m_dxfW->writeCircle(&circle);
}

/**
 * Writes the given arc entity to the file.
 */
void RS_FilterDXFRW::writeArc(RS_Arc* a) {
    DRW_Arc arc;
    getEntityAttributes(&arc, a);
    arc.basePoint.x = a->getCenter().x;
    arc.basePoint.y = a->getCenter().y;
    arc.radious = a->getRadius();
    if (a->isReversed()) {
        arc.staangle = a->getAngle2();
        arc.endangle = a->getAngle1();
    } else {
        arc.staangle = a->getAngle1();
        arc.endangle = a->getAngle2();
    }
    m_dxfW->writeArc(&arc);
}

/**
 * Writes the given polyline entity to the file as lwpolyline.
 */
void RS_FilterDXFRW::writeLWPolyline(RS_Polyline* l) {
    //skip if are empty polyline
    if (l->isEmpty()) {
        return;
    }
    // version 12 are old style polyline
    if (m_version == 1009) {
        writePolyline(l);
        return;
    }
    bool has_ellipse = false;
    for (RS_Entity* e=l->firstEntity(RS2::ResolveNone); e; e=l->nextEntity(RS2::ResolveNone)) {
        if (e->rtti() == RS2::EntityEllipse) {
            has_ellipse = true;
            break;
        }
    }
    if (has_ellipse) {
        writePolyline(l);
        return;
    }
    DRW_LWPolyline pol;
    RS_Entity* currEntity = nullptr;

    RS_AtomicEntity* ae = nullptr;
    double bulge = 0.0;

    lc::LC_ContainerTraverser traverser{*l, RS2::ResolveNone};
    for (RS_Entity* e = traverser.first(); e != nullptr; e = traverser.next()) {
        currEntity = e;
        // nextEntity = traverser.next();

        if (!e->isAtomic()) {
            continue;
        }
        ae = static_cast<RS_AtomicEntity*>(e);

        // Write vertex:
        if (e->rtti() == RS2::EntityArc) {
            bulge = static_cast<RS_Arc*>(e)->getBulge();
        }
        else {
            bulge = 0.0;
        }
        pol.addVertex(DRW_Vertex2D(ae->getStartpoint().x, ae->getStartpoint().y, bulge));
    }
    if (l->isClosed()) {
        pol.flags = 1;
    }
    else {
        ae = static_cast<RS_AtomicEntity*>(currEntity);
        if (ae->rtti() == RS2::EntityArc) {
            bulge = static_cast<RS_Arc*>(ae)->getBulge();
        }
        pol.addVertex(DRW_Vertex2D(ae->getEndpoint().x, ae->getEndpoint().y, bulge));
    }
    pol.vertexnum = pol.vertlist.size();
    getEntityAttributes(&pol, l);
    m_dxfW->writeLWPolyline(&pol);
}

/**
 * Writes the given polyline entity to the file (old style).
 */
void RS_FilterDXFRW::writePolyline(RS_Polyline* p) {
    if (p == nullptr)
        return;

    DRW_Polyline pol;
    if (p->isClosed()) {
        pol.flags = 1;
    }

    RS_Entity* nextEntity = nullptr;
    for (RS_Entity* e=p->firstEntity(RS2::ResolveNone); e != nullptr; e=nextEntity) {
        nextEntity = p->nextEntity(RS2::ResolveNone);

        if (!e->isAtomic()) {
            continue;
        }
        RS_AtomicEntity* ae = static_cast<RS_AtomicEntity*>(e);

        // Write vertex:
        double bulge=0.0;
        bool isElliptic = false;
        double yRadius = 0.0;
        switch(e->rtti()) {
        case RS2::EntityLine:
            break;
        case RS2::EntityArc:
            bulge = ((RS_Arc*)e)->getBulge();
            break;
        case RS2::EntityEllipse: {
            // Issue #1946: prepare to write elliptic arcs as RS_Arc
            RS_Ellipse* ellipse = static_cast<RS_Ellipse*>(e);
            auto pair = RS_Polyline::convertToArcPair(ellipse);
            std::unique_ptr<RS_Arc> arc{ pair.first };
            bulge = arc->getBulge();
            yRadius = arc->getRadius() * pair.second;
            isElliptic = true;
        }
            break;
        default:
            // should not happen: unknown entity type
            continue;
        }
        pol.addVertex( DRW_Vertex(ae->getStartpoint().x,
                                 ae->getStartpoint().y, 0.0, bulge));
        if (isElliptic) {
            // Add flag to indicate the vertex should be elliptic
            pol.vertlist.back()->extData.push_back(std::make_shared<DRW_Variant>(1001, "LibreCad"));
            // Keep to ellipse minor radius to allow elliptic creation at loading
            pol.vertlist.back()->extData.push_back(std::make_shared<DRW_Variant>(1040, yRadius));
        }
    }
    getEntityAttributes(&pol, p);
    m_dxfW->writePolyline(&pol);
}

/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXFRW::writeSpline(RS_Spline *s) {
    if (s==nullptr) {
        return;
    }

    if (s->getNumberOfControlPoints() < size_t(s->getDegree()+1)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_FilterDXF::writeSpline: "
                                           "Discarding spline: not enough control points given.");
        return;
    }

    // version 12 do not support Spline write as polyline
    if (m_version==1009) {
        DRW_Polyline pol;
        for(RS_Entity* e: lc::LC_ContainerTraverser{*s, RS2::ResolveNone}.entities()) {
            pol.addVertex( DRW_Vertex(e->getStartpoint().x,
                                     e->getStartpoint().y, 0.0, 0.0));
        }
        if (s->isClosed()) {
            pol.flags = 1;
        } else {
            pol.addVertex( DRW_Vertex(s->getEndpoint().x,s->getEndpoint().y, 0.0, 0.0));
        }
        getEntityAttributes(&pol, s);
        m_dxfW->writePolyline(&pol);
        return;
    }

    DRW_Spline sp{};

    // dxf spline group code=70
    // bit coded: 1: closed; 2: periodic; 4: rational; 8: planar; 16:linear
    sp.flags = (s->isClosed()) ? 0b1011 : 0b1000;

    // write spline control points:
    for (const RS_Vector& v: s->getUnwrappedControlPoints()) {
        sp.controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
    }
    sp.weightlist = s->getUnwrappedWeights();

    sp.ncontrol = sp.controllist.size();
    sp.degree = s->getDegree();

    // knot vector from RS_Spline
    sp.knotslist = s->getUnwrappedKnotVector();
    sp.nknots = sp.knotslist.size();

    getEntityAttributes(&sp, s);
    m_dxfW->writeSpline(&sp);
}

/**
 * Writes the given spline entity to the file.
 */
void RS_FilterDXFRW::writeSplinePoints(LC_SplinePoints *s){
	int nCtrls = s->getNumberOfControlPoints();
	auto const& cp = s->getControlPoints();

	if(nCtrls < 3){
        if(nCtrls > 1){
			DRW_Line line;
			line.basePoint.x = cp.at(0).x;
			line.basePoint.y = cp.at(0).y;
			line.secPoint.x = cp.at(1).x;
			line.secPoint.y = cp.at(1).y;
			getEntityAttributes(&line, s);
            m_dxfW->writeLine(&line);
		}
		return;
	}

	// version 12 do not support Spline write as polyline
    if(m_version == 1009){
		DRW_Polyline pol;
		auto const& sp = s->getStrokePoints();

		for(size_t i = 0; i < sp.size(); i++){
			pol.addVertex(DRW_Vertex(sp.at(i).x, sp.at(i).y, 0.0, 0.0));
		}

        if (s->isClosed()) {
            pol.flags = 1;
        }

		getEntityAttributes(&pol, s);
        m_dxfW->writePolyline(&pol);
		return;
	}

	DRW_Spline sp;
    if (s->isClosed()) {
        sp.flags = 11;
    }
    else {
        sp.flags = 8;
    }

	sp.ncontrol = nCtrls;
	sp.degree = 2;
	sp.nknots = nCtrls + 3;

    LC_SplinePointsData &data = s->getData();
    sp.nfit = data.splinePoints.size();

    auto const& fitPoints = data.splinePoints;
    
	// write spline knots:
	for(int i = 1; i <= sp.nknots; i++){
		if(i <= 3){
			sp.knotslist.push_back(0.0);
		}
		else if(i <= nCtrls){
			sp.knotslist.push_back((i - 3.0)/(nCtrls - 2.0));
		}
		else{
			sp.knotslist.push_back(1.0);
		}
	}

	// write spline control points:
    for (auto const& v : cp) {
        sp.controllist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
    }

    // fit points
    for (auto const& v : fitPoints) {
        sp.fitlist.push_back(std::make_shared<DRW_Coord>(v.x, v.y));
    }

    getEntityAttributes(&sp, s);
    m_dxfW->writeSpline(&sp);
}

/**
 * Writes the given Ellipse entity to the file.
 */
void RS_FilterDXFRW::writeEllipse(RS_Ellipse* s) {
// version 12 do not support Ellipse but are
// converted in polyline by library
    DRW_Ellipse el;
    getEntityAttributes(&el, s);
    el.basePoint.x = s->getCenter().x;
    el.basePoint.y = s->getCenter().y;
    el.secPoint.x = s->getMajorP().x;
    el.secPoint.y = s->getMajorP().y;
    el.ratio = s->getRatio();
    if (s->isReversed()) {
        el.staparam = s->getAngle2();
        el.endparam = s->getAngle1();
    } else {
        el.staparam = s->getAngle1();
        el.endparam = s->getAngle2();
    }
    m_dxfW->writeEllipse(&el);
}

/**
 * Writes the given block insert entity to the file.
 */
void RS_FilterDXFRW::writeInsert(RS_Insert* i) {
    DRW_Insert in;
    getEntityAttributes(&in, i);
    in.basePoint.x = i->getInsertionPoint().x;
    in.basePoint.y = i->getInsertionPoint().y;
    in.basePoint.z = i->getInsertionPoint().z;
    in.name = i->getName().toUtf8().data();
    in.xscale = i->getScale().x;
    in.yscale = i->getScale().y;
    in.zscale = i->getScale().z;
    in.angle = i->getAngle();
    in.colcount = i->getCols();
    in.rowcount = i->getRows();
    in.colspace = i->getSpacing().x;
    in.rowspace =i->getSpacing().y;
    m_dxfW->writeInsert(&in);
}

/**
 * Writes the given mText entity to the file.
 */
void RS_FilterDXFRW::writeMText(RS_MText* t) {
    DRW_Text *text;
    DRW_Text txt1;
    DRW_MText txt2;

    if (m_version==1009) {
        text = &txt1;
    }
    else {
        text = &txt2;
    }

    getEntityAttributes(text, t);
    text->basePoint.x = t->getInsertionPoint().x;
    text->basePoint.y = t->getInsertionPoint().y;
    text->height = t->getHeight();
    text->angle = t->getAngle()*180/M_PI;
    text->style = t->getStyle().toStdString();

    if (m_version==1009) {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->alignH =DRW_Text::HLeft;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->alignH =DRW_Text::HCenter;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->alignH = DRW_Text::HRight;
        }
        if (t->getVAlign()==RS_MTextData::VATop) {
            text->alignV = DRW_Text::VTop;
        } else if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->alignV = DRW_Text::VMiddle;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->alignV = DRW_Text::VBaseLine;
        }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList txtList = t->getText().split('\n',Qt::KeepEmptyParts);
#else
        QStringList txtList = t->getText().split('\n',QString::KeepEmptyParts);
#endif
        double dist = t->getLineSpacingFactor()*5*t->getHeight()/3;
        bool setSec = false;
        if (text->alignH != DRW_Text::HLeft || text->alignV != DRW_Text::VBaseLine) {
            text->secPoint.x = t->getInsertionPoint().x;
            text->secPoint.y = t->getInsertionPoint().y;
            setSec = true;
        }
        if (text->alignV == DRW_Text::VTop) {
            dist = dist * -1;
        }
        for (int i=0; i<txtList.size();++i){
            if (!txtList.at(i).isEmpty()) {
                text->text = toDxfString(txtList.at(i)).toUtf8().data();
				RS_Vector inc  = RS_Vector::polar(dist*i, t->getAngle()+M_PI_2);
                if (setSec) {
                    text->secPoint.x += inc.x;
                    text->secPoint.y += inc.y;
                } else {
                    text->basePoint.x += inc.x;
                    text->basePoint.y += inc.y;
                }
                m_dxfW->writeText(text);
            }
        }
    } else {
        if (t->getHAlign()==RS_MTextData::HALeft) {
            text->textgen =1;
        } else if (t->getHAlign()==RS_MTextData::HACenter) {
            text->textgen =2;
        } else if (t->getHAlign()==RS_MTextData::HARight) {
            text->textgen = 3;
        }
        if (t->getVAlign()==RS_MTextData::VAMiddle) {
            text->textgen += 3;
        } else if (t->getVAlign()==RS_MTextData::VABottom) {
            text->textgen += 6;
        }
        if (t->getDrawingDirection() == RS_MTextData::LeftToRight) {
            text->alignH = static_cast<DRW_Text::HAlign>(1);
        }
        else if (t->getDrawingDirection() == RS_MTextData::TopToBottom) {
            text->alignH = static_cast<DRW_Text::HAlign>(3);
        }
        else {
            text->alignH = static_cast<DRW_Text::HAlign>(5);
        }
		if (t->getLineSpacingStyle() == RS_MTextData::AtLeast) {
		    text->alignV = static_cast<DRW_Text::VAlign>(1);
		}
        else {
            text->alignV = static_cast<DRW_Text::VAlign>(2);
        }

        text->text = toDxfString(t->getText()).toUtf8().data();
        //        text->widthscale =t->getWidth();
        text->widthscale =t->getUsedTextWidth(); //getSize().x;
		txt2.interlin = t->getLineSpacingFactor();

        m_dxfW->writeMText(static_cast<DRW_MText*>(text));
    }
}

/**
 * Writes the given Text entity to the file.
 */
void RS_FilterDXFRW::writeText(RS_Text* t){
    DRW_Text text;

    getEntityAttributes(&text, t);
    text.basePoint.x = t->getInsertionPoint().x;
    text.basePoint.y = t->getInsertionPoint().y;
    text.height = t->getHeight();
    text.angle = t->getAngle()*180/M_PI;
    text.style = t->getStyle().toStdString();
    text.alignH =(DRW_Text::HAlign)t->getHAlign();
    text.alignV =(DRW_Text::VAlign)t->getVAlign();
    text.widthscale = t->getWidthRel();

    if (text.alignV != DRW_Text::VBaseLine || text.alignH != DRW_Text::HLeft) {
//    if (text.alignV != DRW_Text::VBaseLine || text.alignH == DRW_Text::HMiddle) {
//        if (text.alignH != DRW_Text::HLeft) {
        if (text.alignH == DRW_Text::HAligned || text.alignH == DRW_Text::HFit) {
            text.secPoint.x = t->getSecondPoint().x;
            text.secPoint.y = t->getSecondPoint().y;
        } else {
            text.secPoint.x = t->getInsertionPoint().x;
            text.secPoint.y = t->getInsertionPoint().y;
        }
    }

/*    if (text.alignH == DRW_Text::HAligned || text.alignH == DRW_Text::HFit) {
        text.secPoint.x = t->getSecondPoint().x;
        text.secPoint.y = t->getSecondPoint().y;
    }*/

    if (!t->getText().isEmpty()) {
        text.text = toDxfString(t->getText()).toUtf8().data();
        m_dxfW->writeText(&text);
    }
}

/**
 * Writes the given dimension entity to the file.
 */
void RS_FilterDXFRW::writeDimension(RS_Dimension* d) {
    QString blkName;
    if (m_noNameBlock.contains(d)) {
        blkName = m_noNameBlock.take(d);
    }

    // version 12 are inserts of *D blocks
    if (m_version==1009) {
        if (!blkName.isEmpty()) {
            DRW_Insert in;
            getEntityAttributes(&in, d);
            in.basePoint.x = in.basePoint.y = 0.0;
            in.basePoint.z = 0.0;
            in.name = blkName.toStdString();
            in.xscale = in.yscale = 1.0;
            in.zscale = 1.0;
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            m_dxfW->writeInsert(&in);
        }
        return;
    }

    DRW_Dimension* dim;
    int attachmentPoint=1;
    if (d->getHAlign()==RS_MTextData::HALeft) {
        attachmentPoint=1;
    } else if (d->getHAlign()==RS_MTextData::HACenter) {
        attachmentPoint=2;
    } else if (d->getHAlign()==RS_MTextData::HARight) {
        attachmentPoint=3;
    }
    if (d->getVAlign()==RS_MTextData::VATop) {
        attachmentPoint+=0;
    } else if (d->getVAlign()==RS_MTextData::VAMiddle) {
        attachmentPoint+=3;
    } else if (d->getVAlign()==RS_MTextData::VABottom) {
        attachmentPoint+=6;
    }

    switch (d->rtti()) {
        case RS2::EntityDimAligned: {
            auto* da = static_cast<RS_DimAligned*>(d);
            auto dd = new DRW_DimAligned();
            dim = dd;
            dim->type = 1 + 32;
            dd->setDef1Point(DRW_Coord(da->getExtensionPoint1().x, da->getExtensionPoint1().y, 0.0));
            dd->setDef2Point(DRW_Coord(da->getExtensionPoint2().x, da->getExtensionPoint2().y, 0.0));
            break;
        }
        case RS2::EntityDimDiametric: {
            auto* dr = static_cast<RS_DimDiametric*>(d);
            auto dd = new DRW_DimDiametric();
            dim = dd;
            dim->type = 3 + 32;
            dd->setDiameter1Point(DRW_Coord(dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
            dd->setLeaderLength(dr->getLeader());
            break;
        }
        case RS2::EntityDimRadial: {
            auto* dr = static_cast<RS_DimRadial*>(d);
            auto* dd = new DRW_DimRadial();
            dim = dd;
            dim->type = 4 + 32;
            dd->setDiameterPoint(DRW_Coord(dr->getDefinitionPoint().x, dr->getDefinitionPoint().y, 0.0));
            dd->setLeaderLength(dr->getLeader());
            break;
        }
        case RS2::EntityDimAngular: {
            auto* da = static_cast<RS_DimAngular*>(d);
            if (da->getDefinitionPoint3() == da->getData().definitionPoint) {
                auto* dd = new DRW_DimAngular3p();
                dim = dd;
                dim->type = 5 + 32;
                dd->setFirstLine(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //13
                dd->setSecondLine(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //14
                dd->SetVertexPoint(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //15
                dd->setDimPoint(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0)); //10
            }
            else {
                auto* dd = new DRW_DimAngular();
                dim = dd;
                dim->type = 2 + 32;
                dd->setFirstLine1(DRW_Coord(da->getDefinitionPoint1().x, da->getDefinitionPoint1().y, 0.0)); //13
                dd->setFirstLine2(DRW_Coord(da->getDefinitionPoint2().x, da->getDefinitionPoint2().y, 0.0)); //14
                dd->setSecondLine1(DRW_Coord(da->getDefinitionPoint3().x, da->getDefinitionPoint3().y, 0.0)); //15
                dd->setDimPoint(DRW_Coord(da->getDefinitionPoint4().x, da->getDefinitionPoint4().y, 0.0)); //16
            }
            break;
        }
        case RS2::EntityDimOrdinate: {
            auto* da = static_cast<LC_DimOrdinate*>(d);
            auto* dd = new DRW_DimOrdinate();
            dim = dd;
            dd->type = 6 + 32;
            auto dimOridinateData = da->getEData();
            if (dimOridinateData.ordinateForX) {
                dd->type = 6 + 64;
            }
            dd->setOriginPoint(DRW_Coord(da->getDefinitionPoint().x, da->getDefinitionPoint().y, 0.0));
            dd->setSecondLine(DRW_Coord(da->getLeaderEndPoint().x, da->getLeaderEndPoint().y, 0.0));
            dd->setFirstLine(DRW_Coord(da->getFeaturePoint().x, da->getFeaturePoint().y, 0.0));
            break;
        }
        default: {
            //default to DimLinear
            auto dl = static_cast<RS_DimLinear*>(d);
            auto dd = new DRW_DimLinear();
            dim = dd;
            dim->type = 0 + 32;
            dd->setDef1Point(DRW_Coord(dl->getExtensionPoint1().x, dl->getExtensionPoint1().y, 0.0));
            dd->setDef2Point(DRW_Coord(dl->getExtensionPoint2().x, dl->getExtensionPoint2().y, 0.0));
            dd->setAngle(RS_Math::rad2deg(dl->getAngle()));
            dd->setOblique(dl->getOblique());
            break;
        }
    }
    getEntityAttributes(dim, d);
    dim->setDefPoint(DRW_Coord(d->getDefinitionPoint().x, d->getDefinitionPoint().y, 0));
    dim->setTextPoint(DRW_Coord(d->getMiddleOfText().x, d->getMiddleOfText().y, 0));
    dim->setStyle (d->getStyle().toUtf8().data());
    dim->setAlign (attachmentPoint);
    dim->setTextLineStyle(d->getLineSpacingStyle());
    dim->setText (toDxfString(d->getText()).toUtf8().data());
    dim->setTextLineFactor(d->getLineSpacingFactor());
    dim->setHDir(d->getHDir());
    dim->setFlipArrow1(d->isFlipArrow1());
    dim->setFlipArrow2(d->isFlipArrow2());
    if (d->hasUserDefinedTextLocation()) {
        dim->type = dim->type + 128;
    }
    if (!blkName.isEmpty()) {
        dim->setName(blkName.toStdString());
    }
    LC_DimStyle* override = d->getDimStyleOverride();
    if (override != nullptr) {
        LC_ExtEntityData extEntityData;
        addDimStyleOverrideToExtendedData(&extEntityData, override);
        fillEntityExtData(dim->extData, &extEntityData);
    }
    m_dxfW->writeDimension(dim);
    delete dim;
}

/**
 * Writes the given leader entity to the file.
 */
void RS_FilterDXFRW::writeLeader(RS_Leader* l) {
    if (l->count() <= 0) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "dropping leader with no vertices");
    }

    DRW_Leader leader;
    getEntityAttributes(&leader, l);
    leader.style = "Standard";
    leader.arrow = l->hasArrowHead();
    leader.leadertype = 0;
    leader.flag = 3;
    leader.hookline = 0;
    leader.hookflag = 0;
    leader.textheight = 1;
    leader.textwidth = 10;
    leader.vertnum = l->count();
	RS_Line* li =nullptr;
    for(RS_Entity* v: lc::LC_ContainerTraverser{*l, RS2::ResolveNone}.entities()){
        if (v->rtti()==RS2::EntityLine) {
            li = static_cast<RS_Line*>(v);
			leader.vertexlist.push_back(std::make_shared<DRW_Coord>(li->getStartpoint().x, li->getStartpoint().y, 0.0));
        }
    }
	if (li){
		leader.vertexlist.push_back(std::make_shared<DRW_Coord>(li->getEndpoint().x, li->getEndpoint().y, 0.0));
	}

    m_dxfW->writeLeader(&leader);
}

/**
 * Writes the given hatch entity to the file.
 */
void RS_FilterDXFRW::writeHatch(RS_Hatch * h) {
    // version 12 are inserts of *U blocks
    if (m_version==1009) {
        if (m_noNameBlock.contains(h)) {
            DRW_Insert in;
            getEntityAttributes(&in, h);
            in.basePoint.x = in.basePoint.y = 0.0;
            in.basePoint.z = 0.0;
            in.name = m_noNameBlock.value(h).toUtf8().data();
            in.xscale = in.yscale = 1.0;
            in.zscale = 1.0;
            in.angle = 0.0;
            in.colcount = in.rowcount = 1;
            in.colspace = in.rowspace = 0.0;
            m_dxfW->writeInsert(&in);
        }
        return;
    }

    bool writeIt = true;
    if (h->countLoops()>0) {
        // check if all of the loops contain entities:
        for(RS_Entity* l: lc::LC_ContainerTraverser{*h, RS2::ResolveNone}.entities()){
            if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
                if (l->count()==0) {
                    writeIt = false;
                }
            }
        }
    } else {
        writeIt = false;
    }

    if (!writeIt) {
        RS_DEBUG->print(RS_Debug::D_WARNING,"RS_FilterDXF::writeHatch: Dropping Hatch");
        return;
    }

    DRW_Hatch ha;
    getEntityAttributes(&ha, h);
    ha.solid = h->isSolid();
    ha.scale = h->getScale();
    ha.angle = h->getAngle();
    if (ha.solid) {
        ha.name = "SOLID";
    }
    else {
        ha.name = h->getPattern().toUtf8().data();
    }
    ha.loopsnum = h->countLoops();

    for (RS_Entity* l=h->firstEntity(RS2::ResolveNone);
         l;
         l=h->nextEntity(RS2::ResolveNone)) {

        // Write hatch loops:
        if (l->isContainer() && !l->getFlag(RS2::FlagTemp)) {
            auto loop = static_cast<RS_EntityContainer*>(l);
			std::shared_ptr<DRW_HatchLoop> lData = std::make_shared<DRW_HatchLoop>(0);

            for(RS_Entity* ed: lc::LC_ContainerTraverser{*loop, RS2::ResolveNone}.entities()){
                // Write hatch loop edges:
                if (ed->rtti()==RS2::EntityLine) {
                    auto* ln = static_cast<RS_Line*>(ed);
					std::shared_ptr<DRW_Line> line = std::make_shared<DRW_Line>();
                    line->basePoint.x = ln->getStartpoint().x;
                    line->basePoint.y = ln->getStartpoint().y;
                    line->secPoint.x = ln->getEndpoint().x;
                    line->secPoint.y = ln->getEndpoint().y;
                    lData->objlist.push_back(line);
                } else if (ed->rtti()==RS2::EntityArc) {
                    auto ar = static_cast<RS_Arc*>(ed);
					std::shared_ptr<DRW_Arc> arc = std::make_shared<DRW_Arc>();
                    arc->basePoint.x = ar->getCenter().x;
                    arc->basePoint.y = ar->getCenter().y;
                    arc->radious = ar->getRadius();
                    if (!ar->isReversed()) {
                        arc->staangle = ar->getAngle1();
                        arc->endangle = ar->getAngle2();
                        arc->isccw = true;
                    } else {
                        arc->staangle = 2*M_PI-ar->getAngle1();
                        arc->endangle = 2*M_PI-ar->getAngle2();
                        arc->isccw = false;
                    }
                    lData->objlist.push_back(arc);
                } else if (ed->rtti()==RS2::EntityCircle) {
                    auto ci = static_cast<RS_Circle*>(ed);
					std::shared_ptr<DRW_Arc> arc = std::make_shared<DRW_Arc>();
					arc->basePoint.x = ci->getCenter().x;
                    arc->basePoint.y = ci->getCenter().y;
                    arc->radious = ci->getRadius();
                    arc->staangle = 0.0;
                    arc->endangle = 2*M_PI; //2*M_PI;
                    arc->isccw = true;
                    lData->objlist.push_back(arc);
                } else if (ed->rtti()==RS2::EntityEllipse) {
                    auto el = static_cast<RS_Ellipse*>(ed);
					std::shared_ptr<DRW_Ellipse> ell = std::make_shared<DRW_Ellipse>();
                    ell->basePoint.x = el->getCenter().x;
                    ell->basePoint.y = el->getCenter().y;
                    ell->secPoint.x = el->getMajorP().x;
                    ell->secPoint.y = el->getMajorP().y;
                    ell->ratio = el->getRatio();
                    double rot = el->getMajorP().angle();
                    double startAng = el->getCenter().angleTo(el->getStartpoint()) - rot;
                    double endAng = el->getCenter().angleTo(el->getEndpoint()) - rot;
                    if (startAng < 0) {
                        startAng = M_PI*2 + startAng;
                    }
                    if (endAng < 0) {
                        endAng = M_PI*2 + endAng;
                    }
                    ell->staparam = startAng;
                    ell->endparam = endAng;
                    ell->isccw = !el->isReversed();
                    lData->objlist.push_back(ell);
                }
            }
            lData->update(); //change to DRW_HatchLoop
            ha.appendLoop(lData);
        }
    }
    m_dxfW->writeHatch(&ha);
}

/**
 * Writes the given Solid entity to the file.
 */
void RS_FilterDXFRW::writeSolid(RS_Solid* s) {
    RS_SolidData data;
    DRW_Solid solid;
    RS_Vector corner;
    getEntityAttributes(&solid, s);
    corner = s->getCorner(0);
    solid.basePoint.x = corner.x;
    solid.basePoint.y = corner.y;
    corner = s->getCorner(1);
    solid.secPoint.x = corner.x;
    solid.secPoint.y = corner.y;
    corner = s->getCorner(2);
    solid.thirdPoint.x = corner.x;
    solid.thirdPoint.y = corner.y;
    if (s->isTriangle()) {
        solid.fourPoint.x = solid.thirdPoint.x;
        solid.fourPoint.y = solid.thirdPoint.y;
    } else {
        corner = s->getCorner(3);
        solid.fourPoint.x = corner.x;
        solid.fourPoint.y = corner.y;
    }
    m_dxfW->writeSolid(&solid);
}


void RS_FilterDXFRW::writeImage(RS_Image * i) {
    DRW_Image image;
    getEntityAttributes(&image, i);

    image.basePoint.x = i->getInsertionPoint().x;
    image.basePoint.y = i->getInsertionPoint().y;
    image.secPoint.x = i->getUVector().x;
    image.secPoint.y = i->getUVector().y;
    image.vVector.x = i->getVVector().x;
    image.vVector.y = i->getVVector().y;
    image.sizeu = i->getWidth();
    image.sizev = i->getHeight();
    image.brightness = i->getBrightness();
    image.contrast = i->getContrast();
    image.fade = i->getFade();

    DRW_ImageDef *imgDef = m_dxfW->writeImage(&image, i->getFile().toUtf8().data());
	if (imgDef ) {
        imgDef->loaded = 1;
        imgDef->u = i->getData().size.x;
        imgDef->v = i->getData().size.y;
        imgDef->up = 1;
        imgDef->vp = 1;
        imgDef->resolution = 0;
    }
}

/*void RS_FilterDXFRW::writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                                        const DRW_Entity& attrib) {
    QString blkName;
    blkName = "__CE";

    // Creating an unique ID from the element ID
    int tmp, c=1; // tmp = temporary var c = counter var
    tmp = con->getId();

    while (true) {
        tmp = tmp/c;
        blkName.append((char) tmp %10 + 48);
        c *= 10;
        if (tmp < 10) {
            break;
        }
    }

    //Block definition
    dw.sectionTables();
    dxf.writeBlockRecord(dw);
    dw.dxfString(  0, "BLOCK_RECORD");

    dw.handle();
    dw.dxfHex(330, 1);
    dw.dxfString(100, "AcDbSymbolTableRecord");
    dw.dxfString(100, "AcDbBlockTableRecord");
    dw.dxfString(  2, blkName.toLatin1().data());
    dw.dxfHex(340, 0);
    dw.dxfString(0, "ENDTAB");

    //Block creation
    RS_BlockData blkdata(blkName, RS_Vector(0,0), false);

    RS_Block* blk = new RS_Block(graphic, blkdata);

	for (RS_Entity* e1 = con->firstEntity(); e1 ;
            e1 = con->nextEntity() ) {
        blk->addEntity(e1);
    }
    writeBlock(dw, blk);
    //delete e1;
}*/



/**
 * Writes the atomic entities of the given container to the file.
 */
/*void RS_FilterDXFRW::writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c,
                                       const DRW_Entity& attrib,
                                       RS2::ResolveLevel level) {

    for (RS_Entity* e=c->firstEntity(level);
            e;
            e=c->nextEntity(level)) {

        writeEntity(dw, e, attrib);
    }
}*/


/**
 * Sets the entities attributes according to the attributes
 * that come from a DXF file.
 */
void RS_FilterDXFRW::setEntityAttributes(RS_Entity* entity,
                                       const DRW_Entity* attrib) {
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes");

    RS_Pen pen;
    pen.setColor(Qt::black);
    pen.setLineType(RS2::SolidLine);
    QString layName = toNativeString(QString::fromUtf8(attrib->layer.c_str()));

    // Layer: add layer in case it doesn't exist:
    if (!m_graphic->findLayer(layName)) {
        DRW_Layer lay;
        lay.name = attrib->layer;
        addLayer(lay);
    }
    entity->setLayer(layName);

    // Color:
    if (attrib->color24 >= 0) {
        pen.setColor(RS_Color(attrib->color24 >> 16,
                              attrib->color24 >> 8 & 0xFF,
                              attrib->color24 & 0xFF));
    }
    else {
        pen.setColor(numberToColor(attrib->color));
    }

    // Linetype:
    pen.setLineType(nameToLineType( QString::fromUtf8(attrib->lineType.c_str()) ));

    // Width:
    pen.setWidth(numberToWidth(attrib->lWeight));

    entity->setPen(pen);
    RS_DEBUG->print("RS_FilterDXF::setEntityAttributes: OK");
}

/**
 * Gets the entities attributes as a DL_Attributes object.
 */
void RS_FilterDXFRW::getEntityAttributes(DRW_Entity* ent, const RS_Entity* entity) {
//DRW_Entity RS_FilterDXFRW::getEntityAttributes(RS_Entity* /*entity*/) {

    // Layer:
    RS_Layer* layer = entity->getLayer();
    QString layerName;
    if (layer) {
        layerName = layer->getName();
    } else {
        layerName = "0";
    }

    RS_Pen pen = entity->getPen(false);

    // Color:
    int exact_rgb;
    int color = colorToNumber(pen.getColor(), &exact_rgb);
    //printf("Color is: %s -> %d\n", pen.getColor().name().toLatin1().data(), color);

    // Linetype:
    QString lineType = lineTypeToName(pen.getLineType());

    // Width:
    DRW_LW_Conv::lineWidth width = widthToNumber(pen.getWidth());

    ent->layer = toDxfString(layerName).toUtf8().data();
    ent->color = color;
    ent->color24 = exact_rgb;
    ent->lWeight = width;
    ent->lineType = lineType.toUtf8().data();
}

/**
 * @return Pen with the same attributes as 'attrib'.
 */
RS_Pen RS_FilterDXFRW::attributesToPen(const DRW_Layer* att) const {

    RS_Color col;
    if (att->color24 >= 0) {
        col = RS_Color(att->color24 >> 16,
                              att->color24 >> 8 & 0xFF,
                              att->color24 & 0xFF);
    }
    else {
        col = numberToColor(att->color);
    }

    RS_Pen pen(col, numberToWidth(att->lWeight),
               nameToLineType(QString::fromUtf8(att->lineType.c_str())) );
    return pen;
}

/**
 * Converts a color index (num) into a RS_Color object.
 * Please refer to the dxflib documentation for details.
 *
 * @param num Color number.
 */
RS_Color RS_FilterDXFRW::numberToColor(int num) {
        if (num==0) {
            return RS_Color(RS2::FlagByBlock);
        } else if (num==256) {
            return RS_Color(RS2::FlagByLayer);
        } else if (num<=255 && num>=0) {
            return RS_Color(DRW::dxfColors[num][0],
                            DRW::dxfColors[num][1],
                            DRW::dxfColors[num][2]);
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_FilterDXF::numberToColor: Invalid color number given.");
            return RS_Color(RS2::FlagByLayer);
        }

    return RS_Color();
}

/**
 * Converts a color into a color number in the DXF palette.
 * The color that fits best is chosen.
 */
int RS_FilterDXFRW::colorToNumber(const RS_Color& col, int *rgb) {
    //printf("Searching color for %s\n", col.name().toLatin1().data());
    *rgb = -1;
    // Special color BYBLOCK:
    if (col.getFlag(RS2::FlagByBlock)) {
        return 0;
    }
    // Special color BYLAYER
    else if (col.getFlag(RS2::FlagByLayer)) {
        return 256;
    }
    // Special color black is not in the table but white represents both
    // black and white
    else {
        int red = col.red();
        int green = col.green();
        int blue = col.blue();
        if (red==0 && green==0 && blue==0) {
            return 7;
        }
        // All other colors
        else {
            int num=0;
            int diff=255*3;  // smallest difference to a color in the table found so far

            // Run through the whole table and compare
            for (int i=1; i<=255; i++) {
                int d = abs(red-DRW::dxfColors[i][0])
                    + abs(green-DRW::dxfColors[i][1])
                    + abs(blue-DRW::dxfColors[i][2]);

                if (d<diff) {
                    /*
                printf("color %f,%f,%f is closer\n",
                       dxfColors[i][0],
                       dxfColors[i][1],
                       dxfColors[i][2]);
                */
                    diff = d;
                    num = i;
                    if (d==0) {
                        break;
                    }
                }
            }
            //printf("  Found: %d, diff: %d\n", num, diff);
            if(diff != 0) {
                *rgb = 0;
                *rgb = red<<16 | green<<8 | blue;
            }
            return num;
        }
    }
}

void RS_FilterDXFRW::add3dFace(const DRW_3Dface& data) {
    RS_DEBUG->print("RS_FilterDXFRW::add3dFace");
    RS_PolylineData d(RS_Vector(false),
                      RS_Vector(false),
                      !data.invisibleflag);
    auto *polyline = new RS_Polyline(m_currentContainer, d);
    setEntityAttributes(polyline, &data);
    RS_Vector v1(data.basePoint.x, data.basePoint.y);
    RS_Vector v2(data.secPoint.x, data.secPoint.y);
    RS_Vector v3(data.thirdPoint.x, data.thirdPoint.y);
    RS_Vector v4(data.fourPoint.x, data.fourPoint.y);

    polyline->addVertex(v1, 0.0);
    polyline->addVertex(v2, 0.0);
    polyline->addVertex(v3, 0.0);
    polyline->addVertex(v4, 0.0);

    m_currentContainer->addEntity(polyline);
}

void RS_FilterDXFRW::addComment(const char*) {
    RS_DEBUG->print("RS_FilterDXF::addComment(const char*) not yet implemented.");
}

void RS_FilterDXFRW::addPlotSettings(const DRW_PlotSettings *data) {
    m_graphic->setPagesNum(QString::fromStdString(data->plotViewName));
    m_graphic->setMargins(data->marginLeft, data->marginTop,
                        data->marginRight, data->marginBottom);
}

/**
 * Converts a line type name (e.g. "CONTINUOUS") into a RS2::LineType
 * object.
 */
RS2::LineType RS_FilterDXFRW::nameToLineType(const QString& name) {

    QString uName = name.toUpper();

    // Standard linetypes for QCad II / AutoCAD
    if (uName.isEmpty() || uName=="BYLAYER") {
        return RS2::LineByLayer;
    }
    if (uName=="BYBLOCK") {
        return RS2::LineByBlock;
    }
    if (uName=="CONTINUOUS" || uName=="ACAD_ISO01W100") {
        return RS2::SolidLine;
    }
    if (uName=="ACAD_ISO07W100" || uName=="DOT") {
        return RS2::DotLine;
    }
    if (uName=="DOTTINY") {
        return RS2::DotLineTiny;
    }
    if (uName=="DOT2") {
        return RS2::DotLine2;
    }
    if (uName=="DOTX2") {
        return RS2::DotLineX2;
    }
    if (uName=="ACAD_ISO02W100" || uName=="ACAD_ISO03W100" ||
               uName=="DASHED" || uName=="HIDDEN") {
        return RS2::DashLine;
    }
    if (uName=="DASHEDTINY" || uName=="HIDDEN2") {
        return RS2::DashLineTiny;
    }
    if (uName=="DASHED2" || uName=="HIDDEN2") {
        return RS2::DashLine2;
    }
    if (uName=="DASHEDX2" || uName=="HIDDENX2") {
        return RS2::DashLineX2;
    }
    if (uName=="ACAD_ISO10W100" ||
               uName=="DASHDOT") {
        return RS2::DashDotLine;
    }
    if (uName=="DASHDOTTINY") {
        return RS2::DashDotLineTiny;
    }
    if (uName=="DASHDOT2") {
        return RS2::DashDotLine2;
    }
    if (uName=="ACAD_ISO04W100" ||
               uName=="DASHDOTX2") {
        return RS2::DashDotLineX2;
    }
    if (uName=="ACAD_ISO12W100" || uName=="DIVIDE") {
        return RS2::DivideLine;
    }
    if (uName=="DIVIDETINY") {
        return RS2::DivideLineTiny;
    }
    if (uName=="DIVIDE2") {
        return RS2::DivideLine2;
    }
    if (uName=="ACAD_ISO05W100" || uName=="DIVIDEX2") {
        return RS2::DivideLineX2;
    }
    if (uName=="CENTER") {
        return RS2::CenterLine;
    }
    if (uName=="CENTERTINY") {
        return RS2::CenterLineTiny;
    }
    if (uName=="CENTER2") {
        return RS2::CenterLine2;
    }
    if (uName=="CENTERX2") {
        return RS2::CenterLineX2;
    }
    if (uName=="BORDER") {
        return RS2::BorderLine;
    }
    if (uName=="BORDERTINY") {
        return RS2::BorderLineTiny;
    }
    if (uName=="BORDER2") {
        return RS2::BorderLine2;
    }
    if (uName=="BORDERX2") {
        return RS2::BorderLineX2;
    }

    return RS2::SolidLine;
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
QString RS_FilterDXFRW::lineTypeToName(RS2::LineType lineType) {
    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
        case RS2::SolidLine:
            return "CONTINUOUS";
        case RS2::DotLine:
            return "DOT";
        case RS2::DotLineTiny:
            return "DOTTINY";
        case RS2::DotLine2:
            return "DOT2";
        case RS2::DotLineX2:
            return "DOTX2";
        case RS2::DashLine:
            return "DASHED";
        case RS2::DashLineTiny:
            return "DASHEDTINY";
        case RS2::DashLine2:
            return "DASHED2";
        case RS2::DashLineX2:
            return "DASHEDX2";
        case RS2::DashDotLine:
            return "DASHDOT";
        case RS2::DashDotLineTiny:
            return "DASHDOTTINY";
        case RS2::DashDotLine2:
            return "DASHDOT2";
        case RS2::DashDotLineX2:
            return "DASHDOTX2";
        case RS2::DivideLine:
            return "DIVIDE";
        case RS2::DivideLineTiny:
            return "DIVIDETINY";
        case RS2::DivideLine2:
            return "DIVIDE2";
        case RS2::DivideLineX2:
            return "DIVIDEX2";
        case RS2::CenterLine:
            return "CENTER";
        case RS2::CenterLineTiny:
            return "CENTERTINY";
        case RS2::CenterLine2:
            return "CENTER2";
        case RS2::CenterLineX2:
            return "CENTERX2";
        case RS2::BorderLine:
            return "BORDER";
        case RS2::BorderLineTiny:
            return "BORDERTINY";
        case RS2::BorderLine2:
            return "BORDER2";
        case RS2::BorderLineX2:
            return "BORDERX2";
        case RS2::LineByLayer:
            return "ByLayer";
        case RS2::LineByBlock:
            return "ByBlock";
        default:
            break;
    }
    return "CONTINUOUS";
}

/**
 * Converts a RS_LineType into a name for a line type.
 */
/*QString RS_FilterDXFRW::lineTypeToDescription(RS2::LineType lineType) {

    // Standard linetypes for QCad II / AutoCAD
    switch (lineType) {
    case RS2::SolidLine:
        return "Solid line";
    case RS2::DotLine:
        return "ISO Dashed __ __ __ __ __ __ __ __ __ __ _";
    case RS2::DashLine:
        return "ISO Dashed with Distance __    __    __    _";
    case RS2::DashDotLine:
        return "ISO Long Dashed Dotted ____ . ____ . __";
    case RS2::DashDotDotLine:
        return "ISO Long Dashed Double Dotted ____ .. __";
    case RS2::LineByLayer:
        return "";
    case RS2::LineByBlock:
        return "";
    default:
        break;
    }

    return "CONTINUOUS";
}*/

/**
 * Converts a DRW_LW_Conv::lineWidth into a RS2::LineWidth.
 */
RS2::LineWidth RS_FilterDXFRW::numberToWidth(DRW_LW_Conv::lineWidth lw) {
    switch (lw) {
        case DRW_LW_Conv::widthByLayer:
            return RS2::WidthByLayer;
        case DRW_LW_Conv::widthByBlock:
            return RS2::WidthByBlock;
        case DRW_LW_Conv::widthDefault:
            return RS2::WidthDefault;
        case DRW_LW_Conv::width00:
            return RS2::Width00;
        case DRW_LW_Conv::width01:
            return RS2::Width01;
        case DRW_LW_Conv::width02:
            return RS2::Width02;
        case DRW_LW_Conv::width03:
            return RS2::Width03;
        case DRW_LW_Conv::width04:
            return RS2::Width04;
        case DRW_LW_Conv::width05:
            return RS2::Width05;
        case DRW_LW_Conv::width06:
            return RS2::Width06;
        case DRW_LW_Conv::width07:
            return RS2::Width07;
        case DRW_LW_Conv::width08:
            return RS2::Width08;
        case DRW_LW_Conv::width09:
            return RS2::Width09;
        case DRW_LW_Conv::width10:
            return RS2::Width10;
        case DRW_LW_Conv::width11:
            return RS2::Width11;
        case DRW_LW_Conv::width12:
            return RS2::Width12;
        case DRW_LW_Conv::width13:
            return RS2::Width13;
        case DRW_LW_Conv::width14:
            return RS2::Width14;
        case DRW_LW_Conv::width15:
            return RS2::Width15;
        case DRW_LW_Conv::width16:
            return RS2::Width16;
        case DRW_LW_Conv::width17:
            return RS2::Width17;
        case DRW_LW_Conv::width18:
            return RS2::Width18;
        case DRW_LW_Conv::width19:
            return RS2::Width19;
        case DRW_LW_Conv::width20:
            return RS2::Width20;
        case DRW_LW_Conv::width21:
            return RS2::Width21;
        case DRW_LW_Conv::width22:
            return RS2::Width22;
        case DRW_LW_Conv::width23:
            return RS2::Width23;
        default:
            break;
    }
    return RS2::WidthDefault;
}

/**
 * Converts a RS2::LineWidth into an DRW_LW_Conv::lineWidth.
 */
DRW_LW_Conv::lineWidth RS_FilterDXFRW::widthToNumber(RS2::LineWidth width) {
    switch (width) {
        case RS2::WidthByLayer:
            return DRW_LW_Conv::widthByLayer;
        case RS2::WidthByBlock:
            return DRW_LW_Conv::widthByBlock;
        case RS2::WidthDefault:
            return DRW_LW_Conv::widthDefault;
        case RS2::Width00:
            return DRW_LW_Conv::width00;
        case RS2::Width01:
            return DRW_LW_Conv::width01;
        case RS2::Width02:
            return DRW_LW_Conv::width02;
        case RS2::Width03:
            return DRW_LW_Conv::width03;
        case RS2::Width04:
            return DRW_LW_Conv::width04;
        case RS2::Width05:
            return DRW_LW_Conv::width05;
        case RS2::Width06:
            return DRW_LW_Conv::width06;
        case RS2::Width07:
            return DRW_LW_Conv::width07;
        case RS2::Width08:
            return DRW_LW_Conv::width08;
        case RS2::Width09:
            return DRW_LW_Conv::width09;
        case RS2::Width10:
            return DRW_LW_Conv::width10;
        case RS2::Width11:
            return DRW_LW_Conv::width11;
        case RS2::Width12:
            return DRW_LW_Conv::width12;
        case RS2::Width13:
            return DRW_LW_Conv::width13;
        case RS2::Width14:
            return DRW_LW_Conv::width14;
        case RS2::Width15:
            return DRW_LW_Conv::width15;
        case RS2::Width16:
            return DRW_LW_Conv::width16;
        case RS2::Width17:
            return DRW_LW_Conv::width17;
        case RS2::Width18:
            return DRW_LW_Conv::width18;
        case RS2::Width19:
            return DRW_LW_Conv::width19;
        case RS2::Width20:
            return DRW_LW_Conv::width20;
        case RS2::Width21:
            return DRW_LW_Conv::width21;
        case RS2::Width22:
            return DRW_LW_Conv::width22;
        case RS2::Width23:
            return DRW_LW_Conv::width23;
        default:
            break;
    }
    return DRW_LW_Conv::widthDefault;
}

/**
 * Converts a native unicode string into a DXF encoded string.
 *
 * DXF endoding includes the following special sequences:
 * - %%%c for a diameter sign
 * - %%%d for a degree sign
 * - %%%p for a plus/minus sign
 */
QString RS_FilterDXFRW::toDxfString(const QString& str) {
    QString res = "";
    int j=0;
    for (int i=0; i<str.length(); ++i) {
        int c = str.at(i).unicode();
        if (c>175 || c<11){
            res.append(str.mid(j,i-j));
            j=i;

            switch (c) {
                case 0x0A:
                    res += "\\P";
                    break;
                // diameter:
                case 0x2205: //RLZ: Empty_set, diameter is 0x2300 need to add in all fonts
                case 0x2300:
                    res += "%%C";
                    break;
                // degree:
                case 0x00B0:
                    res += "%%D";
                    break;
                // plus/minus
                case 0x00B1:
                    res += "%%P";
                    break;
                default:
                    j--;
                    break;
            }
            j++;
        }
    }
    res.append(str.mid(j));
    return res;
}

/**
 * Converts a DXF encoded string into a native Unicode string.
 */
QString RS_FilterDXFRW::toNativeString(const QString& data) {
    QString res;

    // Ignore font tags:
    int j = 0;
    for (int i=0; i<data.length(); ++i) {
        if (data.at(i).unicode() == 0x7B){ //is '{' ?
            if (data.at(i+1).unicode() == 0x5c){ //and is "{\" ?
                //check known codes
                if ( (data.at(i+2).unicode() == 0x66) || //is "\f" ?
                     (data.at(i+2).unicode() == 0x48) || //is "\H" ?
                     (data.at(i+2).unicode() == 0x43)    //is "\C" ?
                   ) {
                    //found tag, append parsed part
                    res.append(data.mid(j,i-j));
                    qsizetype pos = data.indexOf(QChar(0x7D), i+3);//find '}'
                    if (pos <0) break; //'}' not found
                    QString tmp = data.mid(i+1, pos-i-1);
                    do {
                        tmp = tmp.remove(0,tmp.indexOf(QChar{0x3B}, 0)+1 );//remove to ';'
                    } while(tmp.startsWith("\\f") || tmp.startsWith("\\H") || tmp.startsWith("\\C"));
                    res.append(tmp);
                    i = j = pos;
                    ++j;
                }
            }
        }
    }
    res.append(data.mid(j));

    // Line feed:
    res = res.replace(QRegularExpression("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegularExpression("\\\\~"), " ");
    // Tab:
    res = res.replace(QRegularExpression("\\^I"), "    ");//RLZ: change 4 spaces for \t when mtext have support for tab
    // diameter:
    res = res.replace(QRegularExpression("%%[cC]"), QChar(0x2300));//RLZ: Empty_set is 0x2205, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegularExpression("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegularExpression("%%[pP]"), QChar(0x00B1));

    return res;
}

/**
 * Converts the given number from a DXF file into an AngleFormat enum.
 *
 * @param num $DIMAUNIT from DXF (0: decimal deg, 1: deg/min/sec, 2: gradians,
 *                                3: radians, 4: surveyor's units)
 *
 * @ret Matching AngleFormat enum value.
 */
RS2::AngleFormat RS_FilterDXFRW::numberToAngleFormat(int num) {
    switch (num) {
        default:
        case 0:
            return RS2::DegreesDecimal;
        case 1:
            return RS2::DegreesMinutesSeconds;
        case 2:
            return RS2::Gradians;
        case 3:
            return RS2::Radians;
        case 4:
            return RS2::Surveyors;
    }
}

/**
 * Converts AngleFormat enum to DXF number.
 */
int RS_FilterDXFRW::angleFormatToNumber(RS2::AngleFormat af) {
    switch (af) {
        default:
        case RS2::DegreesDecimal:
            return  0;
        case RS2::DegreesMinutesSeconds:
            return 1;
        case RS2::Gradians:
            return 2;
        case RS2::Radians:
            return 3;
        case RS2::Surveyors:
            return 4;
    }
}

/**
 * converts a DXF unit setting (e.g. INSUNITS) to a unit enum.
 */
RS2::Unit RS_FilterDXFRW::numberToUnit(int num) {
    switch (num) {
        default:
        case 0:
            return RS2::None;
        case 1:
            return RS2::Inch;
        case 2:
            return RS2::Foot;
        case 3:
            return RS2::Mile;
        case 4:
            return RS2::Millimeter;
        case 5:
            return RS2::Centimeter;
        case 6:
            return RS2::Meter;
        case 7:
            return RS2::Kilometer;
        case 8:
            return RS2::Microinch;
        case 9:
            return RS2::Mil;
        case 10:
            return RS2::Yard;
        case 11:
            return RS2::Angstrom;
        case 12:
            return RS2::Nanometer;
        case 13:
            return RS2::Micron;
        case 14:
            return RS2::Decimeter;
        case 15:
            return RS2::Decameter;
        case 16:
            return RS2::Hectometer;
        case 17:
            return RS2::Gigameter;
        case 18:
            return RS2::Astro;
        case 19:
            return RS2::Lightyear;
        case 20:
            return RS2::Parsec;
    }
    return RS2::None;
}

/**
 * Converts a unit enum into a DXF unit number e.g. for INSUNITS.
 */
int RS_FilterDXFRW::unitToNumber(RS2::Unit unit) {
    switch (unit) {
        default:
        case RS2::None:
            return 0;
        case RS2::Inch:
            return 1;
        case RS2::Foot:
            return 2;
        case RS2::Mile:
            return 3;
        case RS2::Millimeter:
            return 4;
        case RS2::Centimeter:
            return 5;
        case RS2::Meter:
            return 6;
        case RS2::Kilometer:
            return 7;
        case RS2::Microinch:
            return 8;
        case RS2::Mil:
            return 9;
        case RS2::Yard:
            return 10;
        case RS2::Angstrom:
            return 11;
        case RS2::Nanometer:
            return 12;
        case RS2::Micron:
            return 13;
        case RS2::Decimeter:
            return 14;
        case RS2::Decameter:
            return 15;
        case RS2::Hectometer:
            return 16;
        case RS2::Gigameter:
            return 17;
        case RS2::Astro:
            return 18;
        case RS2::Lightyear:
            return 19;
        case RS2::Parsec:
            return 20;
    }
    return 0;
}

/**
 * Checks if the given variable is two-dimensional (e.g. $LIMMIN).
 */
bool RS_FilterDXFRW::isVariableTwoDimensional(const QString& var) {
    if (var=="$LIMMIN" ||
            var=="$LIMMAX" ||
            var=="$PLIMMIN" ||
            var=="$PLIMMAX" ||
            var=="$GRIDUNIT" ||
            var=="$VIEWCTR") {

        return true;
    } else {
        return false;
    }
}

void RS_FilterDXFRW::addPolylineSegment(RS_Polyline& polyline, RS_Vector prev_pos, RS_Vector curr_pos, double bulge, const std::vector<std::shared_ptr<DRW_Variant>>& extData, bool isClosedSegment) {
    bool isLcData = false;
    double yRadius = 0.0;

    // Issue #1946: LibreCAD may use elliptic arcs internally, but dxf file saving
    // uses arcs only. Minor radius of elliptic is saved as DRW_Variant::content
    // The content is flagged as "LibreCAD"
    for (const std::shared_ptr<DRW_Variant>& var : extData) {
        if (var->code() == 1001) {
            isLcData = *(var->content.s) == "LibreCad";
        } else if (isLcData && var->code() == 1040) {
            yRadius = var->content.d;
        }
    }
    bool isElliptic = yRadius > RS_TOLERANCE;

    if (isElliptic) {
        std::unique_ptr<RS_Arc> arc{ RS_Polyline::arcFromBulge(prev_pos, curr_pos, bulge)};
        if (arc != nullptr && arc->getRadius() >= RS_TOLERANCE) {
            double radius = arc->getRadius();
            double scaleRatio = yRadius / radius;
            RS_Ellipse* ellipse = RS_Polyline::convertToEllipse(std::make_pair(arc.get(), scaleRatio));
            if (ellipse != nullptr) {
                ellipse->setParent(&polyline);
                ellipse->setSelected(polyline.isSelected());
                ellipse->setPen(RS_Pen(RS2::FlagInvalid));
                ellipse->setLayer(nullptr);
                polyline.addEntity(ellipse);
                polyline.getData().endpoint = curr_pos;
            }
        }
    } else {
        polyline.addVertex(curr_pos, bulge, false);
    }

    if (isClosedSegment) {
        polyline.setNextBulge(bulge);
    }
}

#ifdef DWGSUPPORT
QString RS_FilterDXFRW::printDwgVersion(int v){
    switch (v) {
        case DRW::AC1006:
            return "10";
        case DRW::AC1009:
            return "dwg version 11 or 12";
        case DRW::AC1012:
            return "dwg version 13";
        case DRW::AC1014:
            return "dwg version 14";
        case DRW::AC1015:
            return "dwg version 2000";
        case DRW::AC1018:
            return "dwg version 2004";
        case DRW::AC1021:
            return "dwg version 2007";
        case DRW::AC1024:
            return "dwg version 2010";
        case DRW::AC1027:
            return "dwg version 2013";
        case DRW::AC1032:
            return "dwg version 2018";
        default:
            return "unknown";
    }
}

void RS_FilterDXFRW::printDwgError(int le){
    switch (le) {
        case DRW::BAD_UNKNOWN:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("unknown error opening dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_UNKNOWN");
            break;
        case DRW::BAD_OPEN:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("can't open this dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_OPEN");
            break;
        case DRW::BAD_VERSION:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("unsupported dwg version"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_VERSION");
            break;
        case DRW::BAD_READ_METADATA:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading file metadata in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_FILE_HEADER");
            break;
        case DRW::BAD_READ_FILE_HEADER:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading file header in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_FILE_HEADER");
            break;
        case DRW::BAD_READ_HEADER:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading header vars in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_HEADER");
            break;
        case DRW::BAD_READ_CLASSES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading classes in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_CLASSES");
            break;
        case DRW::BAD_READ_HANDLES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading offsets in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OFFSETS");
            break;
        case DRW::BAD_READ_TABLES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading tables in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_TABLES");
            break;
        case DRW::BAD_READ_BLOCKS:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading blocks in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OFFSETS");
            break;
        case DRW::BAD_READ_ENTITIES:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading entities in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_ENTITIES");
            break;
        case DRW::BAD_READ_OBJECTS:
            RS_DIALOGFACTORY->commandMessage(QObject::tr("error reading objects in dwg file"));
            RS_DEBUG->print("RS_FilterDXFRW::printDwgError: DRW::BAD_READ_OBJECTS");
            break;
        default:
            break;
    }
}

QString RS_FilterDXFRW::strVal(DRW_Variant* var) {
    return QString::fromUtf8(var->c_str());
}

LC_DimStyle *RS_FilterDXFRW::createDimStyle(const DRW_Dimstyle &s) {
    auto* result = new LC_DimStyle();
    QString name = QString::fromUtf8(s.name.c_str());
    result->setName(name);

    bool styleForEntityType = !result->isBaseStyle();
    if (styleForEntityType) {
        result->setModifyCheckMode(LC_DimStyle::ModificationAware::ALL);
    }

    auto arrowStyle = result->arrowhead();

    DRW_Variant* var = s.get("$DIMBLK");
    if (var != nullptr) {
        arrowStyle->setSameBlockName(strVal(var));
    }
    var = s.get("$DIMBLK1");
    if (var != nullptr) {
        arrowStyle->setArrowHeadBlockNameFirst(strVal(var));
    }
    var = s.get("$DIMBLK2");
    if (var != nullptr) {
        arrowStyle->setArrowHeadBlockNameSecond(strVal(var));
    }
    var = s.get("$DIMASZ");
    if (var != nullptr) {
        arrowStyle->setSize(var->d_val());
    }
    var = s.get("$DIMTSZ");
    if (var != nullptr) {
        arrowStyle->setTickSize(var->d_val());
    }
    var = s.get("$DIMSAH");
    if (var != nullptr) {
        arrowStyle->setUseSeparateArrowHeads(var->i_val());
    }
    var = s.get("$DIMSOXD");
    if (var != nullptr) {
        arrowStyle->setSuppressionsRaw(var->i_val());
    }

    auto scaleStyle = result->scaling();
    var = s.get("$DIMSCALE");
    if (var != nullptr) {
        scaleStyle->setScale(var->d_val());
    }
    var = s.get("$DIMLFAC");
    if (var != nullptr) {
        scaleStyle->setLinearFactor(var->d_val());
    }

    auto extLineStyle = result->extensionLine();

    var = s.get("$DIMEXO");
    if (var != nullptr) {
        extLineStyle->setDistanceFromOriginPoint(var->d_val());
    }
    var = s.get("$DIMEXE");
    if (var != nullptr) {
        extLineStyle->setDistanceBeyondDimLine(var->d_val());
    }
    var = s.get("$DIMFXL");
    if (var != nullptr) {
        extLineStyle->setFixedLength(var->d_val());
    }
    var = s.get("$DIMFXLON");
    if (var != nullptr) {
        extLineStyle->setHasFixedLength(var->i_val()  == 1);
    }
    var = s.get("$DIMLWE");
    if (var != nullptr) {
        extLineStyle->setLineWidthRaw(var->i_val());
    }
    var = s.get("$DIMCLRE");
    if (var != nullptr) {
        extLineStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMSE1");
    if (var != nullptr) {
        extLineStyle->setSuppressFirstRaw(var->i_val());
    }
    var = s.get("$DIMSE2");
    if (var != nullptr) {
        extLineStyle->setSuppressSecondRaw(var->i_val());
    }
    var = s.get("$DIMLTEX1");
    if (var != nullptr) {
        auto dimltex1 = strVal(var);
        if (!dimltex1.isEmpty()) {
            extLineStyle->setLineTypeFirst(dimltex1);
        }
    }
    var = s.get("$DIMLTEX2");
    if (var != nullptr) {
        auto dimltex2 = strVal(var);
        if (!dimltex2.isEmpty()) {
            extLineStyle->setLineTypeSecond(dimltex2);
        }
    }

    auto dimLineStyle = result->dimensionLine();
    var = s.get("$DIMLWD");
    if (var != nullptr) {
        dimLineStyle->setLineWidthRaw(var->i_val());
    }
    var = s.get("$DIMDLE");
    if (var != nullptr) {
        dimLineStyle->setDistanceBeyondExtLinesForObliqueStroke(var->d_val());
    }
    var = s.get("$DIMDLI");
    if (var != nullptr) {
        dimLineStyle->setBaselineDimLinesSpacing(var->d_val());
    }
    var = s.get("$DIMGAP");
    if (var != nullptr) {
        dimLineStyle->setLineGap(var->d_val());
    }
    var = s.get("$DIMCLRD");
    if (var != nullptr) {
        dimLineStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMSD1");
    if (var != nullptr) {
        dimLineStyle->setSuppressFirstLineRaw(var->i_val());
    }
    var = s.get("$DIMSD2");
    if (var != nullptr) {
        dimLineStyle->setSuppressSecondLineRaw(var->i_val());
    }
    var = s.get("$DIMTOFL");
    if (var != nullptr) {
        dimLineStyle->setDrawPolicyForOutsideTextRaw(var->i_val());
    }
    var = s.get("$DIMLTYPE");
    if (var != nullptr) {
        auto dimltype = strVal(var);
        if (!dimltype.isEmpty()) {
            dimLineStyle->setLineType(dimltype);
        }
    }

    auto textStyle = result->text();
    var = s.get("$DIMTXT");
    if (var != nullptr) {
        textStyle->setHeight(var->d_val());
    }
    var = s.get("$DIMTXSTY");
    if (var != nullptr) {
        auto dimtxsty = strVal(var);
        prepareTextStyleName(dimtxsty);
        textStyle->setStyle(dimtxsty); // fixme - resolve via table?
    }
    var = s.get("$DIMTOH");
    if (var != nullptr) {
        textStyle->setOrientationOutsideRaw(var->i_val());
    }
    var = s.get("$DIMTIH");
    if (var != nullptr) {
        textStyle->setOrientationInsideRaw(var->i_val());
    }
    var = s.get("$DIMJUST");
    if (var != nullptr) {
        textStyle->setHorizontalPositioningRaw(var->i_val());
    }
    var = s.get("$DIMCLRT");
    if (var != nullptr) {
        textStyle->setColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMTAD");
    if (var != nullptr) {
        textStyle->setVerticalPositioningRaw(var->i_val());
    }
    var = s.get("$DIMTIX");
    if (var != nullptr) {
        textStyle->setExtLinesRelativePlacementRaw(var->i_val());
    }
    var = s.get("$DIMTFILL");
    if (var != nullptr) {
        textStyle->setBackgroundFillModeRaw(var->i_val());
    }
    var = s.get("$DIMTFILLCLR");
    if (var != nullptr) {
        textStyle->setExplicitBackgroundFillColor(numberToColor(var->i_val()));
    }
    var = s.get("$DIMTXTDIRECTION");
    if (var != nullptr) {
        textStyle->setReadingDirectionRaw(var->i_val());
    }
    var = s.get("$DIMTVP");
    if (var != nullptr) {
        textStyle->setVerticalDistanceToDimLine(var->d_val());
    }
    var = s.get("$DIMUPT");
    if (var != nullptr) {
        textStyle->setCursorControlPolicyRaw(var->i_val());
    }
    var = s.get("$DIMTMOVE");
    if (var != nullptr) {
        textStyle->setPositionMovementPolicyRaw(var->i_val());
    }
    var = s.get("$DIMATFIT");
    if (var != nullptr) {
        textStyle->setUnsufficientSpacePolicyRaw(var->i_val());
    }

    auto zerosSuppression = result->zerosSuppression();

    var = s.get("$DIMZIN");
    if (var != nullptr) {
        zerosSuppression->setLinearRaw(var->i_val());
    }
    var = s.get("$DIMAZIN");
    if (var != nullptr) {
        zerosSuppression->setAngularRaw(var->i_val());
    }
    var = s.get("$DIMTZIN");
    if (var != nullptr) {
        zerosSuppression->setToleranceRaw(var->i_val());
    }
    var = s.get("$DIMALTZ");
    if (var != nullptr) {
        zerosSuppression->setAltLinearRaw(var->i_val());
    }
    var = s.get("$DIMALTTZ");
    if (var != nullptr) {
        zerosSuppression->setAltToleranceRaw(var->i_val());
    }

    auto linearFormat = result->linearFormat();

    var = s.get("$DIMLUNIT");
    if (var != nullptr) {
        linearFormat->setFormatRaw(var->i_val());
    }
    var = s.get("$DIMDSEP");
    if (var != nullptr) {
        linearFormat->setDecimalFormatSeparatorChar(var->i_val());
    }
    var = s.get("$DIMDEC");
    if (var != nullptr) {
        linearFormat->setDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMPOST");
    if (var != nullptr) {
        linearFormat->setPrefixOrSuffix(strVal(var));
    }
    var = s.get("$DIMALT");
    if (var != nullptr) {
        linearFormat->setAlternateUnitsRaw(var->i_val());
    }
    var = s.get("$DIMALTU");
    if (var != nullptr) {
        linearFormat->setAltFormatRaw(var->i_val());
    }
    var = s.get("$DIMALTD");
    if (var != nullptr) {
        linearFormat->setAltDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMALTF");
    if (var != nullptr) {
        linearFormat->setAltUnitsMultiplier(var->d_val());
    }
    var = s.get("$DIMAPOST");
    if (var != nullptr) {
        linearFormat->setAltPrefixOrSuffix(strVal(var));
    }

    auto angularFormat = result->angularFormat();
    var = s.get("$DIMAUNIT");
    if (var != nullptr) {
        angularFormat->setFormatRaw(var->i_val());
    }
    var = s.get("$DIMADEC");
    if (var != nullptr) {
        angularFormat->setDecimalPlaces(var->i_val());
    }

    auto linearRoundOff = result->roundOff();
    var = s.get("$DIMRND");
    if (var != nullptr) {
        linearRoundOff->setRoundToValue(var->d_val());
    }
    var = s.get("$DIMALTRND");
    if (var != nullptr) {
        linearRoundOff->setAltRoundToValue(var->d_val());
    }

    auto fractionStyle = result->fractions();
    var = s.get("$DIMFRAC");
    if (var != nullptr) {
        fractionStyle->setStyleRaw(var->i_val());
    }

    auto radialStyle = result->radial();
    var = s.get("$DIMCEN");
    if (var != nullptr) {
        radialStyle->setCenterMarkOrLineSize(var->d_val());
    }

    auto toleranceStyle = result->latteralTolerance();

    var = s.get("$DIMTDEC");
    if (var != nullptr) {
        toleranceStyle->setDecimalPlaces(var->i_val());
    }
    var = s.get("$DIMALTTD");
    if (var != nullptr) {
        toleranceStyle->setDecimalPlacesAltDim(var->i_val());
    }
    var = s.get("$DIMTOL");
    if (var != nullptr) {
        toleranceStyle->setAppendTolerancesToDimText(var->i_val()); // fixme - review
    }
    var = s.get("$DIMTOLJ");
    if (var != nullptr) {
        toleranceStyle->setVerticalJustificationRaw(var->i_val());
    }
    var = s.get("$DIMTM");
    if (var != nullptr) {
        toleranceStyle->setLowerToleranceLimit(var->d_val()); // fixme - review
    }
    var = s.get("$DIMTP");
    if (var != nullptr) {
        toleranceStyle->setUpperToleranceLimit(var->d_val());
    }
    var = s.get("$DIMTFAC");
    if (var != nullptr) {
        toleranceStyle->setHeightScaleFactorToDimText(var->d_val());
    }
    var = s.get("$DIMLIM");
    if (var != nullptr) {
        toleranceStyle->setLimitsAreGeneratedAsDefaultText(var->i_val()); // fixme - review
    }

    auto leaderStyle = result->leader();
    var = s.get("$DIMLDRBLK");
    if (var != nullptr) {
        leaderStyle->setArrowBlockName(strVal(var));
    }

    auto arc = result->arc();
    var = s.get("$DIMARCSYM");
    if (var != nullptr) {
      arc->setArcSymbolPositionRaw(var->i_val());
    }

    // fixme - remove to MLeaderStyle
    /*auto mleaderStyle = result->mleader();
    var = s.get("$MLEADERSCALE");
    if (var != nullptr) {
        mleaderStyle->setScale(var->d_val());
    }*/
    // mleaderStyle->setScale(s.mleaderscale);

    parseDimStyleExtData(s, result);

    if (styleForEntityType) {
        result->setModifyCheckMode(LC_DimStyle::ModificationAware::SET);
    }
    return result;
}

bool RS_FilterDXFRW::resolveBlockNameByHandle(duint32 blockHandle, QString& blockName) const {
    std::string name = m_dxfR->getReadingContext()->resolveBlockRecordName(blockHandle);
    if (name.empty()) {
        return false;
    }
    blockName = name.c_str();
    return true;
}

void RS_FilterDXFRW::parseDimStyleExtData(const DRW_Dimstyle& s, LC_DimStyle* result) {
    std::vector<DRW_Variant> tagData;
    int currentValType = 0;
    QString applicationName = "";
    bool expectType = false;
    // https://help.autodesk.com/view/OARX/2024/ENU/?guid=GUID-3F0380A5-1C15-464D-BC66-2C5F094BCFB9
    // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-7943.htm
    for (auto v: s.extData) {
        int code = v->code();
        switch (code) {
            case 1001: { // application name
                if (!applicationName.isEmpty()) {
                    applyParsedDimStyleExtData(result, applicationName, tagData);
                    tagData.clear();
                }
                applicationName = v->c_str();
                expectType = false; // for later "not", as actually we do expect it
                break;
            }
            case 1002: { // control name
                break;
            }
            case 1070: // integer
            case 1071:{// long
                int val = v->i_val();
                if (expectType) {
                    // code of var
                    currentValType = val;
                }
                else {
                    // it fields
                    auto intVar = DRW_Variant(currentValType, val);
                    tagData.push_back(intVar);
                }
                break;
            }
            case 1040: // real
            case 1041: // distance
            case 1042: { // scale factor
                double val = v->d_val();
                auto doubleVar = DRW_Variant(currentValType, val);
                tagData.push_back(doubleVar);
                break;
            }
            default:
                break;
        }
        expectType = !expectType;
    }
    if (!applicationName.isEmpty()) { // process last app name setion
        applyParsedDimStyleExtData(result, applicationName, tagData);
        tagData.clear();
    }
}

void RS_FilterDXFRW::applyParsedDimStyleExtData(LC_DimStyle* dimStyle, const QString& appName, const std::vector<DRW_Variant>& vector) {
    if (vector.empty()) {
        return;
    }
    const DRW_Variant* var = &vector.at(0);
    int code = var->code();

    if (appName == "ACAD_DSTYLE_DIMJAG") {
        if (code == 388) {
            // double val = var->d_val();
            // fixme - decide where to store it. this is "Jog Height Factor"
            // dimStyle->dimensionLine()->set
        }

        // code 388, float
    }
    else if (appName == "ACAD_DSTYLE_DIMTALN") {
        // code 392, int
        if (code == 392) {
            int val = var->i_val();
            dimStyle->latteralTolerance()->setAdjustmentRaw(val);
        }
    }
    else if (appName == "ACAD_DSTYLE_DIMBREAK") {
        // code 391, float
        if (code == 391) {
            // double val = var->d_val();
            // fixme - decide where to store it. This is "Dimension Break" in acad.
        }
    }
}

#endif // DWGSUPPORT

