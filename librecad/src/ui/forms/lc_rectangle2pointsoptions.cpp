#include "lc_rectangle2pointsoptions.h"
#include "ui_lc_rectangle2pointsoptions.h"
#include "rs_settings.h"
#include "rs_math.h"

LC_Rectangle2PointsOptions::LC_Rectangle2PointsOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_Rectangle2PointsOptions)
{
    ui->setupUi(this);

//    connect(ui->leWidth, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onWidthEditingFinished);
//    connect(ui->leHeight, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onHeightEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onAngleEditingFinished);
    connect(ui->cbCorners, SIGNAL(currentIndexChanged(int)), SLOT(onCornersIndexChanged(int)));
    connect(ui->cbSnapStart, SIGNAL(currentIndexChanged(int)), SLOT(onInsertionPointSnapIndexChanged(int)));
    connect(ui->cbSnapEnd, SIGNAL(currentIndexChanged(int)), SLOT(onSecondPointSnapIndexChanged(int)));

    connect(ui->cbPolyline, SIGNAL(clicked(bool)), this, SLOT(onUsePolylineClicked(bool)));
    connect(ui->cbSnapRadiusCenter, SIGNAL(clicked(bool)), this, SLOT(onSnapToCornerArcCenterClicked(bool)));
}

LC_Rectangle2PointsOptions::~LC_Rectangle2PointsOptions(){
    delete ui;
}

void LC_Rectangle2PointsOptions::languageChange(){
    ui->retranslateUi(this);
}


void LC_Rectangle2PointsOptions::clearAction(){
    action = nullptr;
}

bool LC_Rectangle2PointsOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawRectangle2Points;
}

void LC_Rectangle2PointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = static_cast<LC_ActionDrawRectangle2Points *>(a);


    QString angle;
    QString radius;
    QString lenX;
    QString lenY;

    int cornersMode;
    int insertSnapMode;
    int secondPointSnapMode;
    bool usePolyline;
    bool snapRadiusCenter;

    if (update){
        cornersMode = action->getCornersMode();
        insertSnapMode = action->getInsertionPointSnapMode();
        secondPointSnapMode = action->getSecondPointSnapMode();
        usePolyline = action->isUsePolyline();

        double an = action->getAngle();
        double r  = action->getRadius();
        double lX = action->getLengthX();
        double lY = action->getLengthY();


        angle = QString::number(an, 'g', 6);
        radius = QString::number(r, 'g', 6);
        lenX = QString::number(lX, 'g', 6);
        lenY = QString::number(lY, 'g', 6);
        snapRadiusCenter = action->isSnapToCornerArcCenter();
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        angle = RS_SETTINGS->readEntry("/Rectangle2PointsAngle", "0");
        insertSnapMode = RS_SETTINGS->readNumEntry("/Rectangle2PointsInsertSnapMode", 0);
        secondPointSnapMode = RS_SETTINGS->readNumEntry("/Rectangle2PointsSecondPointMode", 0);
        cornersMode = RS_SETTINGS->readNumEntry("/Rectangle2PointsCorners", 0);
        radius = RS_SETTINGS->readEntry("/Rectangle2PointsRadius", "0.0");
        lenX = RS_SETTINGS->readEntry("/Rectangle2PointsLengthX", "5");
        lenY = RS_SETTINGS->readEntry("/Rectangle2PointsLengthY", "5");
        usePolyline = RS_SETTINGS->readNumEntry("/Rectangle2PointsPolyline", 1) == 1;
        snapRadiusCenter = RS_SETTINGS->readNumEntry("/Rectangle2PointsRadiusSnap", 1) == 1;
        RS_SETTINGS->endGroup();
    }

    setAngleToActionAndView(angle);
    setRadiusToActionAnView(radius);
    setLenXToActionAnView(lenX);
    setLenYToActionAnView(lenY);
    setCornersModeToActionAndView(cornersMode);
    setInsertSnapPointModeToActionAndView(insertSnapMode);
    setSecondPointSnapPointModeToActionAndView(secondPointSnapMode);
    setUsePolylineToActionAndView(usePolyline);
    setSnapToCornerArcCenter(snapRadiusCenter);
}


void LC_Rectangle2PointsOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/Rectangle2PointsAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("/Rectangle2PointsInsertSnapMode", ui->cbSnapStart->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle2PointsSecondPointMode", ui->cbSnapEnd->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle2PointsCorners", ui->cbCorners->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle2PointsRadius", ui->leRadius->text());
    RS_SETTINGS->writeEntry("/Rectangle2PointsLengthX", ui->leX->text());
    RS_SETTINGS->writeEntry("/Rectangle2PointsLengthY", ui->leLenY->text());
    RS_SETTINGS->writeEntry("/Rectangle2PointsPolyline", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle2PointsRadiusSnap", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->endGroup();
}

void LC_Rectangle2PointsOptions::onCornersIndexChanged(int index){
    if (action != nullptr){
        setCornersModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::setCornersModeToActionAndView(int index){
    action->setCornersMode(index);
    bool round = index == LC_AbstractActionDrawRectangle::DRAW_RADIUS;
    bool bevel = index == LC_AbstractActionDrawRectangle::DRAW_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->leX->setVisible(bevel);


    ui->cbCorners->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::onLenYEditingFinished(){
    if (action != nullptr){
        QString value = ui->leLenY->text();
        setLenYToActionAnView(value);
    }
}

void LC_Rectangle2PointsOptions::onLenXEditingFinished(){
    if (action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
    }
}


void LC_Rectangle2PointsOptions::onRadiusEditingFinished(){
    if (action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
    }
}


void LC_Rectangle2PointsOptions::onInsertionPointSnapIndexChanged(int index){
    if (action != nullptr){
        setInsertSnapPointModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::onSecondPointSnapIndexChanged(int index){
    if (action != nullptr){
        setSecondPointSnapPointModeToActionAndView(index);
    }
}

// fixme - mapping between indexes and mode
void LC_Rectangle2PointsOptions::setInsertSnapPointModeToActionAndView(int index){
    action->setInsertionPointSnapMode(index);
    ui->cbSnapStart->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::setSecondPointSnapPointModeToActionAndView(int index){
    action->setSecondPointSnapMode(index);
    ui->cbSnapEnd->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_Rectangle2PointsOptions::setAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_Rectangle2PointsOptions::setLenYToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthY(y);
    ui->leLenY->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle2PointsOptions::setLenXToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthX(y);
    ui->leX->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle2PointsOptions::setRadiusToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setRadius(y);
    ui->leRadius->setText(QString::number(y, 'g', 6));
}


void LC_Rectangle2PointsOptions::onUsePolylineClicked(bool value){
    if (action != nullptr){
        setUsePolylineToActionAndView(value);
    }
}

void LC_Rectangle2PointsOptions::onSnapToCornerArcCenterClicked(bool value){
    if (action != nullptr){
        setSnapToCornerArcCenter(value);
    }
}

void LC_Rectangle2PointsOptions::setUsePolylineToActionAndView(bool value){
    action->setUsePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_Rectangle2PointsOptions::setSnapToCornerArcCenter(bool value){
    action->setSnapToCornerArcCenter(value);
    ui->cbSnapRadiusCenter->setChecked(value);
}

