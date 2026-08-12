/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include <QCoreApplication>

#include "lc_insert_transform.h"
#include "lc_wipeout.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_blocklistlistener.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_layerlistlistener.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_settings.h"

namespace {

void ensureTestApp() {
  static int argc = 1;
  static char arg0[] = "librecad_tests";
  static char *argv[] = {arg0, nullptr};
  static QCoreApplication *app = QCoreApplication::instance()
                                   ? QCoreApplication::instance()
                                   : new QCoreApplication(argc, argv);
  static const bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)app;
  (void)settingsReady;
}

RS_InsertData insertData(const QString &name, const RS_Vector &point,
                         const RS_Vector &scale = RS_Vector(1.0, 1.0),
                         double angle = 0.0) {
  return RS_InsertData(name, point, scale, angle, 1, 1, RS_Vector(0.0, 0.0));
}

RS_Line *onlyLine(RS_Insert &insert) {
  REQUIRE(insert.count() == 1);
  auto *line = dynamic_cast<RS_Line *>(insert.entityAt(0));
  REQUIRE(line != nullptr);
  return line;
}

class BlockToggleListener final : public RS_BlockListListener {
public:
  void blockToggled(RS_Block *block) override {
    ++count;
    lastBlock = block;
  }

  int count = 0;
  RS_Block *lastBlock = nullptr;
};

class LayerToggleListener final : public RS_LayerListListener {
public:
  void layerToggled(RS_Layer *layer) override {
    ++count;
    lastLayer = layer;
  }

  int count = 0;
  RS_Layer *lastLayer = nullptr;
};

class LayerRemovalListener final : public RS_LayerListListener {
public:
  LayerRemovalListener(RS_Graphic &graphic, QString expectedName)
      : graphic(graphic), expectedName(std::move(expectedName)) {}

  void layerRemoved(RS_Layer *layer) override {
    removedExpectedLayer = layer != nullptr && layer->getName() == expectedName;
    activeDuringRemoval = graphic.getActiveLayer();
  }

  RS_Graphic &graphic;
  QString expectedName;
  bool removedExpectedLayer = false;
  RS_Layer *activeDuringRemoval = nullptr;
};

} // namespace

TEST_CASE("INSERT uses its supplied parent and field transform",
          "[block-insert][insert-parent][insert-transform]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("BASE"), RS_Vector(10.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(10.0, 0.0),
                                                  RS_Vector(20.0, 0.0))));
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("BASE"),
                                        RS_Vector(100.0, 200.0), RS_Vector(2.0, 3.0)));
  REQUIRE(insert.getParent() == &graphic);
  const RS_Line *line = onlyLine(insert);
  CHECK(line->getStartpoint().x == Catch::Approx(100.0));
  CHECK(line->getStartpoint().y == Catch::Approx(200.0));
  CHECK(line->getEndpoint().x == Catch::Approx(120.0));
  CHECK(line->getEndpoint().y == Catch::Approx(200.0));

  // INSERT expansion owns clones; updating it must not mutate the definition.
  const RS_Line *source = dynamic_cast<const RS_Line *>(block->entityAt(0));
  REQUIRE(source != nullptr);
  CHECK(source->getStartpoint().x == Catch::Approx(10.0));
  CHECK(source->getEndpoint().x == Catch::Approx(20.0));

  insert.update();
  line = onlyLine(insert);
  CHECK(line->getStartpoint().x == Catch::Approx(100.0));
  CHECK(line->getEndpoint().x == Catch::Approx(120.0));
}

TEST_CASE("INSERT transform decomposition is typed and preserves leaf update state",
          "[block-insert][insert-transform]") {
  ensureTestApp();

  LC_InsertTransform transform;
  transform.a = 0.0;
  transform.b = 2.0;
  transform.c = 3.0;
  transform.d = 0.0;
  LC_InsertTransformParts parts;
  REQUIRE(transform.decompose(parts) == LC_InsertTransformDecompositionStatus::Ok);
  CHECK(parts.scaleX == Catch::Approx(2.0));
  CHECK(parts.scaleY == Catch::Approx(-3.0));
  CHECK(parts.reversesOrientation);

    transform.d = 1.0;
  CHECK(transform.decompose(parts) == LC_InsertTransformDecompositionStatus::Shear);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("UPDATE-STATE"),
                                          RS_Vector(0.0, 0.0), false));
  auto *source = new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0),
                                                RS_Vector(1.0, 0.0)));
  source->setUpdateEnabled(false);
  block->addEntity(source);
  graphic.addBlock(block);

  RS_Insert insert(&graphic,
                   insertData(QStringLiteral("UPDATE-STATE"), RS_Vector(2.0, 3.0),
                              RS_Vector(2.0, -3.0)));
  const auto *derived = onlyLine(insert);
  CHECK_FALSE(derived->isUpdateEnabled());
}

TEST_CASE("INSERT name change invalidates the resolved block cache",
          "[block-insert][insert-cache]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *first = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("FIRST"), RS_Vector(0.0, 0.0), false));
  first->addEntity(new RS_Line(first, RS_LineData(RS_Vector(0.0, 0.0),
                                                  RS_Vector(1.0, 0.0))));
  graphic.addBlock(first);
  auto *second = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("SECOND"), RS_Vector(0.0, 0.0), false));
  second->addEntity(new RS_Line(second, RS_LineData(RS_Vector(10.0, 0.0),
                                                    RS_Vector(12.0, 0.0))));
  graphic.addBlock(second);

  RS_Insert insert(&graphic, insertData(QStringLiteral("FIRST"), RS_Vector(0.0, 0.0)));
  REQUIRE(onlyLine(insert)->getEndpoint().x == Catch::Approx(1.0));

  insert.setName(QStringLiteral("SECOND"));
  REQUIRE(insert.getBlockForInsert() == second);
  CHECK(onlyLine(insert)->getStartpoint().x == Catch::Approx(10.0));
  CHECK(onlyLine(insert)->getEndpoint().x == Catch::Approx(12.0));
}

TEST_CASE("INSERT re-resolves a removed block instead of retaining a dangling cache",
          "[block-insert][insert-cache]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("TRANSIENT"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0),
                                                  RS_Vector(1.0, 0.0))));
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("TRANSIENT"), RS_Vector(0.0, 0.0)));
  REQUIRE(insert.getBlockForInsert() == block);
  REQUIRE(insert.count() == 1);

  graphic.getBlockList()->remove(block);
  CHECK(insert.getBlockForInsert() == nullptr);
  insert.update();
  CHECK(insert.count() == 0);
  CHECK_FALSE(insert.getMin().valid);
}

