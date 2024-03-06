#include "lc_rectangle1pointoptions.h"
#include "ui_lc_rectangle1pointoptions.h"
#include "rs_actioninterface.h"
#include "lc_actiondrawrectangle1point.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"


LC_Rectangle1PointOptions::LC_Rectangle1PointOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_Rectangle1PointOptions)
{
    ui->setupUi(this);

    connect(ui->leWidth, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onWidthEditingFinished);
    connect(ui->leHeight, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onHeightEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle1PointOptions::onAngleEditingFinished);
    connect(ui->cbCorners, SIGNAL(currentIndexChanged(int)), SLOT(onCornersIndexChanged(int)));
    connect(ui->cbSnapPoint, SIGNAL(currentIndexChanged(int)), SLOT(onSnapPointIndexChanged(int)));

    connect(ui->cbPolyline, SIGNAL(clicked(bool)), this, SLOT(onUsePolylineClicked(bool)));
    connect(ui->cbSnapRadiusCenter, SIGNAL(clicked(bool)), this, SLOT(onSnapToCornerArcCenterClicked(bool)));
    connect(ui->cbInnerSize, SIGNAL(clicked(bool)), this, SLOT(onInnerSizeClicked(bool)));
}

LC_Rectangle1PointOptions::~LC_Rectangle1PointOptions(){
    delete ui;
}

void LC_Rectangle1PointOptions::clearAction(){
    action = nullptr;
}
bool LC_Rectangle1PointOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType ==RS2::ActionDrawRectangle1Point;
}

void LC_Rectangle1PointOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/Rectangle1PointWidth", ui->leWidth->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointHeight", ui->leHeight->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointSnapMode", ui->cbSnapPoint->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle1PointCorners", ui->cbCorners->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle1PointRadius", ui->leRadius->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointLengthX", ui->leX->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointLengthY", ui->leLenY->text());
    RS_SETTINGS->writeEntry("/Rectangle1PointPolyline", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle1PointRadiusSnap", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle1PointSizeInner", ui->cbInnerSize->isChecked()  ? 1 : 0);
    RS_SETTINGS->endGroup();
}

void LC_Rectangle1PointOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_Rectangle1PointOptions::doSetAction(RS_ActionInterface * a, bool update){
        action = static_cast<LC_ActionDrawRectangle1Point *>(a);

        QString width;
        QString height;
        QString angle;
        QString radius;
        QString lenX;
        QString lenY;

        int cornersMode;
        int snapMode;
        bool usePolyline;
        bool snapRadiusCenter;
        bool sizeIsInner;

        if (update){
            cornersMode = action->getCornersMode();
            snapMode = action->getInsertionPointSnapMode();
            usePolyline = action->isUsePolyline();

            double w = action->getWidth();
            double h = action->getHeight();
            double an = action->getAngle();
            double r  = action->getRadius();
            double lX = action->getLengthX();
            double lY = action->getLengthY();

            width = QString::number(w, 'g', 6);
            height = QString::number(h, 'g', 6);
            angle = QString::number(an, 'g', 6);
            radius = QString::number(r, 'g', 6);
            lenX = QString::number(lX, 'g', 6);
            lenY = QString::number(lY, 'g', 6);
            snapRadiusCenter = action->isSnapToCornerArcCenter();
            sizeIsInner = action->isSizeInner();
        }
        else{
            RS_SETTINGS->beginGroup("/Draw");
            width = RS_SETTINGS->readEntry("/Rectangle1PointWidth", "10");
            height = RS_SETTINGS->readEntry("/Rectangle1PointHeight", "10");
            angle = RS_SETTINGS->readEntry("/Rectangle1PointAngle", "0");
            snapMode = RS_SETTINGS->readNumEntry("/Rectangle1PointSnapMode", 0);
            cornersMode = RS_SETTINGS->readNumEntry("/Rectangle1PointCorners", 0);
            radius = RS_SETTINGS->readEntry("/Rectangle1PointRadius", "0.0");
            lenX = RS_SETTINGS->readEntry("/Rectangle1PointLengthX", "5");
            lenY = RS_SETTINGS->readEntry("/Rectangle1PointLengthY", "5");
            usePolyline = RS_SETTINGS->readNumEntry("/Rectangle1PointPolyline", 1) == 1;
            snapRadiusCenter = RS_SETTINGS->readNumEntry("/Rectangle1PointRadiusSnap", 1) == 1;
            sizeIsInner = RS_SETTINGS->readNumEntry("/Rectangle1PointSizeInner", 1) == 1;

            RS_SETTINGS->endGroup();
        }

        setWidthToActionAnView(width);
        setHeightToActionAnView(height);
        setAngleToActionAndView(angle);
        setRadiusToActionAnView(radius);
        setLenXToActionAnView(lenX);
        setLenYToActionAnView(lenY);
        setCornersModeToActionAndView(cornersMode);
        setSnapPointModeToActionAndView(snapMode);
        setUsePolylineToActionAndView(usePolyline);
        setSnapToCornerArcCenterToActionAndView(snapRadiusCenter);
        setSizeInnerToActionAndView(sizeIsInner);
}

