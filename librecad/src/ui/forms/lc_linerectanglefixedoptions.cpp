#include "lc_linerectanglefixedoptions.h"
#include "ui_lc_linerectanglefixedoptions.h"
#include "rs_actioninterface.h"
#include "lc_actiondrawlinerectanglefixed.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"

LC_LineRectangleFixedOptions::LC_LineRectangleFixedOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LC_LineRectangleFixedOptions)
{
    ui->setupUi(this);

    connect(ui->leWidth, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onWidthEditingFinished);
    connect(ui->leHeight, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onHeightEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineRectangleFixedOptions::onAngleEditingFinished);
    connect(ui->cbCorners, SIGNAL(currentIndexChanged(int)), SLOT(onCornersIndexChanged(int)));
    connect(ui->cbSnapPoint, SIGNAL(currentIndexChanged(int)), SLOT(onSnapPointIndexChanged(int)));

    connect(ui->cbPolyline, SIGNAL(clicked(bool)), this, SLOT(onUsePolylineClicked(bool)));
}

LC_LineRectangleFixedOptions::~LC_LineRectangleFixedOptions()
{
    LC_ERR<<"In Destructor: ";
    saveSettings();
    delete ui;
}

void LC_LineRectangleFixedOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/RectangleFixWidth", ui->leWidth->text());
    RS_SETTINGS->writeEntry("/RectangleFixHeight", ui->leHeight->text());
    RS_SETTINGS->writeEntry("/RectangleFixAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("/RectangleFixSnapMode", ui->cbSnapPoint->currentIndex());
    RS_SETTINGS->writeEntry("/RectangleFixCorners", ui->cbCorners->currentIndex());
    RS_SETTINGS->writeEntry("/RectangleFixRadius", ui->leRadius->text());
    RS_SETTINGS->writeEntry("/RectangleFixLengthX", ui->leX->text());
    RS_SETTINGS->writeEntry("/RectangleFixLengthY", ui->leLenY->text());
    RS_SETTINGS->writeEntry("/RectangleFixPolyline", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->endGroup();
}

void LC_LineRectangleFixedOptions::languageChange()
{
    ui->retranslateUi(this);
}

void LC_LineRectangleFixedOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawLineRectangleFixed){
        action = static_cast<LC_ActionDrawLineRectangleFixed *>(a);

        QString width;
        QString height;
        QString angle;
        QString radius;
        QString lenX;
        QString lenY;

        int cornersMode;
        int snapMode;
        bool usePolyline;

        if (update){
            cornersMode = action->getCornersMode();
            snapMode = action->getSnapPointMode();
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
        }
        else{
            LC_ERR<<"In Set Action READ: ";
            width = RS_SETTINGS->readEntry("/RectangleFixWidth", "10");
            height = RS_SETTINGS->readEntry("/RectangleFixHeight", "10");
            angle = RS_SETTINGS->readEntry("/RectangleFixAngle", "0");
            snapMode = RS_SETTINGS->readNumEntry("/RectangleFixSnapMode", 0);
            cornersMode = RS_SETTINGS->readNumEntry("/RectangleFixCorners", 0);
            radius = RS_SETTINGS->readEntry("/RectangleFixRadius", "0.0");
            lenX = RS_SETTINGS->readEntry("/RectangleFixLengthX", "5");
            lenY = RS_SETTINGS->readEntry("/RectangleFixLengthY", "5");
            usePolyline = RS_SETTINGS->readNumEntry("/RectangleFixPolyline", 1) == 1;
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
    }
}

void LC_LineRectangleFixedOptions::onCornersIndexChanged(int index){
  if (action != nullptr){
      setCornersModeToActionAndView(index);
  }
}

void LC_LineRectangleFixedOptions::setCornersModeToActionAndView(int index){
    action->setCornersMode(index);
    bool round = index == LC_ActionDrawLineRectangleFixed::DRAW_RADIUS;
    bool bevel = index == LC_ActionDrawLineRectangleFixed::DRAW_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->leX->setVisible(bevel);

    ui->cbCorners->setCurrentIndex(index);
}

void LC_LineRectangleFixedOptions::onLenYEditingFinished(){
     if (action != nullptr){
         QString value = ui->leLenY->text();
         setLenYToActionAnView(value);
         saveSettings();
     }
}

void LC_LineRectangleFixedOptions::onLenXEditingFinished(){
    if (action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
        saveSettings();
    }
}


void LC_LineRectangleFixedOptions::onRadiusEditingFinished(){
    if (action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
        saveSettings();
    }
}

void LC_LineRectangleFixedOptions::onHeightEditingFinished(){
    if (action != nullptr){
        QString value = ui->leHeight->text();
        setHeightToActionAnView(value);
        saveSettings();
    }
}

void LC_LineRectangleFixedOptions::onWidthEditingFinished(){
    if (action != nullptr){
        QString value = ui->leWidth->text();
        setWidthToActionAnView(value);
        saveSettings();
    }
}

void LC_LineRectangleFixedOptions::onSnapPointIndexChanged(int index){
  if (action != nullptr){
      setSnapPointModeToActionAndView(index);
      saveSettings();
  }
}

void LC_LineRectangleFixedOptions::setSnapPointModeToActionAndView(int index){
    action->setSnapPointMode(index);
    ui->cbSnapPoint->setCurrentIndex(index);
    saveSettings();
}

void LC_LineRectangleFixedOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
    saveSettings();
}

void LC_LineRectangleFixedOptions::setAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_LineRectangleFixedOptions::setLenYToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthY(y);
    ui->leLenY->setText(QString::number(y, 'g', 6));
}

void LC_LineRectangleFixedOptions::setLenXToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthX(y);
    ui->leX->setText(QString::number(y, 'g', 6));
}

void LC_LineRectangleFixedOptions::setRadiusToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setRadius(y);
    ui->leRadius->setText(QString::number(y, 'g', 6));
}

void LC_LineRectangleFixedOptions::setHeightToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setHeight(y);
    ui->leHeight->setText(QString::number(y, 'g', 6));
}

void LC_LineRectangleFixedOptions::setWidthToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setWidth(y);
    ui->leWidth->setText(QString::number(y, 'g', 6));
}

void LC_LineRectangleFixedOptions::onUsePolylineClicked(bool value){
   if (action != nullptr){
       setUsePolylineToActionAndView(value);
       saveSettings();
   }
}

void LC_LineRectangleFixedOptions::setUsePolylineToActionAndView(bool value){
   action->setUsePolyline(value);
   ui->cbPolyline->setChecked(value);
}
