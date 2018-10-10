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

/** 
  * TODO:
  * - Complete block support
  * - Support for hatches, dimensions, text, solids
  * - Support for user interactions
  * - Support for more than one document
  */
#ifdef RS_OPT_PYTHON

#include <boost/python.hpp>
using namespace boost::python;

#include "rs_python_wrappers.h"
#include "rs_python.h"

#include "rs.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_constructionline.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_entity.h"
#include "rs_flags.h"
#include "rs_graphic.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_vector.h"

/* Global root functions */
RS_Graphic* currentGraphic() { return RS_PYTHON->getGraphic(); }

    /* more to be added later (access to global properties, all documents,
       creation of new documents, ... */

/* To/From Python string conversion logic for string management */
namespace QString_Python_Conversions {
    namespace {
        struct QString_to_python_str
        {
            static PyObject* convert(QString const& s)
            {
                return boost::python::incref(boost::python::object((const char*)s).ptr());
            }
        };

        struct QString_from_python_str
        {
            QString_from_python_str()
            {
                boost::python::converter::registry::push_back(
                    &convertible,
                    &construct,
                    boost::python::type_id<QString>());
            }

            static void* convertible(PyObject* obj_ptr)
            {
                if (!PyString_Check(obj_ptr)) return 0;
                return obj_ptr;
            }

            static void construct(
                PyObject* obj_ptr,
                boost::python::converter::rvalue_from_python_stage1_data* data)
            {
                const char* value = PyString_AsString(obj_ptr);
                if (!value)
                    boost::python::throw_error_already_set();
                void* storage = (
                    (boost::python::converter::rvalue_from_python_storage<QString>*)
                    data)->storage.bytes;
                new (storage) QString(value);
                data->convertible = storage;
            }
        };

        void registerConversions()
        {
            using namespace boost::python;

            boost::python::to_python_converter<
                QString, QString_to_python_str>();

            QString_from_python_str();
        }
    }
}

/* Transfer of ownership is done by using auto pointers */
/* These are the helper functions needed for this mechanism */

#define TRANSFER_OWNERSHIP_FUNCTION(fname, container, addfunc, entity) \
   void fname(container& cont, std::auto_ptr<entity> obj) \
   { cont.addfunc(obj.get()); obj.release(); }

TRANSFER_OWNERSHIP_FUNCTION(RS_Graphic_addLayer, RS_Graphic, addLayer, RS_Layer)
TRANSFER_OWNERSHIP_FUNCTION(RS_LayerList_add, RS_LayerList, add, RS_Layer)

#define ADDVERTEX_FUNCTION(fname, entity) \
   TRANSFER_OWNERSHIP_FUNCTION(fname, RS_EntityContainer, addEntity, entity)

ADDVERTEX_FUNCTION(RS_EntityContainer_addArc, RS_Arc)
ADDVERTEX_FUNCTION(RS_EntityContainer_addBlock, RS_Block)
ADDVERTEX_FUNCTION(RS_EntityContainer_addCircle, RS_Circle)
ADDVERTEX_FUNCTION(RS_EntityContainer_addConstructionLine, RS_ConstructionLine)
ADDVERTEX_FUNCTION(RS_EntityContainer_addEllipse, RS_Ellipse)
ADDVERTEX_FUNCTION(RS_EntityContainer_addImage, RS_Image)
ADDVERTEX_FUNCTION(RS_EntityContainer_addLine, RS_Line)
ADDVERTEX_FUNCTION(RS_EntityContainer_addPoint, RS_Point)
ADDVERTEX_FUNCTION(RS_EntityContainer_addPolyline, RS_Polyline)

/* Overloaded functions helpers */
void (RS_LayerList::*RS_LayerList_activate_string)(const QString&) = &RS_LayerList::activate;
void (RS_LayerList::*RS_LayerList_activate_layer)(RS_Layer*) = &RS_LayerList::activate;
void (RS_LayerList::*RS_LayerList_toggle_string)(const QString&) = &RS_LayerList::toggle;
void (RS_LayerList::*RS_LayerList_toggle_layer)(const QString&) = &RS_LayerList::toggle;
void (RS_Graphic::*RS_Graphic_toggleLayer_string)(const QString&) = &RS_Graphic::toggleLayer;
void (RS_Graphic::*RS_Graphic_toggleLayer_layer)(RS_Layer*) = &RS_Graphic::toggleLayer;
void (RS_Entity::*RS_Entity_setLayer_string)(const QString&) = &RS_Entity::setLayer;
void (RS_Entity::*RS_Entity_setLayer_layer)(RS_Layer*) = &RS_Entity::setLayer;