TEST_CASE("Nested INSERT arrays compose fields without coordinate repair",
          "[block-insert][insert-nested][insert-array]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("LEAF"), RS_Vector(250000.0, 0.0), false));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(250000.0, 0.0),
                                                RS_Vector(250010.0, 0.0))));
  graphic.addBlock(leaf);

  auto *parent = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("PARENT"), RS_Vector(0.0, 0.0), false));
  graphic.addBlock(parent);
  RS_InsertData childData(QStringLiteral("LEAF"), RS_Vector(5.0, 0.0),
                          RS_Vector(1.0, 1.0), 0.0, 2, 1, RS_Vector(20.0, 0.0));
  parent->addEntity(new RS_Insert(parent, childData));

  RS_Insert insert(&graphic, insertData(QStringLiteral("PARENT"), RS_Vector(100.0, 0.0)));
  REQUIRE(insert.count() == 2);
  const auto *first = dynamic_cast<const RS_Line *>(insert.entityAt(0));
  const auto *second = dynamic_cast<const RS_Line *>(insert.entityAt(1));
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  CHECK(first->getStartpoint().x == Catch::Approx(105.0));
  CHECK(second->getStartpoint().x == Catch::Approx(125.0));

  const auto *source = dynamic_cast<const RS_Line *>(leaf->entityAt(0));
  REQUIRE(source != nullptr);
  CHECK(source->getStartpoint().x == Catch::Approx(250000.0));
  CHECK(source->getEndpoint().x == Catch::Approx(250010.0));
}

TEST_CASE("INSERT handles axial extrusion and rejects non-planar OCS",
          "[block-insert][insert-extrusion]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("AXIAL"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(1.0, 0.0),
                                                  RS_Vector(2.0, 0.0))));
  graphic.addBlock(block);

  RS_InsertData plusData = insertData(QStringLiteral("AXIAL"), RS_Vector(10.0, 0.0));
  plusData.extrusion = RS_Vector(0.0, 0.0, 1.0);
  RS_Insert plus(&graphic, plusData);
  const RS_Line *plusLine = onlyLine(plus);
  CHECK(plusLine->getStartpoint().x == Catch::Approx(11.0));
  CHECK(plusLine->getEndpoint().x == Catch::Approx(12.0));

  RS_InsertData minusData = insertData(QStringLiteral("AXIAL"), RS_Vector(10.0, 0.0));
  minusData.extrusion = RS_Vector(0.0, 0.0, -1.0);
  RS_Insert minus(&graphic, minusData);
  const RS_Line *minusLine = onlyLine(minus);
  CHECK(minusLine->getStartpoint().x == Catch::Approx(-11.0));
  CHECK(minusLine->getEndpoint().x == Catch::Approx(-12.0));

  RS_InsertData nonPlanarData = insertData(QStringLiteral("AXIAL"), RS_Vector(10.0, 0.0));
  nonPlanarData.extrusion = RS_Vector(1.0, 0.0, 1.0);
  RS_Insert nonPlanar(&graphic, nonPlanarData);
  CHECK(nonPlanar.count() == 0);
  CHECK_FALSE(nonPlanar.getMin().valid);
}

TEST_CASE("INSERT axial OCS maps base point, rotation, and insertion point",
          "[block-insert][insert-extrusion][insert-transform]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("OCS"), RS_Vector(1.0, 2.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(3.0, 4.0),
                                                  RS_Vector(4.0, 4.0))));
  graphic.addBlock(block);

  RS_InsertData plusData = insertData(QStringLiteral("OCS"), RS_Vector(10.0, 20.0),
                                      RS_Vector(2.0, 3.0), std::atan2(1.0, 0.0));
  plusData.extrusion = RS_Vector(0.0, 0.0, 1.0);
  RS_Insert plus(&graphic, plusData);
  const RS_Line *plusLine = onlyLine(plus);
  CHECK(plusLine->getStartpoint().x == Catch::Approx(4.0));
  CHECK(plusLine->getStartpoint().y == Catch::Approx(24.0));
  CHECK(plusLine->getEndpoint().x == Catch::Approx(4.0));
  CHECK(plusLine->getEndpoint().y == Catch::Approx(26.0));

  RS_InsertData minusData = plusData;
  minusData.extrusion = RS_Vector(0.0, 0.0, -1.0);
  RS_Insert minus(&graphic, minusData);
  const RS_Line *minusLine = onlyLine(minus);
  CHECK(minusLine->getStartpoint().x == Catch::Approx(-4.0));
  CHECK(minusLine->getStartpoint().y == Catch::Approx(24.0));
  CHECK(minusLine->getEndpoint().x == Catch::Approx(-4.0));
  CHECK(minusLine->getEndpoint().y == Catch::Approx(26.0));
}

TEST_CASE("INSERT data defaults to one ordinary instance", "[block-insert][insert-array]") {
  const RS_InsertData data;
  CHECK(data.cols == 1);
  CHECK(data.rows == 1);
}

TEST_CASE("INSERT scale preserves stored Z metadata for a 2D factor",
          "[block-insert][insert-transform]") {
  RS_InsertData data(QStringLiteral("UNRESOLVED"), RS_Vector(2.0, 3.0),
                     RS_Vector(4.0, 5.0, 6.0), 0.0, 1, 1,
                     RS_Vector(7.0, 8.0, 9.0));
  RS_Insert insert(nullptr, data);
  insert.scale(RS_Vector(0.0, 0.0), RS_Vector(2.0, 3.0));

  const RS_InsertData scaled = insert.getData();
  CHECK(scaled.scaleFactor.x == Catch::Approx(8.0));
  CHECK(scaled.scaleFactor.y == Catch::Approx(15.0));
  CHECK(scaled.scaleFactor.z == Catch::Approx(6.0));
  CHECK(scaled.spacing.x == Catch::Approx(14.0));
  CHECK(scaled.spacing.y == Catch::Approx(24.0));
  CHECK(scaled.spacing.z == Catch::Approx(9.0));
  CHECK(scaled.insertionPoint.x == Catch::Approx(4.0));
  CHECK(scaled.insertionPoint.y == Catch::Approx(9.0));
}

TEST_CASE("INSERT source edits preserve axial OCS placement",
          "[block-insert][insert-transform][insert-extrusion]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("SOURCE-OCS"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(1.0, 0.0),
                                                  RS_Vector(2.0, 0.0))));
  graphic.addBlock(block);

  RS_InsertData data = insertData(QStringLiteral("SOURCE-OCS"), RS_Vector(10.0, 0.0));
  data.extrusion = RS_Vector(0.0, 0.0, -1.0);
  RS_Insert insert(&graphic, data);
  CHECK(onlyLine(insert)->getStartpoint().x == Catch::Approx(-11.0));

  // A drawing-plane move right becomes a negative OCS X adjustment for the
  // axial -Z arbitrary-axis frame. The visible geometry must still move right.
  insert.move(RS_Vector(5.0, 0.0));
  const RS_InsertData moved = insert.getData();
  CHECK(moved.insertionPoint.x == Catch::Approx(5.0));
  CHECK(moved.insertionPoint.y == Catch::Approx(0.0));
  CHECK(moved.extrusion.z == Catch::Approx(-1.0));
  CHECK(onlyLine(insert)->getStartpoint().x == Catch::Approx(-6.0));
}

