// lc_hyperbolapropertieseditingwidget.cpp - updated without goto

#include "lc_hyperbolapropertieseditingwidget.h"
#include "rs_dialogfactoryinterface.h"
#include "ui_lc_hyperbolapropertieseditingwidget.h"

#include "lc_hyperbola.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"

static bool evalDouble(const QLineEdit* le, double& value)
{
  bool ok = false;
  value = RS_Math::eval(le->text(), &ok);
  return ok;
}

LC_HyperbolaPropertiesEditingWidget::LC_HyperbolaPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent), ui(new Ui::LC_HyperbolaPropertiesEditingWidget)
{
  ui->setupUi(this);
}

LC_HyperbolaPropertiesEditingWidget::~LC_HyperbolaPropertiesEditingWidget()
{
  delete ui;
}

void LC_HyperbolaPropertiesEditingWidget::setEntity(RS_Entity* entity)
{
  m_entity = static_cast<LC_Hyperbola*>(entity);
  if (m_entity && m_entity->isValid()) updateUI();
}

void LC_HyperbolaPropertiesEditingWidget::updateUI()
{
  if (!m_entity) return;
  const LC_HyperbolaData& d = m_entity->getData();

  auto setValue = [](QLineEdit* le, double v) { le->setText(QString::number(v, 'f', 10)); };

  setValue(ui->leCenterX, d.center.x);
  setValue(ui->leCenterY, d.center.y);
  setValue(ui->leMajorRadius, m_entity->getMajorRadius());
  setValue(ui->leMinorRadius, m_entity->getMinorRadius());
  setValue(ui->leRatio, m_entity->getRatio());
  setValue(ui->leAngle, RS_Math::rad2deg(d.majorP.angle()));

  ui->cbReversed->setChecked(d.reversed);

  setValue(ui->leAngle1, d.angle1);
  setValue(ui->leAngle2, d.angle2);

  RS_Vector f1 = m_entity->getFocus1();
  RS_Vector f2 = m_entity->getFocus2();
  setValue(ui->leFocus1X, f1.x); setValue(ui->leFocus1Y, f1.y);
  setValue(ui->leFocus2X, f2.x); setValue(ui->leFocus2Y, f2.y);

  RS_Vector v = m_entity->getPrimaryVertex();
  setValue(ui->leVertexX, v.x); setValue(ui->leVertexY, v.y);

  setValue(ui->leEccentricity, m_entity->getEccentricity());
}

void LC_HyperbolaPropertiesEditingWidget::updateEntityData()
{
  if (!m_entity || !m_entity->isValid()) return;

  LC_HyperbolaData& d = const_cast<LC_HyperbolaData&>(m_entity->getData());

  bool applied = false;

         // Try foci + point mode first
  double f1x, f1y, f2x, f2y, px, py;
  if (evalDouble(ui->leFocus1X, f1x) && evalDouble(ui->leFocus1Y, f1y) &&
      evalDouble(ui->leFocus2X, f2x) && evalDouble(ui->leFocus2Y, f2y) &&
      evalDouble(ui->leStartX, px)   && evalDouble(ui->leStartY, py)) {

    RS_Vector newF1(f1x, f1y), newF2(f2x, f2y), point(px, py);

    if ((newF1 - newF2).squared() >= RS_TOLERANCE2) {
      LC_HyperbolaData test(newF1, newF2, point);
      if (test.isValid()) {
        m_entity->setFocus1(newF1);
        m_entity->setFocus2(newF2);
        m_entity->setPointOnCurve(point);
        d.reversed = ui->cbReversed->isChecked();
        applied = true;
      } else {
        RS_DIALOGFACTORY->commandMessage(tr("Invalid hyperbola definition"));
      }
    } else {
      RS_DIALOGFACTORY->commandMessage(tr("Foci must be distinct"));
    }
  }

         // If foci mode didn't apply, use standard parameters
  if (!applied) {
    double cx, cy, a, ratio = -1.0, b = -1.0, angleDeg;

    if (!evalDouble(ui->leCenterX, cx) || !evalDouble(ui->leCenterY, cy)) {
      RS_DIALOGFACTORY->commandMessage(tr("Invalid center coordinates"));
      return;
    }
    if (!evalDouble(ui->leMajorRadius, a) || a <= 0.0) {
      RS_DIALOGFACTORY->commandMessage(tr("Major radius must be positive"));
      return;
    }
    if (!evalDouble(ui->leAngle, angleDeg)) {
      RS_DIALOGFACTORY->commandMessage(tr("Invalid rotation angle"));
      return;
    }

    if (evalDouble(ui->leRatio, ratio) && ratio > 0.0) {
      m_entity->setRatio(ratio);
    } else if (evalDouble(ui->leMinorRadius, b) && b > 0.0) {
      m_entity->setMinorRadius(b);
    } else {
      RS_DIALOGFACTORY->commandMessage(tr("Ratio or minor radius must be positive"));
      return;
    }

    double angleRad = RS_Math::deg2rad(angleDeg);
    d.center = RS_Vector(cx, cy);
    d.majorP = RS_Vector(a * std::cos(angleRad), a * std::sin(angleRad));
    d.reversed = ui->cbReversed->isChecked();

    double a1, a2;
    if (evalDouble(ui->leAngle1, a1)) d.angle1 = a1;
    if (evalDouble(ui->leAngle2, a2)) d.angle2 = a2;

    applied = true;
  }

  if (applied) {
    m_entity->calculateBorders();
    m_entity->updateLength();
    m_entity->update();
    updateUI();
  }
}
