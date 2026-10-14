/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>

namespace {
void initializeSettings() {
    static int argc=1;
    static char name[]="entity_pen_inheritance_tests";
    static char *argv[]={name,nullptr};
    static QCoreApplication *app=QCoreApplication::instance()?QCoreApplication::instance():new QCoreApplication(argc,argv);
    static const bool initialized=[] { RS_Settings::init("LibreCAD","LibreCAD-tests");return true; }();
    (void)app;(void)initialized;
    RS_DEBUG->setLevel(RS_Debug::D_ERROR);
}
}

TEST_CASE("Invalid native containers transparently inherit current ancestor pens", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_EntityContainer outer(nullptr,false), middle(&outer,false), inner(&middle,false);
    RS_Line line(&inner,RS_LineData({0,0},{1,0}));
    outer.setPen(RS_Pen(RS_Color(Qt::red),RS2::Width07,RS2::DashLine));
    middle.setPen(RS_Pen(RS2::FlagInvalid));inner.setPen(RS_Pen(RS2::FlagInvalid));
    line.setPen(RS_Pen(RS2::FlagInvalid));
    auto resolved=line.getPen();
    CHECK(resolved.isValid());CHECK(resolved.getColor()==RS_Color(Qt::red));
    CHECK(resolved.getWidth()==RS2::Width07);CHECK(resolved.getLineType()==RS2::DashLine);
    outer.setPen(RS_Pen(RS_Color(Qt::blue),RS2::Width11,RS2::SolidLine));
    resolved=line.getPen();
    CHECK(resolved.getColor()==RS_Color(Qt::blue));CHECK(resolved.getWidth()==RS2::Width11);
    CHECK(resolved.getLineType()==RS2::SolidLine);
    outer.setPen(RS_Pen(RS2::FlagInvalid));CHECK_FALSE(line.getPen().isValid());
}

TEST_CASE("ByBlock components traverse actual valid ancestors independently", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_EntityContainer outer(nullptr,false), block(&outer,false), transparent(&block,false);
    outer.setPen(RS_Pen(RS_Color(Qt::red),RS2::Width11,RS2::DashLine));
    block.setPen(RS_Pen(RS_Color(RS2::FlagByBlock),RS2::Width07,RS2::LineByBlock));
    transparent.setPen(RS_Pen(RS2::FlagInvalid));
    RS_Line line(&transparent,RS_LineData({0,0},{1,0}));
    line.setPen(RS_Pen(RS_Color(Qt::green),RS2::WidthByBlock,RS2::LineByBlock));
    auto pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::green));CHECK(pen.getWidth()==RS2::Width07);
    CHECK(pen.getLineType()==RS2::DashLine);
    line.setPen(RS_Pen(RS2::FlagInvalid));pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::red));CHECK(pen.getWidth()==RS2::Width07);
    CHECK(pen.getLineType()==RS2::DashLine);
    outer.setPen(RS_Pen(RS_Color(Qt::blue),RS2::Width11,RS2::SolidLine));
    pen=line.getPen();CHECK(pen.getColor()==RS_Color(Qt::blue));CHECK(pen.getLineType()==RS2::SolidLine);
}

TEST_CASE("ByLayer keeps each supplying ancestor's live layer and Graphic boundary", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_Graphic graphic;
    auto *parentLayer=new RS_Layer(QStringLiteral("parent-pen"));
    auto *childLayer=new RS_Layer(QStringLiteral("child-pen"));
    parentLayer->setPen(RS_Pen(RS_Color(Qt::red),RS2::Width07,RS2::DashLine));
    childLayer->setPen(RS_Pen(RS_Color(Qt::green),RS2::Width11,RS2::SolidLine));
    graphic.getLayerList()->add(parentLayer);graphic.getLayerList()->add(childLayer);
    graphic.setPen(RS_Pen(RS_Color(Qt::yellow),RS2::Width11,RS2::SolidLine));
    RS_EntityContainer parent(&graphic,false), transparent(&parent,false);
    parent.setLayer(parentLayer);
    parent.setPen(RS_Pen(RS_Color(RS2::FlagByLayer),RS2::WidthByLayer,RS2::LineByLayer));
    transparent.setPen(RS_Pen(RS2::FlagInvalid));
    RS_Line line(&transparent,RS_LineData({0,0},{1,0}));line.setLayer(childLayer);
    line.setPen(RS_Pen(RS_Color(RS2::FlagByBlock),RS2::WidthByLayer,RS2::LineByBlock));
    auto pen=line.getPen();CHECK(pen.getColor()==RS_Color(Qt::red));CHECK(pen.getWidth()==RS2::Width11);
    CHECK(pen.getLineType()==RS2::DashLine);
    parentLayer->setPen(RS_Pen(RS_Color(Qt::blue),RS2::Width07,RS2::SolidLine));
    pen=line.getPen();CHECK(pen.getColor()==RS_Color(Qt::blue));CHECK(pen.getLineType()==RS2::SolidLine);
    line.setPen(RS_Pen(RS2::FlagInvalid));pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::blue));CHECK(pen.getWidth()==RS2::Width07);
    parent.setLayer(childLayer);pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::green));CHECK(pen.getWidth()==RS2::Width11);
    parent.setPen(RS_Pen(RS2::FlagInvalid));CHECK_FALSE(line.getPen().isValid());
    parent.setPen(RS_Pen(RS_Color(RS2::FlagByBlock),RS2::WidthByBlock,RS2::LineByBlock));
    pen=line.getPen();CHECK(pen.isColorByBlock());CHECK(pen.isWidthByBlock());CHECK(pen.isLineTypeByBlock());
}