TEST_CASE("INSERT source edits reject a world-space shear",
          "[block-insert][insert-transform]") {
  ensureTestApp();
  const double quarterTurn = std::atan2(1.0, 1.0);
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("SOURCE-SHEAR"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(1.0, 0.0),
                                                  RS_Vector(2.0, 0.0))));
  graphic.addBlock(block);

  RS_InsertData data = insertData(QStringLiteral("SOURCE-SHEAR"), RS_Vector(10.0, 20.0),
                                  RS_Vector(2.0, 3.0), quarterTurn);
  data.cols = 2;
  data.spacing = RS_Vector(4.0, 5.0);
  RS_Insert insert(&graphic, data);
  REQUIRE(insert.count() == 2);
  const auto *before = dynamic_cast<const RS_Line *>(insert.entityAt(0));
  REQUIRE(before != nullptr);
  const RS_Vector beforeStart = before->getStartpoint();
  const RS_InsertData beforeData = insert.getData();

  // diag(2,3) after a 45-degree INSERT rotation has non-orthogonal columns;
  // it cannot be represented by INSERT rotation plus diagonal scales.
  LC_InsertTransform edit;
  REQUIRE(LC_InsertTransform::scale(RS_Vector(0.0, 0.0), RS_Vector(2.0, 3.0), edit));
  RS_InsertData transformed;
  CHECK(lcApplyInsertSourceEdit(beforeData, edit, transformed)
        == LC_InsertSourceEditStatus::Unrepresentable);

  RS_ScaleData modificationData;
  modificationData.referencePoint = RS_Vector(0.0, 0.0);
  modificationData.factor = RS_Vector(2.0, 3.0);
  modificationData.isotropicScaling = false;
  LC_DocumentModificationBatch batch;
  QList<RS_Entity*> selection {&insert};
  CHECK_FALSE(RS_Modification::scale(modificationData, selection, false, batch));
  CHECK(batch.entitiesToAdd.isEmpty());
  CHECK(batch.entitiesToDelete.isEmpty());

  insert.scale(RS_Vector(0.0, 0.0), RS_Vector(2.0, 3.0));

  const RS_InsertData afterData = insert.getData();
  CHECK(afterData.insertionPoint.x == Catch::Approx(beforeData.insertionPoint.x));
  CHECK(afterData.insertionPoint.y == Catch::Approx(beforeData.insertionPoint.y));
  CHECK(afterData.scaleFactor.x == Catch::Approx(beforeData.scaleFactor.x));
  CHECK(afterData.scaleFactor.y == Catch::Approx(beforeData.scaleFactor.y));
  CHECK(afterData.angle == Catch::Approx(beforeData.angle));
  CHECK(afterData.spacing.x == Catch::Approx(beforeData.spacing.x));
  CHECK(afterData.spacing.y == Catch::Approx(beforeData.spacing.y));
  CHECK(insert.count() == 2);
  const auto *after = dynamic_cast<const RS_Line *>(insert.entityAt(0));
  REQUIRE(after != nullptr);
  CHECK(after->getStartpoint().x == Catch::Approx(beforeStart.x));
  CHECK(after->getStartpoint().y == Catch::Approx(beforeStart.y));
}

TEST_CASE("INSERT source reflection decomposes to native fields",
          "[block-insert][insert-transform]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("SOURCE-MIRROR"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(1.0, 2.0),
                                                  RS_Vector(2.0, 2.0))));
  graphic.addBlock(block);

  RS_InsertData data = insertData(QStringLiteral("SOURCE-MIRROR"), RS_Vector(10.0, 20.0),
                                  RS_Vector(2.0, 3.0));
  RS_Insert insert(&graphic, data);
  insert.mirror(RS_Vector(0.0, 0.0), RS_Vector(0.0, 1.0));

  const RS_InsertData reflected = insert.getData();
  CHECK(reflected.insertionPoint.x == Catch::Approx(-10.0));
  CHECK(reflected.insertionPoint.y == Catch::Approx(20.0));
  CHECK(reflected.scaleFactor.x == Catch::Approx(2.0));
  CHECK(reflected.scaleFactor.y == Catch::Approx(-3.0));
  const RS_Line *line = onlyLine(insert);
  CHECK(line->getStartpoint().x == Catch::Approx(-12.0));
  CHECK(line->getStartpoint().y == Catch::Approx(26.0));
}

TEST_CASE("MINSERT rotates spacing without applying INSERT scale",
          "[block-insert][insert-extrusion][insert-array]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("REFLECTED"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Line(block, RS_LineData(RS_Vector(1.0, 0.0),
                                                  RS_Vector(2.0, 0.0))));
  graphic.addBlock(block);

  RS_InsertData data(QStringLiteral("REFLECTED"), RS_Vector(10.0, 0.0),
                     RS_Vector(2.0, 1.0), 0.0, 2, 1, RS_Vector(5.0, 0.0));
  data.extrusion = RS_Vector(0.0, 0.0, -1.0);
  RS_Insert insert(&graphic, data);

  REQUIRE(insert.count() == 2);
  const auto *first = dynamic_cast<const RS_Line *>(insert.entityAt(0));
  const auto *second = dynamic_cast<const RS_Line *>(insert.entityAt(1));
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  CHECK(first->getStartpoint().x == Catch::Approx(-12.0));
  CHECK(second->getStartpoint().x == Catch::Approx(-17.0));
}

TEST_CASE("INSERT preserves source attributes when a circle becomes an ellipse",
          "[block-insert][insert-transform][insert-attributes]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("CIRCLE"), RS_Vector(0.0, 0.0), false));
  auto *circle = new RS_Circle(block, RS_CircleData(RS_Vector(1.0, 2.0), 3.0));
  const RS_Pen sourcePen(RS_Color(12, 34, 56), RS2::Width05, RS2::DashLine);
  circle->setPen(sourcePen);
  block->addEntity(circle);
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("CIRCLE"), RS_Vector(0.0, 0.0),
                                        RS_Vector(2.0, 3.0)));
  REQUIRE(insert.count() == 1);
  const auto *ellipse = dynamic_cast<const RS_Ellipse *>(insert.entityAt(0));
  REQUIRE(ellipse != nullptr);
  CHECK(ellipse->getPen(false).getColor() == sourcePen.getColor());
  CHECK(ellipse->getPen(false).getWidth() == sourcePen.getWidth());
  CHECK(ellipse->getPen(false).getLineType() == sourcePen.getLineType());
}