void LC_Rectangle1PointOptions::onCornersIndexChanged(int index){
  if (action != nullptr){
      setCornersModeToActionAndView(index);
  }
}

void LC_Rectangle1PointOptions::setCornersModeToActionAndView(int index){
    action->setCornersMode(index);
    bool round = index == LC_AbstractActionDrawRectangle::DRAW_RADIUS;
    bool bevel = index == LC_AbstractActionDrawRectangle::DRAW_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);
    ui->cbInnerSize->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->leX->setVisible(bevel);

    ui->cbCorners->setCurrentIndex(index);
}

void LC_Rectangle1PointOptions::onLenYEditingFinished(){
     if (action != nullptr){
         QString value = ui->leLenY->text();
         setLenYToActionAnView(value);
     }
}

void LC_Rectangle1PointOptions::onLenXEditingFinished(){
    if (action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
    }
}


void LC_Rectangle1PointOptions::onRadiusEditingFinished(){
    if (action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
    }
}

void LC_Rectangle1PointOptions::onHeightEditingFinished(){
    if (action != nullptr){
        QString value = ui->leHeight->text();
        setHeightToActionAnView(value);
    }
}

void LC_Rectangle1PointOptions::onWidthEditingFinished(){
    if (action != nullptr){
        QString value = ui->leWidth->text();
        setWidthToActionAnView(value);
    }
}

void LC_Rectangle1PointOptions::onSnapPointIndexChanged(int index){
  if (action != nullptr){
      setSnapPointModeToActionAndView(index);
  }
}

void LC_Rectangle1PointOptions::setSnapPointModeToActionAndView(int index){
    action->setInsertionPointSnapMode(index);
    ui->cbSnapPoint->setCurrentIndex(index);
}

void LC_Rectangle1PointOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_Rectangle1PointOptions::setAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_Rectangle1PointOptions::setLenYToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthY(y);
    ui->leLenY->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle1PointOptions::setLenXToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthX(y);
    ui->leX->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle1PointOptions::setRadiusToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setRadius(y);
    ui->leRadius->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle1PointOptions::setHeightToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=0;
    action->setHeight(y);
    ui->leHeight->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle1PointOptions::setWidthToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=0;
    action->setWidth(y);
    ui->leWidth->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle1PointOptions::onUsePolylineClicked(bool value){
   if (action != nullptr){
       setUsePolylineToActionAndView(value);
   }
}

void LC_Rectangle1PointOptions::onSnapToCornerArcCenterClicked(bool value){
    if (action != nullptr){
        setSnapToCornerArcCenterToActionAndView(value);
    }
}

void LC_Rectangle1PointOptions::onInnerSizeClicked(bool value){
    if (action != nullptr){
        setSizeInnerToActionAndView(value);
    }
}


void LC_Rectangle1PointOptions::setUsePolylineToActionAndView(bool value){
   action->setUsePolyline(value);
   ui->cbPolyline->setChecked(value);
}

void LC_Rectangle1PointOptions::setSnapToCornerArcCenterToActionAndView(bool value){
   action->setSnapToCornerArcCenter(value);
   ui->cbSnapRadiusCenter->setChecked(value);
}

void LC_Rectangle1PointOptions::setSizeInnerToActionAndView(bool value){
   action->setSizeInner(value);
   ui->cbInnerSize->setChecked(value);
}


