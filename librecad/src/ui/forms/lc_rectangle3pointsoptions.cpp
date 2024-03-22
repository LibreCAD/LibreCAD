#include "lc_rectangle3pointsoptions.h"
#include "ui_lc_rectangle3pointsoptions.h"
#include "rs_settings.h"
#include "rs_math.h"

LC_Rectangle3PointsOptions::LC_Rectangle3PointsOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_Rectangle3PointsOptions)
{
    ui->setupUi(this);


    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onAngleEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onLenXEditingFinished);
    connect(ui->leInnerAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onInnerAngleEditingFinished);
    connect(ui->cbCorners, SIGNAL(currentIndexChanged(int)), SLOT(onCornersIndexChanged(int)));
    connect(ui->cbPolyline, SIGNAL(clicked(bool)), this, SLOT(onUsePolylineClicked(bool)));
    connect(ui->cbQuadrangle, SIGNAL(clicked(bool)), this, SLOT(onQuadrangleClicked(bool)));
    connect(ui->cbFixedInnerAngle, SIGNAL(clicked(bool)), this, SLOT(onInnerAngleFixedClicked(bool)));
    connect(ui->cbSnapRadiusCenter, SIGNAL(clicked(bool)), this, SLOT(onSnapToCornerArcCenterClicked(bool)));
    connect(ui->cbEdges, SIGNAL(currentIndexChanged(int)), SLOT(onEdgesIndexChanged(int)));
}

LC_Rectangle3PointsOptions::~LC_Rectangle3PointsOptions(){
    saveSettings();
    delete ui;
}

void LC_Rectangle3PointsOptions::languageChange(){
    ui->retranslateUi(this);
}


void LC_Rectangle3PointsOptions::clearAction(){
    action = nullptr;
}

bool LC_Rectangle3PointsOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawRectangle3Points;
}

void LC_Rectangle3PointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = static_cast<LC_ActionDrawRectangle3Points *>(a);


    QString angle;
    QString radius;
    QString lenX;
    QString lenY;

    int cornersMode;
    bool usePolyline;
    bool snapRadiusCenter;
    bool quadrangle;
    bool fixedInnerAngle;
    QString innerAngle;
    int edges;

    if (update){
        cornersMode = action->getCornersMode();

        usePolyline = action->isUsePolyline();

        double an = action->getAngle();
        double r  = action->getRadius();
        double lX = action->getLengthX();
        double lY = action->getLengthY();

        edges = action->getEdgesDrawMode();


        angle = QString::number(an, 'g', 6);
        radius = QString::number(r, 'g', 6);
        lenX = QString::number(lX, 'g', 6);
        lenY = QString::number(lY, 'g', 6);
        snapRadiusCenter = action->isSnapToCornerArcCenter();
        innerAngle = QString::number(action->getFixedInnerAngle(), 'g', 6);
        quadrangle = action->isCreateQuadrangle();
        fixedInnerAngle = action->isInnerAngleFixed();
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        angle = RS_SETTINGS->readEntry("/Rectangle3PointsAngle", "0");
        cornersMode = RS_SETTINGS->readNumEntry("/Rectangle3PointsCorners", 0);
        radius = RS_SETTINGS->readEntry("/Rectangle3PointsRadius", "0.0");
        lenX = RS_SETTINGS->readEntry("/Rectangle3PointsLengthX", "5");
        lenY = RS_SETTINGS->readEntry("/Rectangle3PointsLengthY", "5");
        usePolyline = RS_SETTINGS->readNumEntry("/Rectangle3PointsPolyline", 1) == 1;
        snapRadiusCenter = RS_SETTINGS->readNumEntry("/Rectangle3PointsRadiusSnap", 1) == 1;

        quadrangle = RS_SETTINGS->readNumEntry("/Rectangle3PointsQuadrangle", 0)==1;
        fixedInnerAngle = RS_SETTINGS->readNumEntry("/Rectangle3PointsQuadrangleAngleIsFixed", 0) == 1;
        innerAngle = RS_SETTINGS->readEntry("/Rectangle3PointsQuadrangleFixedAngle", "90");
        edges = RS_SETTINGS->readNumEntry("/Rectangle3PointsEdges", 0);
        RS_SETTINGS->endGroup();
    }

    setAngleToActionAndView(angle);
    setRadiusToActionAnView(radius);
    setLenXToActionAnView(lenX);
    setLenYToActionAnView(lenY);
    setCornersModeToActionAndView(cornersMode);
    setUsePolylineToActionAndView(usePolyline);
    setSnapToCornerArcCenter(snapRadiusCenter);
    setQuadrangleToActionAndView(quadrangle);
    setInnerAngleFixedToActionAndView(fixedInnerAngle);
    setInnerAngleToActionAndView(innerAngle);
    setEdgesModeToActionAndView(edges);

}