TEST_CASE("INSERT preserves reflected arc sweep through non-uniform transforms",
          "[block-insert][insert-transform][insert-arc]") {
  ensureTestApp();
  const double halfTurn = std::acos(-1.0);
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("ARC-REFLECTION"),
                                          RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Arc(block,
                              RS_ArcData(RS_Vector(1.0, 2.0), 2.0, 0.0,
                                         halfTurn / 2.0, false)));
  graphic.addBlock(block);

  RS_Insert reflected(&graphic,
                      insertData(QStringLiteral("ARC-REFLECTION"), RS_Vector(10.0, 20.0),
                                 RS_Vector(2.0, -3.0), halfTurn / 2.0));
  const auto *arc = dynamic_cast<const RS_Ellipse *>(reflected.entityAt(0));
  REQUIRE(arc != nullptr);
  CHECK(arc->isReversed());
  CHECK(arc->getCenter().x == Catch::Approx(16.0));
  CHECK(arc->getCenter().y == Catch::Approx(22.0));
  CHECK(arc->getStartpoint().x == Catch::Approx(16.0));
  CHECK(arc->getStartpoint().y == Catch::Approx(26.0));
  CHECK(arc->getEndpoint().x == Catch::Approx(22.0));
  CHECK(arc->getEndpoint().y == Catch::Approx(22.0));
}

TEST_CASE("INSERT applies non-uniform transforms to both image frame vectors",
          "[block-insert][insert-transform][insert-image]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("IMAGE-FRAME"),
                                          RS_Vector(0.0, 0.0), false));
  const RS_ImageData imageData(0, RS_Vector(1.0, 2.0), RS_Vector(1.0, 2.0),
                               RS_Vector(-3.0, 4.0), RS_Vector(2.0, 3.0),
                               QString(), 50, 50, 0);
  block->addEntity(new RS_Image(block, imageData));
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("IMAGE-FRAME"),
                                        RS_Vector(10.0, 20.0),
                                        RS_Vector(2.0, 3.0),
                                        std::atan2(1.0, 0.0)));
  REQUIRE(insert.count() == 1);
  const auto *image = dynamic_cast<const RS_Image *>(insert.entityAt(0));
  REQUIRE(image != nullptr);
  CHECK(image->getInsertionPoint().x == Catch::Approx(4.0));
  CHECK(image->getInsertionPoint().y == Catch::Approx(22.0));
  CHECK(image->getUVector().x == Catch::Approx(-6.0));
  CHECK(image->getUVector().y == Catch::Approx(2.0));
  CHECK(image->getVVector().x == Catch::Approx(-12.0));
  CHECK(image->getVVector().y == Catch::Approx(-6.0));

  RS_Insert reflected(&graphic, insertData(QStringLiteral("IMAGE-FRAME"),
                                           RS_Vector(10.0, 20.0),
                                           RS_Vector(2.0, -3.0)));
  const auto *reflectedImage = dynamic_cast<const RS_Image *>(reflected.entityAt(0));
  REQUIRE(reflectedImage != nullptr);
  CHECK(reflectedImage->getInsertionPoint().x == Catch::Approx(12.0));
  CHECK(reflectedImage->getInsertionPoint().y == Catch::Approx(14.0));
  CHECK(reflectedImage->getUVector().x == Catch::Approx(2.0));
  CHECK(reflectedImage->getUVector().y == Catch::Approx(-6.0));
  CHECK(reflectedImage->getVVector().x == Catch::Approx(-6.0));
  CHECK(reflectedImage->getVVector().y == Catch::Approx(-12.0));
}

TEST_CASE("INSERT applies non-uniform transforms to WIPEOUT native frames",
          "[block-insert][insert-transform][insert-wipeout]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("WIPEOUT-FRAME"),
                                          RS_Vector(0.0, 0.0), false));
  LC_WipeoutData data;
  data.hasNativeFrame = true;
  data.insertionPoint = RS_Vector(1.0, 2.0);
  data.uPixel = RS_Vector(0.5, 1.0);
  data.vPixel = RS_Vector(-1.0, 0.25);
  data.sizeU = 2.0;
  data.sizeV = 3.0;
  data.clipPath = {RS_Vector(-0.5, -0.5), RS_Vector(1.5, -0.5),
                   RS_Vector(1.5, 2.5), RS_Vector(-0.5, 2.5)};
  block->addEntity(new LC_Wipeout(block, data));
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("WIPEOUT-FRAME"),
                                        RS_Vector(10.0, 20.0),
                                        RS_Vector(2.0, 3.0),
                                        std::atan2(1.0, 0.0)));
  REQUIRE(insert.count() == 1);
  const auto *wipeout = dynamic_cast<const LC_Wipeout *>(insert.entityAt(0));
  REQUIRE(wipeout != nullptr);
  const LC_WipeoutData &transformed = wipeout->getData();
  CHECK(transformed.insertionPoint.x == Catch::Approx(4.0));
  CHECK(transformed.insertionPoint.y == Catch::Approx(22.0));
  CHECK(transformed.uPixel.x == Catch::Approx(-3.0));
  CHECK(transformed.uPixel.y == Catch::Approx(1.0));
  CHECK(transformed.vPixel.x == Catch::Approx(-0.75));
  CHECK(transformed.vPixel.y == Catch::Approx(-2.0));

  RS_Insert reflected(&graphic, insertData(QStringLiteral("WIPEOUT-FRAME"),
                                           RS_Vector(10.0, 20.0),
                                           RS_Vector(2.0, -3.0)));
  const auto *reflectedWipeout = dynamic_cast<const LC_Wipeout *>(reflected.entityAt(0));
  REQUIRE(reflectedWipeout != nullptr);
  const LC_WipeoutData &reflectedData = reflectedWipeout->getData();
  CHECK(reflectedData.insertionPoint.x == Catch::Approx(12.0));
  CHECK(reflectedData.insertionPoint.y == Catch::Approx(14.0));
  CHECK(reflectedData.uPixel.x == Catch::Approx(1.0));
  CHECK(reflectedData.uPixel.y == Catch::Approx(-3.0));
  CHECK(reflectedData.vPixel.x == Catch::Approx(-2.0));
  CHECK(reflectedData.vPixel.y == Catch::Approx(-0.75));
}

TEST_CASE("nested INSERT resolves BYBLOCK attributes at the nearest owner",
          "[block-insert][insert-attributes]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("LEAF-PEN"), RS_Vector(0.0, 0.0), false));
  auto *line = new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0)));
  line->setPen(RS_Pen(RS2::FlagByBlock, RS2::WidthByBlock, RS2::LineByBlock));
  leaf->addEntity(line);
  graphic.addBlock(leaf);

  auto *middle = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("MIDDLE-PEN"), RS_Vector(0.0, 0.0), false));
  auto *nested = new RS_Insert(middle, insertData(QStringLiteral("LEAF-PEN"), RS_Vector(0.0, 0.0)));
  const RS_Pen nestedPen(RS_Color(12, 34, 56), RS2::Width05, RS2::DashLine);
  nested->setPen(nestedPen);
  middle->addEntity(nested);
  graphic.addBlock(middle);

  RS_Insert root(&graphic, insertData(QStringLiteral("MIDDLE-PEN"), RS_Vector(0.0, 0.0)));
  root.setPen(RS_Pen(RS_Color(200, 100, 50), RS2::Width09, RS2::DotLine));
  root.update();

  const auto *expanded = onlyLine(root);
  REQUIRE(expanded != nullptr);
  CHECK(expanded->getPen(false).getColor() == nestedPen.getColor());
  CHECK(expanded->getPen(false).getWidth() == nestedPen.getWidth());
  CHECK(expanded->getPen(false).getLineType() == nestedPen.getLineType());
}

