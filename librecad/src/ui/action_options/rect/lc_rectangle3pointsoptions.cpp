/****************************************************************************
**
* Options widget for "Rectangle3Point" action.

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
#include "lc_rectangle3pointsoptions.h"
#include "lc_actiondrawrectangle3points.h"
#include "ui_lc_rectangle3pointsoptions.h"

LC_Rectangle3PointsOptions::LC_Rectangle3PointsOptions() :
    LC_ActionOptionsWidgetBase(RS2::ActionDrawRectangle3Points, "Draw", "Rectangle3Points"),
    m_action(nullptr),
    ui(new Ui::LC_Rectangle3PointsOptions){
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onAngleEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onRadiusEditingFinished);
    connect(ui->leLenY, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onLenYEditingFinished);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onLenXEditingFinished);
    connect(ui->leInnerAngle, &QLineEdit::editingFinished, this, &LC_Rectangle3PointsOptions::onInnerAngleEditingFinished);
    connect(ui->cbCorners, &QComboBox::currentIndexChanged, this, &LC_Rectangle3PointsOptions::onCornersIndexChanged);
    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptions::onUsePolylineClicked);
    connect(ui->cbQuadrangle,&QCheckBox::clicked, this, &LC_Rectangle3PointsOptions::onQuadrangleClicked);
    connect(ui->cbFixedInnerAngle, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptions::onInnerAngleFixedClicked);
    connect(ui->chkFixedBaseAngle, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptions::onBaseAngleFixedClicked);
    connect(ui->cbSnapRadiusCenter, &QCheckBox::clicked, this, &LC_Rectangle3PointsOptions::onSnapToCornerArcCenterClicked);
    connect(ui->cbEdges,&QComboBox::currentIndexChanged, this, &LC_Rectangle3PointsOptions::onEdgesIndexChanged);

    pickAngleSetup("angleBase", ui->tbPickAngleBase, ui->leAngle);
    pickAngleSetup("angleInner", ui->tbPickAngleInner, ui->leInnerAngle);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
    pickDistanceSetup("lengthY", ui->tbPickLengthY, ui->leLenY);
    pickDistanceSetup("lengthX", ui->tbPickLengthX, ui->leX);
}

LC_Rectangle3PointsOptions::~LC_Rectangle3PointsOptions(){
    m_action = nullptr;
    delete ui;
}

void LC_Rectangle3PointsOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_Rectangle3PointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<LC_ActionDrawRectangle3Points *>(a);

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
    bool fixedBaseAngle;

    if (update){
        cornersMode = m_action->getCornersMode();
        usePolyline = m_action->isUsePolyline();

        double an = m_action->getUcsAngleDegrees();
        double r  = m_action->getRadius();
        double lX = m_action->getLengthX();
        double lY = m_action->getLengthY();

        edges = m_action->getEdgesDrawMode();
        angle = fromDouble(an);
        radius = fromDouble(r);
        lenX = fromDouble(lX);
        lenY = fromDouble(lY);
        snapRadiusCenter = m_action->isSnapToCornerArcCenter();
        innerAngle = fromDouble(m_action->getFixedInnerAngle());
        quadrangle = m_action->isCreateQuadrangle();
        fixedInnerAngle = m_action->isInnerAngleFixed();
        fixedBaseAngle = m_action->hasBaseAngle();
    }
    else{
  
        angle =load("Angle", "0");
        cornersMode = loadInt("Corners", 0);
        radius =load("Radius", "0.0");
        lenX =load("LengthX", "5");
        lenY =load("LengthY", "5");
        usePolyline = loadBool("Polyline", true);
        snapRadiusCenter = loadBool("RadiusSnap", true);

        quadrangle = loadBool("Quadrangle", false);
        fixedInnerAngle = loadBool("QuadrangleAngleIsFixed", false);
        fixedBaseAngle = loadBool("BaseAngleIsFixed", false);
        innerAngle =load("QuadrangleFixedAngle", "90");
        edges = loadInt("Edges", 0);        
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
    setBaseAngleFixedToActionAndView(fixedBaseAngle);
}

void LC_Rectangle3PointsOptions::doSaveSettings(){    
    save("Angle", ui->leAngle->text());
    save("Corners", ui->cbCorners->currentIndex());
    save("Radius", ui->leRadius->text());
    save("LengthX", ui->leX->text());
    save("LengthY", ui->leLenY->text());
    save("Polyline", ui->cbPolyline->isChecked() );
    save("RadiusSnap", ui->cbPolyline->isChecked() );
    save("Quadrangle", ui->cbQuadrangle->isChecked() );
    save("QuadrangleAngleIsFixed", ui->cbFixedInnerAngle->isChecked() );
    save("BaseAngleIsFixed", ui->chkFixedBaseAngle->isChecked() );
    save("QuadrangleFixedAngle", ui->leInnerAngle->text());
    save("Edges", ui->cbEdges->currentIndex());
}

void LC_Rectangle3PointsOptions::onCornersIndexChanged(int index){
    if (m_action != nullptr){
        setCornersModeToActionAndView(index);
    }
}

void LC_Rectangle3PointsOptions::setCornersModeToActionAndView(int index){
    m_action->setCornersMode(index);
    bool round = index == LC_AbstractActionDrawRectangle::CORNER_RADIUS;
    bool bevel = index == LC_AbstractActionDrawRectangle::CORNER_BEVEL;

    ui->lblRadius->setVisible(round);
    ui->leRadius->setVisible(round);
    ui->tbPickRadius->setVisible(round);
    ui->cbSnapRadiusCenter->setVisible(round);

    ui->lblLenY->setVisible(bevel);
    ui->lblX->setVisible(bevel);
    ui->leLenY->setVisible(bevel);
    ui->tbPickLengthY->setVisible(bevel);
    ui->leX->setVisible(bevel);
    ui->tbPickLengthX->setVisible(bevel);

    bool straight = index == LC_AbstractActionDrawRectangle::CORNER_STRAIGHT || ui->cbQuadrangle->isChecked();
    ui->lblEdges->setVisible(straight);
    ui->cbEdges->setVisible(straight);

    ui->cbCorners->setCurrentIndex(index);
}

void LC_Rectangle3PointsOptions::onLenYEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leLenY->text();
        setLenYToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onLenXEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leX->text();
        setLenXToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onInnerAngleEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leInnerAngle->text();
        setInnerAngleToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onRadiusEditingFinished(){
    if (m_action != nullptr){
        QString value = ui->leRadius->text();
        setRadiusToActionAnView(value);
    }
}

void LC_Rectangle3PointsOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_Rectangle3PointsOptions::setAngleToActionAndView(const QString &val){
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)){
        m_action->setUcsAngleDegrees(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_Rectangle3PointsOptions::setLenYToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setLengthY(y);
        ui->leLenY->setText(fromDouble(y));
    }
}

void LC_Rectangle3PointsOptions::setLenXToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setLengthX(y);
        ui->leX->setText(fromDouble(y));
    }
}

void LC_Rectangle3PointsOptions::setRadiusToActionAnView(const QString& value){
    double y;
    if (toDouble(value, y, 1.0, true)){
        m_action->setRadius(y);
        ui->leRadius->setText(fromDouble(y));
    }
}

void LC_Rectangle3PointsOptions::onUsePolylineClicked(bool value){
    if (m_action != nullptr){
        setUsePolylineToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onSnapToCornerArcCenterClicked(bool value){
    if (m_action != nullptr){
        setSnapToCornerArcCenter(value);
    }
}
void LC_Rectangle3PointsOptions::onQuadrangleClicked(bool value){
    if (m_action != nullptr){
        setQuadrangleToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onInnerAngleFixedClicked(bool value){
    if (m_action != nullptr){
        setInnerAngleFixedToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::onBaseAngleFixedClicked(bool value){
    if (m_action != nullptr){
        setBaseAngleFixedToActionAndView(value);
    }
}

void LC_Rectangle3PointsOptions::setUsePolylineToActionAndView(bool value){
    m_action->setUsePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_Rectangle3PointsOptions::setSnapToCornerArcCenter(bool value){
    m_action->setSnapToCornerArcCenter(value);
    ui->cbSnapRadiusCenter->setChecked(value);
}

void LC_Rectangle3PointsOptions::setQuadrangleToActionAndView(bool value){
    m_action->setCreateQuadrangle(value);
    ui->cbQuadrangle->setChecked(value);
    ui->frmQuad->setVisible(value);
    ui->frmRectSettings->setVisible(!value);
    setCornersModeToActionAndView(ui->cbCorners->currentIndex());
}

void LC_Rectangle3PointsOptions::setInnerAngleFixedToActionAndView(bool value){
    ui->cbFixedInnerAngle->setChecked(value);
    m_action->setInnerAngleFixed(value);
    ui->leInnerAngle->setEnabled(value);
    ui->tbPickAngleInner->setEnabled(value);
}

void LC_Rectangle3PointsOptions::setBaseAngleFixedToActionAndView(bool value){
    ui->chkFixedBaseAngle->setChecked(value);
    m_action->setBaseAngleFixed(value);
    ui->leAngle->setEnabled(value);
    ui->tbPickAngleBase->setEnabled(value);
}

void LC_Rectangle3PointsOptions::setInnerAngleToActionAndView(const QString& value){
    double y;
    if (toDoubleAngleDegrees(value, y, 1.0, true)){
        m_action->setFixedInnerAngle(y);
        ui->leInnerAngle->setText(fromDouble(y));
    }
}

void LC_Rectangle3PointsOptions::onEdgesIndexChanged(int index){
    if (m_action != nullptr){
        setEdgesModeToActionAndView(index);
    }
}

void LC_Rectangle3PointsOptions::setEdgesModeToActionAndView(int index){
    m_action->setEdgesDrawMode(index);
    ui->cbEdges->setCurrentIndex(index);
}