TEST_CASE("Inherited pens retain explicit RGBA and named component identities", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_EntityContainer outer(nullptr,false), transparent(&outer,false);
    RS_Color color(12,34,56,78);color.setColorName(QStringLiteral("BOOK$ENTRY"));
    RS_Pen source(color,RS2::Width05,RS2::SolidLine);
    source.setLineTypeName(QStringLiteral("native-custom-line"));
    outer.setPen(source);transparent.setPen(RS_Pen(RS2::FlagInvalid));
    RS_Line leaf(&transparent,RS_LineData({0,0},{1,0}));
    leaf.setPen(RS_Pen(RS2::FlagInvalid));
    auto pen=leaf.getPen();
    CHECK(pen.getColor()==color);CHECK(pen.getColor().colorName()==color.colorName());
    CHECK(pen.getLineTypeId()==source.getLineTypeId());
    CHECK(pen.getLineTypeName()==source.getLineTypeName());
    leaf.setPen(RS_Pen(RS_Color(Qt::white),RS2::WidthByBlock,RS2::LineByBlock));
    pen=leaf.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::white));CHECK(pen.getWidth()==RS2::Width05);
    CHECK(pen.getLineTypeId()==source.getLineTypeId());
    source.setLineTypeName(QStringLiteral("changed-live-line"));source.setWidth(RS2::Width11);
    outer.setPen(source);pen=leaf.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::white));CHECK(pen.getWidth()==RS2::Width11);
    CHECK(pen.getLineTypeName()==source.getLineTypeName());
}

TEST_CASE("A ByBlock width that inherits ByLayer takes the layer of the ancestor that supplied it", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_Graphic graphic;
    auto *parentLayer=new RS_Layer(QStringLiteral("parent-pen"));
    auto *childLayer=new RS_Layer(QStringLiteral("child-pen"));
    parentLayer->setPen(RS_Pen(RS_Color(Qt::red),RS2::Width07,RS2::DashLine));
    childLayer->setPen(RS_Pen(RS_Color(Qt::green),RS2::Width11,RS2::SolidLine));
    graphic.getLayerList()->add(parentLayer);graphic.getLayerList()->add(childLayer);
    RS_EntityContainer parent(&graphic,false);
    parent.setLayer(parentLayer);
    parent.setPen(RS_Pen(RS_Color(RS2::FlagByLayer),RS2::WidthByLayer,RS2::LineByLayer));
    RS_Line line(&parent,RS_LineData({0,0},{1,0}));line.setLayer(childLayer);
    // each component in turn is the only one that is ByBlock
    line.setPen(RS_Pen(RS_Color(Qt::white),RS2::WidthByBlock,RS2::DotLine));
    auto pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::white));CHECK(pen.getWidth()==RS2::Width07);CHECK(pen.getLineType()==RS2::DotLine);
    line.setPen(RS_Pen(RS_Color(RS2::FlagByBlock),RS2::Width03,RS2::DotLine));
    pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::red));CHECK(pen.getWidth()==RS2::Width03);CHECK(pen.getLineType()==RS2::DotLine);
    line.setPen(RS_Pen(RS_Color(Qt::white),RS2::Width03,RS2::LineByBlock));
    pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::white));CHECK(pen.getWidth()==RS2::Width03);CHECK(pen.getLineType()==RS2::DashLine);
    // a component that is ByLayer itself stays with the entity's own layer
    line.setPen(RS_Pen(RS_Color(RS2::FlagByLayer),RS2::WidthByBlock,RS2::LineByLayer));
    pen=line.getPen();
    CHECK(pen.getColor()==RS_Color(Qt::green));CHECK(pen.getWidth()==RS2::Width07);CHECK(pen.getLineType()==RS2::SolidLine);
}

TEST_CASE("An ancestor without a layer leaves the entity's own layer in charge", "[pen][native-prerequisite]") {
    initializeSettings();
    RS_Graphic graphic;
    auto *childLayer=new RS_Layer(QStringLiteral("child-pen"));
    childLayer->setPen(RS_Pen(RS_Color(Qt::green),RS2::Width11,RS2::SolidLine));
    graphic.getLayerList()->add(childLayer);
    RS_EntityContainer parent(&graphic,false);
    parent.setPen(RS_Pen(RS_Color(RS2::FlagByLayer),RS2::WidthByLayer,RS2::LineByLayer));
    parent.setLayer(nullptr); // a new entity is put on the active layer
    REQUIRE(parent.getLayer(false)==nullptr);
    RS_Line line(&parent,RS_LineData({0,0},{1,0}));line.setLayer(childLayer);
    for (const RS_Pen &own:{RS_Pen(RS_Color(RS2::FlagByBlock),RS2::WidthByBlock,RS2::LineByBlock),RS_Pen(RS2::FlagInvalid)}) {
        line.setPen(own);
        const auto pen=line.getPen();
        CHECK(pen.getColor()==RS_Color(Qt::green));CHECK(pen.getWidth()==RS2::Width11);
        CHECK(pen.getLineType()==RS2::SolidLine);
    }
    // without any layer the components stay what the ancestor said
    line.setLayer(nullptr);
    line.setPen(RS_Pen(RS_Color(RS2::FlagByBlock),RS2::WidthByBlock,RS2::LineByBlock));
    const auto pen=line.getPen();
    CHECK(pen.isColorByLayer());CHECK(pen.isWidthByLayer());CHECK(pen.isLineTypeByLayer());
}