TEST_CASE("INSERT preserves point geometry at the origin",
          "[block-insert][insert-bounds]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("ORIGIN"), RS_Vector(0.0, 0.0), false));
  block->addEntity(new RS_Point(block, RS_Vector(0.0, 0.0)));
  graphic.addBlock(block);

  RS_Insert insert(&graphic, insertData(QStringLiteral("ORIGIN"), RS_Vector(0.0, 0.0)));
  REQUIRE(insert.count() == 1);
  CHECK(insert.getMin().valid);
  CHECK(insert.getMax().valid);
  CHECK(insert.getMin().x == Catch::Approx(0.0));
  CHECK(insert.getMin().y == Catch::Approx(0.0));
  CHECK(insert.getMax().x == Catch::Approx(0.0));
  CHECK(insert.getMax().y == Catch::Approx(0.0));
}

TEST_CASE("INSERT cycle rejects the entire expansion transaction",
          "[block-insert][insert-cycle]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *a = new RS_Block(&graphic, RS_BlockData(QStringLiteral("A"), RS_Vector(0.0, 0.0), false));
  auto *b = new RS_Block(&graphic, RS_BlockData(QStringLiteral("B"), RS_Vector(0.0, 0.0), false));
  graphic.addBlock(a);
  graphic.addBlock(b);
  a->addEntity(new RS_Line(a, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  a->addEntity(new RS_Insert(a, insertData(QStringLiteral("B"), RS_Vector(0.0, 0.0))));
  b->addEntity(new RS_Insert(b, insertData(QStringLiteral("A"), RS_Vector(0.0, 0.0))));

  const RS_Block* const aConst = a;
  const QStringList cyclePath = aConst->findNestedInsert(QStringLiteral("A"));
  REQUIRE(cyclePath.size() == 2);
  CHECK(cyclePath.at(0) == QStringLiteral("A"));
  CHECK(cyclePath.at(1) == QStringLiteral("B"));
  CHECK(aConst->findNestedInsert(QStringLiteral("MISSING")).empty());

  RS_Insert insert(&graphic, insertData(QStringLiteral("A"), RS_Vector(0.0, 0.0)));
  CHECK(insert.count() == 0);
  CHECK_FALSE(insert.getMin().valid);
}

TEST_CASE("BLOCK nested-insert lookup is iterative and retains its full owner path",
          "[block-insert][block-graph]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  constexpr int blockDepth = 1024;
  std::vector<RS_Block *> blocks;
  blocks.reserve(blockDepth);
  for (int index = 0; index < blockDepth; ++index) {
    auto *block = new RS_Block(&graphic,
                               RS_BlockData(QStringLiteral("PATH-%1").arg(index),
                                            RS_Vector(0.0, 0.0), false));
    graphic.addBlock(block);
    blocks.push_back(block);
  }
  for (int index = 0; index < blockDepth; ++index) {
    const QString childName = index + 1 < blockDepth
        ? blocks.at(index + 1)->getName()
        : QStringLiteral("TARGET");
    RS_InsertData data = insertData(childName, RS_Vector(0.0, 0.0));
    data.updateMode = RS2::NoUpdate;
    blocks.at(index)->addEntity(new RS_Insert(blocks.at(index), data));
  }

  const QStringList path = blocks.front()->findNestedInsert(QStringLiteral("TARGET"));
  REQUIRE(path.size() == blockDepth);
  CHECK(path.front() == blocks.front()->getName());
  CHECK(path.back() == blocks.back()->getName());
}

TEST_CASE("INSERT rejects non-finite composed transforms transactionally",
          "[block-insert][insert-transform]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("OVERFLOW-LEAF"),
                                         RS_Vector(0.0, 0.0), false));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0),
                                                RS_Vector(1.0, 0.0))));
  graphic.addBlock(leaf);

  auto *middle = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("OVERFLOW-MIDDLE"),
                                           RS_Vector(0.0, 0.0), false));
  RS_InsertData nested = insertData(QStringLiteral("OVERFLOW-LEAF"),
                                    RS_Vector(0.0, 0.0),
                                    RS_Vector(std::numeric_limits<double>::max(),
                                              std::numeric_limits<double>::max()));
  nested.updateMode = RS2::NoUpdate;
  middle->addEntity(new RS_Insert(middle, nested));
  graphic.addBlock(middle);

  RS_Insert root(&graphic, insertData(QStringLiteral("OVERFLOW-MIDDLE"),
                                      RS_Vector(0.0, 0.0),
                                      RS_Vector(std::numeric_limits<double>::max(),
                                                std::numeric_limits<double>::max())));
  CHECK(root.count() == 0);
  CHECK_FALSE(root.getMin().valid);
}

