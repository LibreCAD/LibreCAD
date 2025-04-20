/****************************************************************************
**
* Options widget for "Rectangle2Points" action.

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "lc_rectangle2pointsoptions.h"
#include "lc_actiondrawrectangle2points.h"
#include "ui_lc_rectangle2pointsoptions.h"

LC_Rectangle2PointsOptions::LC_Rectangle2PointsOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionDrawRectangle2Points, "Draw", "Rectangle2Points"),
    ui(new Ui::LC_Rectangle2PointsOptions),
    m_action(nullptr){
    ui->setupUi(this);

    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onLenXEditingFinished);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle2PointsOptions::onAngleEditingFinished);
    connect(ui->cbCorners, &QComboBox::currentIndexChanged, this, &LC_Rectangle2PointsOptions::onCornersIndexChanged);
    connect(ui->cbSnapStart, &QComboBox::currentIndexChanged,this,  &LC_Rectangle2PointsOptions::onInsertionPointSnapIndexChanged);
    connect(ui->cbSnapEnd, &QComboBox::currentIndexChanged,  this,&LC_Rectangle2PointsOptions::onSecondPointSnapIndexChanged);
    connect(ui->cbEdges, &QComboBox::currentIndexChanged,  this, &LC_Rectangle2PointsOptions::onEdgesIndexChanged);
    connect(ui->chkFixedBaseAngle, &QCheckBox::clicked, this,  &LC_Rectangle2PointsOptions::onBaseAngleFixedClicked);

    connect(ui->cbPolyline, &QCheckBox::clicked, this,  &LC_Rectangle2PointsOptions::onUsePolylineClicked);
    connect(ui->cbSnapRadiusCenter, &QCheckBox::clicked, this,  &LC_Rectangle2PointsOptions::onSnapToCornerArcCenterClicked);
}

LC_Rectangle2PointsOptions::~LC_Rectangle2PointsOptions(){
    delete ui;
}

void LC_Rectangle2PointsOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_Rectangle2PointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionDrawRectangle2Points *>(a);

    QString angle;
    QString radius;
    QString lenX;
    QString lenY;

    int cornersMode;
    int insertSnapMode;
    int secondPointSnapMode;
    bool usePolyline;
    bool snapRadiusCenter;
    int edges;
    bool fixedBaseAngle;

    if (update){
        cornersMode = m_action->getCornersMode();
        insertSnapMode = m_action->getInsertionPointSnapMode();
        secondPointSnapMode = m_action->getSecondPointSnapMode();
        usePolyline = m_action->isUsePolyline();
        edges = m_action->getEdgesDrawMode();

        double an = m_action->getUcsAngleDegrees();
        double r  = m_action->getRadius();
        double lX = m_action->getLengthX();
        double lY = m_action->getLengthY();


        angle = fromDouble(an);
        radius = fromDouble(r);
        lenX = fromDouble(lX);
        lenY = fromDouble(lY);
        snapRadiusCenter = m_action->isSnapToCornerArcCenter();
        fixedBaseAngle = m_action->hasBaseAngle();
    }
    else{
        angle = load("Angle", "0");
        insertSnapMode =loadInt("InsertSnapMode", 0);
        secondPointSnapMode = loadInt("SecondPointMode", 0);
        cornersMode = loadInt("Corners", 0);
        radius = load("Radius", "0.0");
        lenX = load("LengthX", "5");
        lenY = load("LengthY", "5");
        usePolyline = loadBool("Polyline", true);
        snapRadiusCenter = loadBool("RadiusSnap", true);
        edges = loadInt("Edges", 0);
        fixedBaseAngle = loadBool("BaseAngleIsFixed", false);
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
    setEdgesModeToActionAndView(edges);
    setBaseAngleFixedToActionAndView(fixedBaseAngle);
}

void LC_Rectangle2PointsOptions::doSaveSettings(){
    save("Angle", ui->leAngle->text());
    save("InsertSnapMode", ui->cbSnapStart->currentIndex());
    save("SecondPointMode", ui->cbSnapEnd->currentIndex());
    save("Corners", ui->cbCorners->currentIndex());
    save("Radius", ui->leRadius->text());
    save("LengthX", ui->leX->text());
    save("LengthY", ui->leLenY->text());
    save("Polyline", ui->cbPolyline->isChecked());
    save("RadiusSnap", ui->cbPolyline->isChecked());
    save("Edges", ui->cbEdges->currentIndex());
    save("BaseAngleIsFixed", ui->chkFixedBaseAngle->isChecked());
}

void LC_Rectangle2PointsOptions::onCornersIndexChanged(int index){
    if (m_action != nullptr){
        setCornersModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::setCornersModeToActionAndView(int index){
    m_action->setCornersMode(index);
    bool round = index == LC_AbstractActionDrawRectangle::CORNER_RADIUS;
    bool bevel = index == LC_AbstractActionDrawRectangle::CORNER_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->leX->setVisible(bevel);

    bool straight = index == LC_AbstractActionDrawRectangle::CORNER_STRAIGHT;
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbCorners->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::onLenYEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leLenY->text();
        setLenYToActionAnView(value);
    }
}

void LC_Rectangle2PointsOptions::onLenXEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
    }
}

void LC_Rectangle2PointsOptions::onRadiusEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
    }
}

void LC_Rectangle2PointsOptions::onInsertionPointSnapIndexChanged(int index){
    if (m_action != nullptr){
        setInsertSnapPointModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::onSecondPointSnapIndexChanged(int index){
    if (m_action != nullptr){
        setSecondPointSnapPointModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::onEdgesIndexChanged(int index){
    if (m_action != nullptr){
        setEdgesModeToActionAndView(index);
    }
}

void LC_Rectangle2PointsOptions::setEdgesModeToActionAndView(int index){
    m_action->setEdgesDrawMode(index);
    ui->cbEdges->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::setInsertSnapPointModeToActionAndView(int index){
    m_action->setInsertionPointSnapMode(index);
    ui->cbSnapStart->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::setSecondPointSnapPointModeToActionAndView(int index){
    m_action->setSecondPointSnapMode(index);
    ui->cbSnapEnd->setCurrentIndex(index);
}

void LC_Rectangle2PointsOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}


void LC_Rectangle2PointsOptions::onBaseAngleFixedClicked(bool value){
    if (m_action != nullptr){
        setBaseAngleFixedToActionAndView(value);
    }
}

void LC_Rectangle2PointsOptions::setBaseAngleFixedToActionAndView(bool value){
    ui->chkFixedBaseAngle->setChecked(value);
    m_action->setBaseAngleFixed(value);
    ui->leAngle->setEnabled(value);
}

void LC_Rectangle2PointsOptions::setAngleToActionAndView(const QString &val){
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        m_action->setUcsAngleDegrees(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_Rectangle2PointsOptions::setLenYToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setLengthY(y);
        ui->leLenY->setText(fromDouble(y));
    }
}

void LC_Rectangle2PointsOptions::setLenXToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setLengthX(y);
        ui->leX->setText(fromDouble(y));
    }
}

void LC_Rectangle2PointsOptions::setRadiusToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setRadius(y);
        ui->leRadius->setText(fromDouble(y));
    }
}

void LC_Rectangle2PointsOptions::onUsePolylineClicked(bool value){
    if (m_action != nullptr){
        setUsePolylineToActionAndView(value);
    }
}

void LC_Rectangle2PointsOptions::onSnapToCornerArcCenterClicked(bool value){
    if (m_action != nullptr){
        setSnapToCornerArcCenter(value);
    }
}

void LC_Rectangle2PointsOptions::setUsePolylineToActionAndView(bool value){
    m_action->setUsePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_Rectangle2PointsOptions::setSnapToCornerArcCenter(bool value){
    m_action->setSnapToCornerArcCenter(value);
    ui->cbSnapRadiusCenter->setChecked(value);
}