/**
  * The main python module
  */
  
BOOST_PYTHON_MODULE(librecad)
{
    /* Initialization code */
    QString_Python_Conversions::registerConversions();
    
    /* Unbound functions */
    
    def("currentGraphic", currentGraphic, return_value_policy<reference_existing_object>());

    /* Enums */
    enum_<RS2::Flags>("Flag")
        .value("Undone", RS2::FlagUndone)
	.value("Visible", RS2::FlagVisible)
	.value("ByLayer", RS2::FlagByLayer)
	.value("ByBlock", RS2::FlagByBlock)
	.value("Frozen", RS2::FlagFrozen)
	.value("DefFrozen", RS2::FlagDefFrozen)
	.value("Locked", RS2::FlagLocked)
	.value("Invalid", RS2::FlagInvalid)
	.value("Selected", RS2::FlagSelected)
	.value("Closed", RS2::FlagClosed)
	.value("Temp", RS2::FlagTemp)
	.value("Processed", RS2::FlagProcessed)
	.value("Selected1", RS2::FlagSelected1)
	.value("Selected2", RS2::FlagSelected2)
    ;

    enum_<RS2::VariableType>("VariableType")
        .value("String", RS2::VariableString)
	.value("Int", RS2::VariableInt)
	.value("Double", RS2::VariableDouble)
	.value("Vector", RS2::VariableVector)
	.value("Void", RS2::VariableVoid)
    ;

    enum_<RS2::EntityType>("EntityType")
        .value("Unknown", RS2::EntityUnknown)
	.value("Container", RS2::EntityContainer)
	.value("Block", RS2::EntityBlock)
	.value("FontChar", RS2::EntityFontChar)
	.value("Insert", RS2::EntityInsert)
	.value("Graphic", RS2::EntityGraphic)
	.value("Point", RS2::EntityPoint)
	.value("Line", RS2::EntityLine)
	.value("Polyline", RS2::EntityPolyline)
	.value("Vertex", RS2::EntityVertex)
	.value("Arc", RS2::EntityArc)
	.value("Circle", RS2::EntityCircle)
	.value("Ellipse", RS2::EntityEllipse)
	.value("Solid", RS2::EntitySolid)
	.value("ConstructionLine", RS2::EntityConstructionLine)
	.value("Text", RS2::EntityText)
	.value("DimAligned", RS2::EntityDimAligned)
	.value("DimLinear", RS2::EntityDimLinear)
	.value("DimRadial", RS2::EntityDimRadial)
	.value("DimDiametric", RS2::EntityDimDiametric)
	.value("DimAngular", RS2::EntityDimAngular)
	.value("DimLeader", RS2::EntityDimLeader)
	.value("Hatch", RS2::EntityHatch)
	.value("Image", RS2::EntityImage)
    ;

    enum_<RS2::LineType>("LineType")
        .value("NoPen", RS2::NoPen)
	.value("SolidLine", RS2::SolidLine)
	.value("DotLine", RS2::DotLine)
	.value("DotLine2", RS2::DotLine2)
	.value("DotLineX2", RS2::DotLineX2)
	.value("DashLine", RS2::DashLine)
	.value("DashLine2", RS2::DashLine2)
	.value("DashLineX2", RS2::DashLineX2)
	.value("DashDotLine", RS2::DashDotLine)
	.value("DashDotLine2", RS2::DashDotLine2)
	.value("DashDotLineX2", RS2::DashDotLineX2)
	.value("DivideLine", RS2::DivideLine)
	.value("DivideLine2", RS2::DivideLine2)
	.value("DivideLineX2", RS2::DivideLineX2)
	.value("CenterLine", RS2::CenterLine)
	.value("CenterLine2", RS2::CenterLine2)
	.value("CenterLineX2", RS2::CenterLineX2)
	.value("BorderLine", RS2::BorderLine)
	.value("BorderLine2", RS2::BorderLine2)
	.value("BorderLineX2", RS2::BorderLineX2)
	.value("ByLayer", RS2::LineByLayer)
	.value("ByBlock", RS2::LineByBlock)
    ;

    enum_<RS2::LineWidth>("LineWidth")
        .value("Width00", RS2::Width00)
        .value("Width01", RS2::Width01)
        .value("Width02", RS2::Width02)
        .value("Width03", RS2::Width03)
        .value("Width04", RS2::Width04)
        .value("Width05", RS2::Width05)
        .value("Width06", RS2::Width06)
        .value("Width07", RS2::Width07)
        .value("Width08", RS2::Width08)
        .value("Width09", RS2::Width09)
        .value("Width10", RS2::Width10)
        .value("Width11", RS2::Width11)
        .value("Width12", RS2::Width12)
        .value("Width13", RS2::Width13)
        .value("Width14", RS2::Width14)
        .value("Width15", RS2::Width15)
        .value("Width16", RS2::Width16)
        .value("Width17", RS2::Width17)
        .value("Width18", RS2::Width18)
        .value("Width19", RS2::Width19)
        .value("Width20", RS2::Width20)
        .value("Width21", RS2::Width21)
        .value("Width22", RS2::Width22)
        .value("Width23", RS2::Width23)
	.value("ByLayer", RS2::WidthByLayer)
	.value("ByBlock", RS2::WidthByBlock)
	.value("Default", RS2::WidthDefault)
    ;

    /* "Small" classes */

    class_<RS_Flags>("Flags")
        .def(init<int>())
	.add_property("flags", &RS_Flags::getFlags, &RS_Flags::setFlags)
	.def("resetFlags", &RS_Flags::resetFlags)
	.def("setFlag", &RS_Flags::setFlag)
	.def("delFlag", &RS_Flags::delFlag)
	.def("toggleFlag", &RS_Flags::toggleFlag)
	.def("getFlag", &RS_Flags::getFlag)
    ;

    class_<RS_Color, bases<RS_Flags> >("Color")
        .def(init<int, int, int>())
	.def(init<int>())
	.def("stripFlags", &RS_Color::stripFlags)
	.add_property("byLayer", &RS_Color::isByLayer)
	.add_property("byBlock", &RS_Color::isByBlock)
    ;

    class_<RS_Vector>("Vector")
        .def(init<double, double, optional<double> >())
	.def("set", &RS_Vector::set)
	.def("setPolar", &RS_Vector::setPolar)
	.def("distanceTo", &RS_Vector::distanceTo)
	.def("angle", &RS_Vector::angle)
	.def("angleTo", &RS_Vector::angleTo)
	.def("magnitude", &RS_Vector::magnitude)
	.def("move", &RS_Vector::move)
	.def_readwrite("x", &RS_Vector::x)
	.def_readwrite("y", &RS_Vector::y)
	.def_readwrite("z", &RS_Vector::z)
	.def_readwrite("valid", &RS_Vector::valid)
    ;

    class_<RS_Pen, bases<RS_Flags> >("Pen")
        .def(init<const RS_Color&, RS2::LineWidth, RS2::LineType>())
	.add_property("lineType", &RS_Pen::getLineType, &RS_Pen::setLineType)
	.add_property("width", &RS_Pen::getWidth, &RS_Pen::setWidth)
	.add_property("screenWidth", &RS_Pen::getScreenWidth, &RS_Pen::setScreenWidth)
	.add_property("color", make_function(&RS_Pen::getColor, return_value_policy<reference_existing_object>()), &RS_Pen::setColor)
	.add_property("valid", &RS_Pen::isValid)
    ;

    /* Common stuff */
    
    class_<RS_EntityContainer>("EntityContainer", init<RS_EntityContainer*, optional<bool> >())
        /* Wrapper functions for ownership transfer */
	.def("addEntity", RS_EntityContainer_addArc)
	.def("addEntity", RS_EntityContainer_addBlock)
	.def("addEntity", RS_EntityContainer_addCircle)
	.def("addEntity", RS_EntityContainer_addConstructionLine)
	.def("addEntity", RS_EntityContainer_addEllipse)
	.def("addEntity", RS_EntityContainer_addImage)
	.def("addEntity", RS_EntityContainer_addLine)
	.def("addEntity", RS_EntityContainer_addPoint)
	.def("addEntity", RS_EntityContainer_addPolyline)
	
	/** Owner-Containers will automatically delete entities upon removing,
	  * so no problem here. Other containers are not allowed in Python at the moment.
	  */
	.def("removeEntity", &RS_EntityContainer::removeEntity)

	/* Standard wrappers */
        .def("clear", &RS_EntityContainer::clear)
	.add_property("empty", &RS_EntityContainer::isEmpty)
	.def("entityAt", &RS_EntityContainer::entityAt, return_value_policy<reference_existing_object>())

        /* Iterators */
        .def("firstEntity", &RS_EntityContainer::firstEntity, return_value_policy<reference_existing_object>())
	.def("lastEntity", &RS_EntityContainer::lastEntity, return_value_policy<reference_existing_object>())
	.def("nextEntity", &RS_EntityContainer::nextEntity, return_value_policy<reference_existing_object>())
    ;

    class_<RS_LayerData>("LayerData")
        .def(init<const QString&, const RS_Pen&, bool>())
	.def_readwrite("name", &RS_LayerData::name)
	.def_readwrite("pen", &RS_LayerData::pen)
	.def_readwrite("frozen", &RS_LayerData::frozen)
    ;

    class_<RS_Layer, std::auto_ptr<RS_Layer> >("Layer", init<const QString&>())
        .add_property("name", &RS_Layer::getName, &RS_Layer::setName)
	.add_property("pen", &RS_Layer::getPen, &RS_Layer::setPen)
	.add_property("frozen", &RS_Layer::isFrozen, &RS_Layer::freeze)
	.add_property("toggle", &RS_Layer::toggle)
    ;

    class_<RS_LayerList>("LayerList")
        .def("clear", &RS_LayerList::clear)
	.def("count", &RS_LayerList::count)
	.def("at", &RS_LayerList::at, return_value_policy<reference_existing_object>())
	.add_property("active", make_function(&RS_LayerList::getActive, return_value_policy<reference_existing_object>()),
	                        RS_LayerList_activate_layer)
	.def("activate", RS_LayerList_activate_string)
	.def("activate", RS_LayerList_activate_layer)
        .def("add", RS_LayerList_add)
	.def("remove", &RS_LayerList::remove)
	.def("edit", &RS_LayerList::edit)
	.def("find", &RS_LayerList::find, return_value_policy<reference_existing_object>())
	.def("toggle", RS_LayerList_toggle_string)
	.def("toggle", RS_LayerList_toggle_layer)
	.def("freezeAll", &RS_LayerList::freezeAll)
        .def("lockAll", &RS_LayerList::lockAll)
    ;

    class_<RS_Document, bases<RS_EntityContainer>, boost::noncopyable >("Document", no_init)
        .add_property("layerList", make_function(&RS_Document::getLayerList, return_value_policy<reference_existing_object>()))
	.add_property("blockList", make_function(&RS_Document::getBlockList, return_value_policy<reference_existing_object>()))
	.def("newDoc", &RS_Document::newDoc)
	.def("save", &RS_Document::save)
	.def("saveAs", &RS_Document::saveAs)
	.def("open", &RS_Document::open)
	.add_property("modified", &RS_Document::isModified)
	.add_property("activePen", &RS_Document::getActivePen, &RS_Document::setActivePen)
	.add_property("filename", &RS_Document::getFilename)
    ;
    
    class_<RS_Graphic, bases<RS_Document> >("Graphic", init<RS_EntityContainer*>())
        .def("count", &RS_Graphic::count)
        .def("findLayer", &RS_Graphic::findLayer, return_value_policy<reference_existing_object>())
	.def("editLayer", &RS_Graphic::editLayer)
	.def("addLayer", RS_Graphic_addLayer)
	.def("removeLayer", &RS_Graphic::removeLayer)
	.def("toggleLayer", RS_Graphic_toggleLayer_string)
	.def("toggleLayer", RS_Graphic_toggleLayer_layer)
	.def("clearLayers", &RS_Graphic::clearLayers)
	.def("freezeAllLayers", &RS_Graphic::freezeAllLayers)
	.def("lockAllLayers", &RS_Graphic::lockAllLayers)
    ;

    /* Entity types */

    class_<RS_Entity, boost::noncopyable>("Entity", no_init)
        .def("init", &RS_Entity::init)
	.def("initId", &RS_Entity::initId)
	.def("clone", &RS_Entity::clone, return_value_policy<reference_existing_object>())
	.def("reparent", &RS_Entity::reparent)
	.def("resetBorders", &RS_Entity::resetBorders)
	.add_property("id", &RS_Entity::getId)
	.add_property("count", &RS_Entity::count)
	.add_property("parent", make_function(&RS_Entity::getParent, return_value_policy<reference_existing_object>()), &RS_Entity::setParent)
	.add_property("graphic", make_function(&RS_Entity::getGraphic, return_value_policy<reference_existing_object>()))
	.add_property("block", make_function(&RS_Entity::getBlock, return_value_policy<reference_existing_object>()))
	.add_property("insert", make_function(&RS_Entity::getInsert, return_value_policy<reference_existing_object>()))
	.add_property("blockOrInsert", make_function(&RS_Entity::getBlockOrInsert, return_value_policy<reference_existing_object>()))
	.add_property("document", make_function(&RS_Entity::getDocument, return_value_policy<reference_existing_object>()))
	.add_property("layer", make_function(&RS_Entity::getLayer, return_value_policy<reference_existing_object>()),
	              RS_Entity_setLayer_layer)
	.def("setLayer", RS_Entity_setLayer_string)
	.def("setLayer", RS_Entity_setLayer_layer)
	.def("setLayerToActive", &RS_Entity::setLayerToActive)
	.add_property("isContainer", &RS_Entity::isContainer)
	.add_property("isAtomic", &RS_Entity::isAtomic)
	.add_property("isEdge", &RS_Entity::isEdge)
	.add_property("isDocument", &RS_Entity::isDocument)
	.add_property("selected", &RS_Entity::isSelected, &RS_Entity::setSelected)
	.def("toggleSelected", &RS_Entity::toggleSelected)
	.add_property("processed", &RS_Entity::isProcessed, &RS_Entity::setProcessed)
	.add_property("visible", &RS_Entity::isVisible, &RS_Entity::setVisible)
	.add_property("min", &RS_Entity::getMin)
	.add_property("max", &RS_Entity::getMax)
	.add_property("size", &RS_Entity::getSize)
	.def("move", &RS_Entity::move)
	.def("rotate", &RS_Entity::rotate)
	.def("calculateBorders", &RS_Entity::calculateBorders)
    ;

    class_<RS_AtomicEntity, bases<RS_Entity>, boost::noncopyable>("AtomicEntity", no_init)
        .add_property("startpointSelected", &RS_AtomicEntity::isStartpointSelected)
	.add_property("endpointSelected", &RS_AtomicEntity::isEndpointSelected)
	.def("moveStartpoint", &RS_AtomicEntity::moveStartpoint)
	.def("moveEndpoint", &RS_AtomicEntity::moveEndpoint)
	.add_property("trimPoint", &RS_AtomicEntity::getTrimPoint)
	.def("reverse", &RS_AtomicEntity::reverse)
    ;

    /* Entities Data */

    class_<RS_ArcData>("ArcData")
        .def(init<RS_Vector&, double, double, double, bool>())
	.def("reset", &RS_ArcData::reset)
	.add_property("valid", &RS_ArcData::isValid)
	.def_readwrite("center", &RS_ArcData::center)
	.def_readwrite("radius", &RS_ArcData::radius)
	.def_readwrite("angle1", &RS_ArcData::angle1)
	.def_readwrite("angle2", &RS_ArcData::angle2)
	.def_readwrite("reversed", &RS_ArcData::reversed)
    ;

    class_<RS_BlockData>("BlockData")
        .def(init<QString&, const RS_Vector&, bool>())
	.add_property("valid", &RS_BlockData::isValid)
	.def_readwrite("name", &RS_BlockData::name)
	.def_readwrite("basePoint", &RS_BlockData::basePoint)
	.def_readwrite("frozen", &RS_BlockData::frozen)
    ;

    class_<RS_CircleData>("CircleData")
        .def(init<RS_Vector&, double>())
	.def("reset", &RS_CircleData::reset)
	.add_property("valid", &RS_CircleData::isValid)
	.def_readwrite("center", &RS_CircleData::center)
	.def_readwrite("radius", &RS_CircleData::radius)
    ;

    class_<RS_ConstructionLineData>("ConstructionLineData")
        .def(init<RS_Vector&, RS_Vector&>())
    ;

    class_<RS_EllipseData>("EllipseData", init<const RS_Vector&, const RS_Vector&, double, double, double, bool>())
    ;

    class_<RS_ImageData>("ImageData")
        .def(init<int, const RS_Vector&, const RS_Vector&, const RS_Vector&, const RS_Vector&, const QString&, int, int, int>())
	.def_readwrite("handle", &RS_ImageData::handle)
	.def_readwrite("insertionPoint", &RS_ImageData::insertionPoint)
	.def_readwrite("uVector", &RS_ImageData::uVector)
	.def_readwrite("vVector", &RS_ImageData::vVector)
	.def_readwrite("size", &RS_ImageData::size)
	.def_readwrite("file", &RS_ImageData::file)
	.def_readwrite("brightness", &RS_ImageData::brightness)
	.def_readwrite("contrast", &RS_ImageData::contrast)
	.def_readwrite("fade", &RS_ImageData::fade)
    ;

    class_<RS_LineData>("LineData")
        .def(init<RS_Vector&, RS_Vector&>())
	.def_readwrite("startpoint", &RS_LineData::startpoint)
	.def_readwrite("endpoint", &RS_LineData::endpoint)
    ;

    class_<RS_PointData>("PointData", init<const RS_Vector&>())
    ;

    class_<RS_PolylineData>("PolylineData")
        .def(init<const RS_Vector&, const RS_Vector&, bool>())
    ;

    /* Entities */

    class_<RS_Arc, bases<RS_AtomicEntity>, std::auto_ptr<RS_Arc> >("Arc", init<RS_EntityContainer*, RS_ArcData&>())
        .add_property("data", &RS_Arc::getData, &RS_Arc::setData)
	.add_property("center", &RS_Arc::getCenter, &RS_Arc::setCenter)
	.add_property("radius", &RS_Arc::getRadius, &RS_Arc::setRadius)
	.add_property("angle1", &RS_Arc::getAngle1, &RS_Arc::setAngle1)
	.add_property("angle2", &RS_Arc::getAngle2, &RS_Arc::setAngle2)
	.add_property("direction1", &RS_Arc::getDirection1)
	.add_property("direction2", &RS_Arc::getDirection2)
	.add_property("reversed", &RS_Arc::isReversed, &RS_Arc::setReversed)
	.add_property("middlepoint", &RS_Arc::getMiddlePoint)
	.add_property("angleLength", &RS_Arc::getAngleLength)
	.add_property("length", &RS_Arc::getLength)
	.add_property("bulge", &RS_Arc::getBulge)
	.def("createFrom3P", &RS_Arc::createFrom3P)
    ;

    class_<RS_Block, bases<RS_Document>, std::auto_ptr<RS_Block> >("Block", init<RS_EntityContainer*, const RS_BlockData&>())
        .add_property("name", &RS_Block::getName, &RS_Block::setName)
	.add_property("basePoint", &RS_Block::getBasePoint)
	.add_property("frozen", &RS_Block::isFrozen, &RS_Block::freeze)
	.def("toggle", &RS_Block::toggle)
    ;

    class_<RS_Circle, bases<RS_AtomicEntity>, std::auto_ptr<RS_Circle> >("Circle", init<RS_EntityContainer*, const RS_CircleData&>())
        .add_property("data", &RS_Circle::getData)
	.add_property("center", &RS_Circle::getCenter, &RS_Circle::setCenter)
	.add_property("radius", &RS_Circle::getRadius, &RS_Circle::setRadius)
	.add_property("angleLength", &RS_Circle::getAngleLength)
	.def("createFromCR", &RS_Circle::createFromCR)
	.def("createFrom2P", &RS_Circle::createFrom2P)
	.def("createFrom3P", &RS_Circle::createFrom3P)
    ;

    class_<RS_ConstructionLine, bases<RS_AtomicEntity>, std::auto_ptr<RS_ConstructionLine> >
            ("ConstructionLine", init<RS_EntityContainer*, const RS_ConstructionLineData&>())
        .add_property("data", &RS_ConstructionLine::getData)
	.add_property("point1", &RS_ConstructionLine::getPoint1)
	.add_property("point2", &RS_ConstructionLine::getPoint2)
    ;

    class_<RS_Ellipse, bases<RS_AtomicEntity>, std::auto_ptr<RS_Ellipse> >
            ("Ellipse", init<RS_EntityContainer*, const RS_EllipseData&>())
	.add_property("data", &RS_Ellipse::getData)
	.add_property("reversed", &RS_Ellipse::isReversed, &RS_Ellipse::setReversed)
	.add_property("angle", &RS_Ellipse::getAngle)
	.add_property("angle1", &RS_Ellipse::getAngle1, &RS_Ellipse::setAngle1)
	.add_property("angle2", &RS_Ellipse::getAngle2, &RS_Ellipse::setAngle2)
	.add_property("center", &RS_Ellipse::getCenter, &RS_Ellipse::setCenter)
	.add_property("majorP", &RS_Ellipse::getMajorP, &RS_Ellipse::setMajorP)
	.add_property("ratio", &RS_Ellipse::getRatio, &RS_Ellipse::setRatio)
	.add_property("majorRadius", &RS_Ellipse::getMajorRadius)
	.add_property("minorRadius", &RS_Ellipse::getMinorRadius)
    ;

    class_<RS_Image, bases<RS_AtomicEntity>, std::auto_ptr<RS_Image> >
            ("Image", init<RS_EntityContainer*, const RS_ImageData&>())
	.add_property("data", &RS_Image::getData)
	.add_property("insertionPoint", &RS_Image::getInsertionPoint, &RS_Image::setInsertionPoint)
	.add_property("file", &RS_Image::getFile, &RS_Image::setFile)
	.add_property("uVector", &RS_Image::getUVector)
	.add_property("vVector", &RS_Image::getVVector)
	.add_property("width", &RS_Image::getWidth)
	.add_property("height", &RS_Image::getHeight)
	.add_property("brightness", &RS_Image::getBrightness)
	.add_property("contrast", &RS_Image::getContrast)
	.add_property("fade", &RS_Image::getFade)
	.add_property("handle", &RS_Image::getHandle, &RS_Image::setHandle)
	.add_property("imageWidth", &RS_Image::getImageWidth)
	.add_property("imageHeight", &RS_Image::getImageHeight)
    ;

    class_<RS_Line, bases<RS_AtomicEntity>, std::auto_ptr<RS_Line> >
            ("Line", init<RS_EntityContainer*, const RS_LineData&>())
	.add_property("data", &RS_Line::getData)
	.add_property("startpoint", &RS_Line::getStartpoint, &RS_Line::setStartpoint)
	.add_property("endpoint", &RS_Line::getEndpoint, &RS_Line::setEndpoint)
    ;

    class_<RS_Point, bases<RS_AtomicEntity>, std::auto_ptr<RS_Point> >
            ("Point", init<RS_EntityContainer*, const RS_PointData&>())
	.add_property("pos", &RS_Point::getPos, &RS_Point::setPos)
    ;

    class_<RS_Polyline, bases<RS_EntityContainer>, std::auto_ptr<RS_Polyline> >
            ("Polyline", init<RS_EntityContainer*, const RS_PolylineData&>())
        .def(init<RS_EntityContainer*>())
	.add_property("startpoint", &RS_Polyline::getStartpoint, &RS_Polyline::setStartpoint)
	.add_property("endpoint", &RS_Polyline::getEndpoint)
	.add_property("closed", &RS_Polyline::isClosed)
	.def("addVertex", &RS_Polyline::addVertex, return_value_policy<reference_existing_object>())
	.def("createVertex", &RS_Polyline::createVertex, return_value_policy<reference_existing_object>())
	.def("endPolyline", &RS_Polyline::endPolyline)
    ;
}

#endif