TEST_CASE("INSERT expansion budgets reject partial output",
          "[block-insert][insert-budget]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("BUDGET-LEAF"), RS_Vector(0.0, 0.0), false));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(2.0, 0.0), RS_Vector(3.0, 0.0))));
  graphic.addBlock(leaf);

  RS_InsertData leafData = insertData(QStringLiteral("BUDGET-LEAF"), RS_Vector(0.0, 0.0));
  leafData.updateMode = RS2::NoUpdate;
  RS_Insert entityBudget(&graphic, leafData);
  entityBudget.update(RS_InsertExpansionBudget{8U, 1U, 8U});
  CHECK(entityBudget.count() == 0);
  CHECK_FALSE(entityBudget.getMin().valid);

  auto *middle = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("BUDGET-MIDDLE"), RS_Vector(0.0, 0.0), false));
  middle->addEntity(new RS_Insert(middle, insertData(QStringLiteral("BUDGET-LEAF"),
                                                       RS_Vector(0.0, 0.0))));
  graphic.addBlock(middle);

  RS_InsertData middleData = insertData(QStringLiteral("BUDGET-MIDDLE"), RS_Vector(0.0, 0.0));
  middleData.updateMode = RS2::NoUpdate;
  RS_Insert depthBudget(&graphic, middleData);
  depthBudget.update(RS_InsertExpansionBudget{1U, 8U, 8U});
  CHECK(depthBudget.count() == 0);
  CHECK_FALSE(depthBudget.getMin().valid);

  auto *empty = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("BUDGET-EMPTY"),
                                          RS_Vector(0.0, 0.0), false));
  graphic.addBlock(empty);
  RS_InsertData oversizedGrid = insertData(QStringLiteral("BUDGET-EMPTY"),
                                           RS_Vector(0.0, 0.0));
  oversizedGrid.cols = 3;
  oversizedGrid.updateMode = RS2::NoUpdate;
  RS_Insert arrayBudget(&graphic, oversizedGrid);
  arrayBudget.update(RS_InsertExpansionBudget{8U, 8U, 2U});
  CHECK(arrayBudget.count() == 0);
  CHECK_FALSE(arrayBudget.getMin().valid);

  auto *arrayLeaf = new RS_Block(&graphic,
                                 RS_BlockData(QStringLiteral("BUDGET-ARRAY-LEAF"),
                                              RS_Vector(0.0, 0.0), false));
  arrayLeaf->addEntity(new RS_Line(arrayLeaf,
                                   RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  graphic.addBlock(arrayLeaf);
  auto *arrayMiddle = new RS_Block(&graphic,
                                   RS_BlockData(QStringLiteral("BUDGET-ARRAY-MIDDLE"),
                                                RS_Vector(0.0, 0.0), false));
  RS_InsertData nestedGrid = insertData(QStringLiteral("BUDGET-ARRAY-LEAF"),
                                        RS_Vector(0.0, 0.0));
  nestedGrid.cols = 2;
  nestedGrid.updateMode = RS2::NoUpdate;
  arrayMiddle->addEntity(new RS_Insert(arrayMiddle, nestedGrid));
  graphic.addBlock(arrayMiddle);
  RS_InsertData rootGrid = insertData(QStringLiteral("BUDGET-ARRAY-MIDDLE"),
                                      RS_Vector(0.0, 0.0));
  rootGrid.cols = 2;
  rootGrid.updateMode = RS2::NoUpdate;
  RS_Insert nestedArrayBudget(&graphic, rootGrid);
  nestedArrayBudget.update(RS_InsertExpansionBudget{8U, 8U, 3U});
  CHECK(nestedArrayBudget.count() == 0);
  CHECK_FALSE(nestedArrayBudget.getMin().valid);

  RS_InsertData boundaryGrid = insertData(QStringLiteral("BUDGET-ARRAY-LEAF"),
                                          RS_Vector(0.0, 0.0));
  boundaryGrid.cols = 2;
  boundaryGrid.spacing = RS_Vector(4.0, 0.0);
  boundaryGrid.updateMode = RS2::NoUpdate;
  RS_Insert exactArrayBudget(&graphic, boundaryGrid);
  exactArrayBudget.update(RS_InsertExpansionBudget{8U, 2U, 2U});
  REQUIRE(exactArrayBudget.count() == 2);
  const auto *firstArrayLine = dynamic_cast<const RS_Line *>(exactArrayBudget.entityAt(0));
  const auto *secondArrayLine = dynamic_cast<const RS_Line *>(exactArrayBudget.entityAt(1));
  REQUIRE(firstArrayLine != nullptr);
  REQUIRE(secondArrayLine != nullptr);
  CHECK(firstArrayLine->getStartpoint().x == Catch::Approx(0.0));
  CHECK(secondArrayLine->getStartpoint().x == Catch::Approx(4.0));
}

TEST_CASE("INSERT propagates frozen nested block visibility",
          "[block-insert][insert-visibility]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("FROZEN-LEAF"), RS_Vector(0.0, 0.0), true));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  graphic.addBlock(leaf);

  auto *middle = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("FROZEN-MIDDLE"), RS_Vector(0.0, 0.0), false));
  middle->addEntity(new RS_Insert(middle, insertData(QStringLiteral("FROZEN-LEAF"),
                                                       RS_Vector(0.0, 0.0))));
  graphic.addBlock(middle);

  RS_Insert insert(&graphic, insertData(QStringLiteral("FROZEN-MIDDLE"), RS_Vector(0.0, 0.0)));
  const RS_Line *line = onlyLine(insert);
  CHECK_FALSE(line->getFlag(RS2::FlagVisible));
}

TEST_CASE("Block visibility changes refresh expanded nested INSERT children",
          "[block-insert][insert-visibility]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("VISIBLE-LEAF"), RS_Vector(0.0, 0.0), false));
  leaf->addEntity(new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  graphic.addBlock(leaf);

  auto *middle = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("VISIBLE-MIDDLE"), RS_Vector(0.0, 0.0), false));
  middle->addEntity(new RS_Insert(middle, insertData(QStringLiteral("VISIBLE-LEAF"),
                                                       RS_Vector(0.0, 0.0))));
  graphic.addBlock(middle);

  auto *insert = new RS_Insert(&graphic,
                               insertData(QStringLiteral("VISIBLE-MIDDLE"), RS_Vector(0.0, 0.0)));
  graphic.addEntity(insert);
  RS_Line *line = onlyLine(*insert);
  CHECK(line->getFlag(RS2::FlagVisible));
  CHECK(graphic.getMin().valid);
  CHECK(graphic.getMax().valid);
  CHECK(graphic.getMax().x == Catch::Approx(1.0));

  graphic.select(QList<RS_Entity*>{line});
  REQUIRE(line->isSelected());
  CHECK(graphic.hasSelection());

  graphic.toggleBlock(leaf);
  CHECK_FALSE(onlyLine(*insert)->getFlag(RS2::FlagVisible));
  CHECK_FALSE(graphic.hasSelection());
  CHECK_FALSE(graphic.getMin().valid);
  CHECK_FALSE(graphic.getMax().valid);

  graphic.toggleBlock(leaf);
  line = onlyLine(*insert);
  CHECK(line->getFlag(RS2::FlagVisible));
  CHECK_FALSE(line->isSelected());
  CHECK(graphic.getMin().valid);
  CHECK(graphic.getMax().valid);
  CHECK(graphic.getMax().x == Catch::Approx(1.0));

  graphic.freezeAllBlocks(true);
  CHECK_FALSE(onlyLine(*insert)->getFlag(RS2::FlagVisible));

  graphic.freezeAllBlocks(false);
  CHECK(onlyLine(*insert)->getFlag(RS2::FlagVisible));
}