void LC_Rectangle3PointsOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/Rectangle3PointsAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("/Rectangle3PointsCorners", ui->cbCorners->currentIndex());
    RS_SETTINGS->writeEntry("/Rectangle3PointsRadius", ui->leRadius->text());
    RS_SETTINGS->writeEntry("/Rectangle3PointsLengthX", ui->leX->text());
    RS_SETTINGS->writeEntry("/Rectangle3PointsLengthY", ui->leLenY->text());
    RS_SETTINGS->writeEntry("/Rectangle3PointsPolyline", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle3PointsRadiusSnap", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle3PointsQuadrangle", ui->cbQuadrangle->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle3PointsQuadrangleAngleIsFixed", ui->cbFixedInnerAngle->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/Rectangle3PointsQuadrangleFixedAngle", ui->leInnerAngle->text());
    RS_SETTINGS->writeEntry("/Rectangle3PointsEdges", ui->cbEdges->currentIndex());
    RS_SETTINGS->endGroup();
}

void LC_Rectangle3PointsOptions::onCornersIndexChanged(int index){
    if (action != nullptr){
        setCornersModeToActionAndView(index);
    }
}

void LC_Rectangle3PointsOptions::setCornersModeToActionAndView(int index){
    action->setCornersMode(index);
    bool round = index == LC_AbstractActionDrawRectangle::CORNER_RADIUS;
    bool bevel = index == LC_AbstractActionDrawRectangle::CORNER_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->leX->setVisible(bevel);

    bool straight = index == LC_AbstractActionDrawRectangle::CORNER_STRAIGHT || ui->cbQuadrangle->isChecked();
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbCorners->setCurrentIndex(index);
}

void LC_Rectangle3PointsOptions::onLenYEditingFinished(){
    if (action != nullptr){
        QString value = ui->leLenY->text();
        setLenYToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onLenXEditingFinished(){
    if (action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onInnerAngleEditingFinished(){
    if (action != nullptr){
        QString value = ui->leInnerAngle->text();
        setInnerAngleToActionAndView(value);
    }
}


void LC_Rectangle3PointsOptions::onRadiusEditingFinished(){
    if (action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_Rectangle3PointsOptions::setAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_Rectangle3PointsOptions::setLenYToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthY(y);
    ui->leLenY->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle3PointsOptions::setLenXToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setLengthX(y);
    ui->leX->setText(QString::number(y, 'g', 6));
}

void LC_Rectangle3PointsOptions::setRadiusToActionAnView(QString value){
    bool ok = false;
    double y =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setRadius(y);
    ui->leRadius->setText(QString::number(y, 'g', 6));
}


void LC_Rectangle3PointsOptions::onUsePolylineClicked(bool value){
    if (action != nullptr){
        setUsePolylineToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onSnapToCornerArcCenterClicked(bool value){
    if (action != nullptr){
        setSnapToCornerArcCenter(value);
    }
}
void LC_Rectangle3PointsOptions::onQuadrangleClicked(bool value){
    if (action != nullptr){
        setQuadrangleToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onInnerAngleFixedClicked(bool value){
    if (action != nullptr){
        setInnerAngleFixedToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::setUsePolylineToActionAndView(bool value){
    action->setUsePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_Rectangle3PointsOptions::setSnapToCornerArcCenter(bool value){
    action->setSnapToCornerArcCenter(value);
    ui->cbSnapRadiusCenter->setChecked(value);
}

void LC_Rectangle3PointsOptions::setQuadrangleToActionAndView(bool value){
    action->setCreateQuadrangle(value);
    ui->cbQuadrangle->setChecked(value);
    ui->frmQuad->setVisible(value);
    ui->frmRectSettings->setVisible(!value);
    setCornersModeToActionAndView(ui->cbCorners->currentIndex());
}

void LC_Rectangle3PointsOptions::setInnerAngleFixedToActionAndView(bool value){
    ui->cbFixedInnerAngle->setChecked(value);
    action->setInnerAngleFixed(value);
    ui->leInnerAngle->setEnabled(value);
}

void LC_Rectangle3PointsOptions::setInnerAngleToActionAndView(QString value){
    bool ok = false;
    double val =std::abs(RS_Math::eval(value, &ok));
    if(!ok) return;
    if (std::abs(val)<RS_TOLERANCE) val=1.0;
    action->setFixedInnerAngle(val);
    ui->leInnerAngle->setText(QString::number(val, 'g', 6));
}

void LC_Rectangle3PointsOptions::onEdgesIndexChanged(int index){
    if (action != nullptr){
        setEdgesModeToActionAndView(index);
    }
}

void LC_Rectangle3PointsOptions::setEdgesModeToActionAndView(int index){
    action->setEdgesDrawMode(index);
    ui->cbEdges->setCurrentIndex(index);
}
