#include "lc_lineanglereloptions.h"
#include "ui_lc_lineanglereloptions.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include "rs_math.h"

LC_LineAngleRelOptions::LC_LineAngleRelOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_LineAngleRelOptions)
{
    ui->setupUi(this);

    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onLengthEditingFinished);
    connect(ui->leOffset, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onOffsetEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineAngleRelOptions::onAngleEditingFinished);
    connect(ui->cbRelativeAngle, SIGNAL(clicked(bool)), this, SLOT(onAngleRelatedClicked(bool)));
    connect(ui->cbFree, SIGNAL(clicked(bool)), this, SLOT(onFreeLengthClicked(bool)));
    connect(ui->cbTickSnapMode, SIGNAL(currentIndexChanged(int)), SLOT(onTickSnapModeIndexChanged(int)));
    connect(ui->cbLineSnapMode, SIGNAL(currentIndexChanged(int)), SLOT(onLineSnapModeIndexChanged(int)));
}

LC_LineAngleRelOptions::~LC_LineAngleRelOptions(){
    saveSettings();
    delete ui;
}

bool LC_LineAngleRelOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineAngleRel || actionType == RS2::ActionDrawLineOrthogonalRel;
}

void LC_LineAngleRelOptions::clearAction(){
  action = nullptr;
}

void LC_LineAngleRelOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = static_cast<LC_ActionDrawLineAngleRel*>(a);
    fixedAngle = a->rtti()==RS2::ActionDrawLineOrthogonalRel;
        QString length;
        QString offset;
        QString angle;
        int lineSnapMode;
        int tickSnapMode;
        bool angleIsRelative;
        bool lengthIsFree;
        if (update) {
            length = QString::number(action->getTickLength(), 'g', 6);
            offset = QString::number(action->getTickOffset(), 'g', 6);
            angle = QString::number(action->getTickAngle(), 'g', 6);
            lineSnapMode = action->getLineSnapMode();
            tickSnapMode = action->getTickSnapMode();
            angleIsRelative = action->isAngleRelative();
            lengthIsFree = action->isLengthFree();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            length = RS_SETTINGS->readEntry(keyName("Length"), "1.0");
            offset = RS_SETTINGS->readEntry(keyName("Offset"), "1.0");
            if (!fixedAngle){
                angle = RS_SETTINGS->readEntry(keyName("Angle"), "1.0");
                angleIsRelative = RS_SETTINGS->readNumEntry(keyName("AngleIsRelative"), 1) == 1 ? true : false;
            }
            lengthIsFree = RS_SETTINGS->readNumEntry(keyName("LengthIsFree"), 0) == 1 ? true : false;
            lineSnapMode = RS_SETTINGS->readNumEntry(keyName("LineSnapMode"), 0);
            tickSnapMode = RS_SETTINGS->readNumEntry(keyName("TickSnapMode"), 1);
            RS_SETTINGS->endGroup();
        }
        ui->leAngle->setVisible(!fixedAngle);
        ui->lblAngle->setVisible(!fixedAngle);
        ui->cbRelativeAngle->setVisible(!fixedAngle);

        setLengthIsFreeToActionAndView(lengthIsFree);
        setLengthToActionAndView(length);
        setOffsetToActionAndView(offset);
        if (!fixedAngle){
            setAngleToActionAndView(angle);
            setAngleIsRelativeToActionAndView(angleIsRelative);
        }
        setLineSnapModeToActionAndView(lineSnapMode);
        setTickSnapModeToActionAndView(tickSnapMode);

}

QString LC_LineAngleRelOptions::keyName(QString key){
    QString result = fixedAngle ? "/LineOrthogonalRel" : "/LineAngleRel" + key;
    return result;
}

void LC_LineAngleRelOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry(keyName("Length"), ui->leLength->text());
    if (!fixedAngle){
        RS_SETTINGS->writeEntry(keyName("Angle"), ui->leAngle->text());
        RS_SETTINGS->writeEntry(keyName("AngleIsRelative"), ui->cbRelativeAngle->isChecked()  ? 1 : 0);
    }
    RS_SETTINGS->writeEntry(keyName("LengthIsFree"), ui->cbFree->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry(keyName("Offset"), ui->leOffset->text());
    RS_SETTINGS->writeEntry(keyName("LineSnapMode"), ui->cbLineSnapMode->currentIndex());
    RS_SETTINGS->writeEntry(keyName("TickSnapMode"), ui->cbTickSnapMode->currentIndex());
    RS_SETTINGS->endGroup();
}

void LC_LineAngleRelOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineAngleRelOptions::setAngleToActionAndView(const QString &expr){
    bool ok = false;
    double angle =RS_Math::eval(expr, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setTickAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}


void LC_LineAngleRelOptions::setLengthToActionAndView(QString val){
    bool ok = false;
    double value =std::abs(RS_Math::eval(val, &ok));
    if(!ok) return;
    if (std::abs(value) < RS_TOLERANCE) value=1.0;
    action->setTickLength(value);
    ui->leLength->setText(QString::number(value, 'g', 6));
}

void LC_LineAngleRelOptions::setOffsetToActionAndView(QString val){
    bool ok = false;
    double value =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(value) < RS_TOLERANCE) value=0.0;
    action->setTickOffset(value);
    ui->leOffset->setText(QString::number(value, 'g', 6));
}


void LC_LineAngleRelOptions::setAngleIsRelativeToActionAndView(bool relative){
    action->setAngleIsRelative(relative);
    ui->cbRelativeAngle->setChecked(relative);
}

void LC_LineAngleRelOptions::setLengthIsFreeToActionAndView(bool free){
    action->setLengthIsFree(free);
    ui->cbFree->setChecked(free);
}

void LC_LineAngleRelOptions::setTickSnapModeToActionAndView(int mode){
    action->setTickSnapMode(mode);
    ui->cbTickSnapMode->setCurrentIndex(mode);
}

void LC_LineAngleRelOptions::setLineSnapModeToActionAndView(int mode){
    action->setLineSnapMode(mode);
    ui->cbLineSnapMode->setCurrentIndex(mode);
}

void LC_LineAngleRelOptions::onLengthEditingFinished(){
    const QString &expr = ui->leLength->text();
    setLengthToActionAndView(expr);
    saveSettings();
}

void LC_LineAngleRelOptions::onOffsetEditingFinished(){
    const QString &expr = ui->leOffset->text();
    setOffsetToActionAndView(expr);
    saveSettings();
}

void LC_LineAngleRelOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
    saveSettings();
}

void LC_LineAngleRelOptions::onLineSnapModeIndexChanged(int index){
    setLineSnapModeToActionAndView(index);
    saveSettings();
}

void LC_LineAngleRelOptions::onTickSnapModeIndexChanged(int index){
    setTickSnapModeToActionAndView(index);
    saveSettings();
}

void LC_LineAngleRelOptions::onFreeLengthClicked(bool clicked){
    setLengthIsFreeToActionAndView(clicked);
    saveSettings();
}

void LC_LineAngleRelOptions::onAngleRelatedClicked(bool clicked){
    setAngleIsRelativeToActionAndView(clicked);
    saveSettings();
}