TEST_CASE("Block visibility batch toggles distinct blocks once",
          "[block-insert][insert-visibility]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();

  auto *first = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("BATCH-FIRST"), RS_Vector(0.0, 0.0), false));
  first->addEntity(new RS_Line(first, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0))));
  graphic.addBlock(first);
  auto *second = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("BATCH-SECOND"), RS_Vector(0.0, 0.0), false));
  second->addEntity(new RS_Line(second, RS_LineData(RS_Vector(10.0, 0.0), RS_Vector(11.0, 0.0))));
  graphic.addBlock(second);

  auto *firstInsert = new RS_Insert(&graphic, insertData(QStringLiteral("BATCH-FIRST"), RS_Vector(0.0, 0.0)));
  auto *secondInsert = new RS_Insert(&graphic, insertData(QStringLiteral("BATCH-SECOND"), RS_Vector(0.0, 0.0)));
  graphic.addEntity(firstInsert);
  graphic.addEntity(secondInsert);
  BlockToggleListener listener;
  graphic.addBlockListListener(&listener);

  graphic.toggleBlocks(QList<RS_Block*>{first, first, second});

  CHECK(first->isFrozen());
  CHECK(second->isFrozen());
  CHECK(listener.count == 1);
  CHECK(listener.lastBlock == nullptr);
  CHECK_FALSE(onlyLine(*firstInsert)->getFlag(RS2::FlagVisible));
  CHECK_FALSE(onlyLine(*secondInsert)->getFlag(RS2::FlagVisible));
  CHECK_FALSE(graphic.getMin().valid);
  CHECK_FALSE(graphic.getMax().valid);
  graphic.removeBlockListListener(&listener);
}

TEST_CASE("Layer visibility refreshes expanded INSERT bounds and selection",
          "[block-insert][layer-visibility]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *detailLayer = new RS_Layer(QStringLiteral("DETAIL"));
  graphic.addLayer(detailLayer);

  auto *block = new RS_Block(&graphic,
                             RS_BlockData(QStringLiteral("LAYERED"), RS_Vector(0.0, 0.0), false));
  auto *source = new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 0.0)));
  source->setLayer(detailLayer);
  block->addEntity(source);
  graphic.addBlock(block);

  auto *insert = new RS_Insert(&graphic, insertData(QStringLiteral("LAYERED"), RS_Vector(0.0, 0.0)));
  graphic.addEntity(insert);
  RS_Line *line = onlyLine(*insert);
  REQUIRE(line->isVisible());
  REQUIRE(graphic.getMax().valid);
  CHECK(graphic.getMax().x == Catch::Approx(1.0));

  graphic.select(QList<RS_Entity*>{line});
  REQUIRE(graphic.hasSelection());

  graphic.toggleLayer(detailLayer);
  CHECK(detailLayer->isFrozen());
  CHECK_FALSE(onlyLine(*insert)->isVisible());
  CHECK_FALSE(graphic.hasSelection());
  CHECK_FALSE(graphic.getMin().valid);
  CHECK_FALSE(graphic.getMax().valid);

  graphic.setFreezeLayers(QList<RS_Layer*>{detailLayer}, {});
  line = onlyLine(*insert);
  CHECK_FALSE(detailLayer->isFrozen());
  CHECK(line->isVisible());
  CHECK_FALSE(line->isSelected());
  CHECK(graphic.getMin().valid);
  CHECK(graphic.getMax().valid);
  CHECK(graphic.getMax().x == Catch::Approx(1.0));

  LayerToggleListener listener;
  graphic.addLayerListListener(&listener);
  graphic.toggleFreezeLayers(QList<RS_Layer*>{detailLayer, detailLayer});
  CHECK(detailLayer->isFrozen());
  CHECK(listener.count == 1);
  CHECK(listener.lastLayer == nullptr);
  CHECK_FALSE(graphic.getMax().valid);

  graphic.freezeAllLayers(false);
  CHECK_FALSE(detailLayer->isFrozen());
  CHECK(graphic.getMax().valid);
  graphic.removeLayerListListener(&listener);
}

TEST_CASE("Current layers remain visible while lock state stays independent", "[layers][active]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *zeroLayer = graphic.findLayer(QStringLiteral("0"));
  REQUIRE(zeroLayer != nullptr);

  auto *detailLayer = new RS_Layer(QStringLiteral("DETAIL"));
  graphic.addLayer(detailLayer);
  REQUIRE(graphic.activateLayer(detailLayer));

  // Locked layers remain valid current layers and can be selected again.
  graphic.toggleLayerLock(detailLayer);
  CHECK(detailLayer->isLocked());
  CHECK(graphic.getActiveLayer() == detailLayer);
  CHECK(graphic.activateLayer(detailLayer, true));
  CHECK(graphic.activateLayer(QStringLiteral("DETAIL"), true));
  auto *lockedLayerLine = new RS_Line(
      &graphic, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(1.0, 1.0)));
  graphic.addEntity(lockedLayerLine);
  CHECK(lockedLayerLine->getLayer() == detailLayer);

  // Hiding the current layer selects a visible fallback. A hidden layer
  // cannot be restored as current through the public activation API.
  graphic.toggleLayer(detailLayer);
  CHECK(detailLayer->isFrozen());
  CHECK(graphic.getActiveLayer() == zeroLayer);
  CHECK_FALSE(graphic.activateLayer(detailLayer, true));
  CHECK_FALSE(graphic.activateLayer(QStringLiteral("DETAIL"), true));
  CHECK_FALSE(graphic.activateLayer(QStringLiteral("MISSING"), true));
  CHECK(graphic.getActiveLayer() == zeroLayer);

  // Lock operations do not alter visibility.
  graphic.toggleLayerLock(detailLayer);
  CHECK_FALSE(detailLayer->isLocked());
  CHECK(detailLayer->isFrozen());

  graphic.toggleLayerLock(detailLayer);
  graphic.toggleLayer(detailLayer);
  REQUIRE(graphic.activateLayer(detailLayer));

  // Locking every layer keeps the current layer locked and current.
  graphic.lockAllLayers(true);
  CHECK(detailLayer->isLocked());
  CHECK(zeroLayer->isLocked());
  CHECK(graphic.getActiveLayer() == detailLayer);

  // Freezing every layer leaves exactly the current layer visible and does
  // not unlock it.
  graphic.freezeAllLayers(true);
  CHECK(graphic.getActiveLayer() == detailLayer);
  CHECK_FALSE(detailLayer->isFrozen());
  CHECK(detailLayer->isLocked());
  CHECK(zeroLayer->isFrozen());
  CHECK(zeroLayer->isLocked());

  // With another visible layer available, hiding the current locked layer
  // switches current layers without changing either lock flag.
  graphic.toggleLayer(zeroLayer);
  graphic.toggleLayer(detailLayer);
  CHECK(graphic.getActiveLayer() == zeroLayer);
  CHECK(detailLayer->isFrozen());
  CHECK(detailLayer->isLocked());
  CHECK(zeroLayer->isLocked());

  RS_Layer foreignLayer(QStringLiteral("FOREIGN"));
  CHECK_FALSE(graphic.activateLayer(&foreignLayer, true));
  CHECK(graphic.getActiveLayer() == zeroLayer);

  // Removing the current layer selects a valid survivor before deleting it.
  graphic.toggleLayer(detailLayer);
  REQUIRE(graphic.activateLayer(detailLayer));
  LayerRemovalListener removalListener(graphic, QStringLiteral("DETAIL"));
  graphic.addLayerListListener(&removalListener);
  graphic.removeLayer(detailLayer);
  CHECK(removalListener.removedExpectedLayer);
  CHECK(removalListener.activeDuringRemoval == zeroLayer);
  CHECK(graphic.getActiveLayer() == zeroLayer);
  CHECK_FALSE(zeroLayer->isFrozen());
  CHECK(zeroLayer->isLocked());
  graphic.removeLayerListListener(&removalListener);
}

TEST_CASE("Layer changes rebuild flattened nested INSERT visibility",
          "[block-insert][layer-visibility][insert-nested]") {
  ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *referenceLayer = new RS_Layer(QStringLiteral("REFERENCE"));
  auto *detailLayer = new RS_Layer(QStringLiteral("DETAIL"));
  graphic.addLayer(referenceLayer);
  graphic.addLayer(detailLayer);

  auto *leaf = new RS_Block(&graphic,
                            RS_BlockData(QStringLiteral("VISIBILITY-LEAF"),
                                         RS_Vector(0.0, 0.0), false));
  auto *source = new RS_Line(leaf, RS_LineData(RS_Vector(0.0, 0.0),
                                                RS_Vector(1.0, 0.0)));
  source->setLayer(detailLayer);
  leaf->addEntity(source);
  graphic.addBlock(leaf);

  auto *parent = new RS_Block(&graphic,
                              RS_BlockData(QStringLiteral("VISIBILITY-PARENT"),
                                           RS_Vector(0.0, 0.0), false));
  auto *nested = new RS_Insert(parent,
                               insertData(QStringLiteral("VISIBILITY-LEAF"),
                                          RS_Vector(0.0, 0.0)));
  nested->setLayer(referenceLayer);
  parent->addEntity(nested);
  graphic.addBlock(parent);

  auto *insert = new RS_Insert(&graphic,
                               insertData(QStringLiteral("VISIBILITY-PARENT"),
                                          RS_Vector(0.0, 0.0)));
  graphic.addEntity(insert);
  RS_Line *line = onlyLine(*insert);
  REQUIRE(line->getFlag(RS2::FlagVisible));
  REQUIRE(graphic.getMax().valid);

  graphic.select(QList<RS_Entity*>{line});
  REQUIRE(graphic.hasSelection());
  graphic.toggleLayer(referenceLayer);
  CHECK_FALSE(onlyLine(*insert)->getFlag(RS2::FlagVisible));
  CHECK_FALSE(graphic.hasSelection());
  CHECK_FALSE(graphic.getMax().valid);

  graphic.toggleLayer(referenceLayer);
  line = onlyLine(*insert);
  CHECK(line->getFlag(RS2::FlagVisible));
  CHECK(graphic.getMax().valid);

  graphic.toggleLayer(detailLayer);
  CHECK_FALSE(onlyLine(*insert)->getFlag(RS2::FlagVisible));
  CHECK_FALSE(graphic.getMax().valid);

  graphic.toggleLayer(detailLayer);
  CHECK(onlyLine(*insert)->getFlag(RS2::FlagVisible));
  CHECK(graphic.getMax().valid);
}

TEST_CASE("WIPEOUT native frame maps and remains synchronized after transforms",
          "[wipeout][wipeout-native-frame]") {
  LC_WipeoutData data;
  data.hasNativeFrame = true;
  data.insertionPoint = RS_Vector(10.0, 20.0);
  data.uPixel = RS_Vector(0.5, 0.0);
  data.vPixel = RS_Vector(0.0, 2.0);
  data.sizeU = 4.0;
  data.sizeV = 2.0;
  data.clipPath = {RS_Vector(-0.5, -0.5), RS_Vector(3.5, -0.5),
                   RS_Vector(3.5, 1.5), RS_Vector(-0.5, 1.5)};
  LC_Wipeout wipeout(nullptr, data);

  REQUIRE(wipeout.getVertices().size() == 4);
  CHECK(wipeout.getVertices()[0].x == Catch::Approx(10.0));
  CHECK(wipeout.getVertices()[0].y == Catch::Approx(20.0));
  CHECK(wipeout.getVertices()[2].x == Catch::Approx(12.0));
  CHECK(wipeout.getVertices()[2].y == Catch::Approx(24.0));

  wipeout.move(RS_Vector(5.0, -10.0));
  const LC_WipeoutData &moved = wipeout.getData();
  CHECK(moved.insertionPoint.x == Catch::Approx(15.0));
  CHECK(moved.insertionPoint.y == Catch::Approx(10.0));
  CHECK(moved.clipPath[2].x == Catch::Approx(3.5));
  CHECK(moved.clipPath[2].y == Catch::Approx(1.5));
  CHECK(wipeout.getVertices()[2].x == Catch::Approx(17.0));
  CHECK(wipeout.getVertices()[2].y == Catch::Approx(14.0));

  wipeout.rotate(RS_Vector(0.0, 0.0), std::atan2(1.0, 0.0));
  CHECK(wipeout.getData().insertionPoint.x == Catch::Approx(-10.0));
  CHECK(wipeout.getData().insertionPoint.y == Catch::Approx(15.0));
  CHECK(wipeout.getVertices()[2].x == Catch::Approx(-14.0));
  CHECK(wipeout.getVertices()[2].y == Catch::Approx(17.0));

  wipeout.scale(RS_Vector(0.0, 0.0), RS_Vector(2.0, 3.0));
  CHECK(wipeout.getData().insertionPoint.x == Catch::Approx(-20.0));
  CHECK(wipeout.getData().insertionPoint.y == Catch::Approx(45.0));
  CHECK(wipeout.getVertices()[2].x == Catch::Approx(-28.0));
  CHECK(wipeout.getVertices()[2].y == Catch::Approx(51.0));
}

TEST_CASE("WIPEOUT rectangles retain two raw corners and derive four world vertices",
          "[wipeout][wipeout-rectangle]") {
  LC_WipeoutData data;
  data.hasNativeFrame = true;
  data.insertionPoint = RS_Vector(0.0, 0.0);
  data.uPixel = RS_Vector(1.0, 0.0);
  data.vPixel = RS_Vector(0.0, 1.0);
  data.clipBoundaryType = 1;
  data.clipPath = {RS_Vector(0.0, 0.0), RS_Vector(2.0, 3.0)};

  LC_Wipeout wipeout(nullptr, data);
  REQUIRE(wipeout.getData().clipPath.size() == 2);
  REQUIRE(wipeout.getVertices().size() == 4);
  CHECK(wipeout.getVertices()[0].x == Catch::Approx(0.5));
  CHECK(wipeout.getVertices()[0].y == Catch::Approx(0.5));
  CHECK(wipeout.getVertices()[2].x == Catch::Approx(2.5));
  CHECK(wipeout.getVertices()[2].y == Catch::Approx(3.5));
}
