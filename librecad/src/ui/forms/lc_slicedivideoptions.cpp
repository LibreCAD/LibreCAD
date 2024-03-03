#include "lc_slicedivideoptions.h"
#include "ui_lc_slicedivideoptions.h"
#include "rs_actioninterface.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_debug.h"

LC_SliceDivideOptions::LC_SliceDivideOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LC_SliceDivideOptions)
{
    ui->setupUi(this);

    connect(ui->leCount, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onCountEditingFinished);
    connect(ui->leTickLengh, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickLengthEditingFinished);
    connect(ui->leTickOffset, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickOffsetEditingFinished);
    connect(ui->leTickAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onTickAngleEditingFinished);
    connect(ui->leCircleStartAngle, &QLineEdit::editingFinished, this, &LC_SliceDivideOptions::onCircleStartAngleEditingFinished);
    connect(ui->cbEdgeTick, SIGNAL(currentIndexChanged(int)),this, SLOT(onDrawTickOnEdgesIndexChanged(int)));
    connect(ui->cbTickSnap, SIGNAL(currentIndexChanged(int)), this, SLOT(onTickSnapIndexChanged(int)));
    connect(ui->cbRelAngle, SIGNAL(clicked(bool)), this, SLOT(onRelAngleClicked(bool)));
    connect(ui->cbDivide, SIGNAL(clicked(bool)), this, SLOT(onDivideClicked(bool)));
}

LC_SliceDivideOptions::~LC_SliceDivideOptions(){
    saveSettings();
    delete ui;
}



void LC_SliceDivideOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti() == RS2::ActionDrawSliceDivide){
        action = static_cast<LC_ActionDrawSliceDivide *>(a);

        QString count;
        QString tickLen;
        QString tickOffset;
        QString tickAngle;
        QString circleStartAngle;
        int drawEdgesMode;
        int tickSnapMode;
        bool tickAngleRelative;
        bool divide;
        LC_ERR<<__func__<<"(): update: "<<update;
        if (update){
            count = QString::number(action->getTickCount(), 'g', 6);
            tickLen = QString::number(action->getTickLength(), 'g', 6);
            tickOffset = QString::number(action->getTickOffset(), 'g', 6);
            tickAngle = QString::number(action->getTickAngle(), 'g', 6);
            circleStartAngle = QString::number(action->getCircleStartAngle(), 'g', 6);
            tickSnapMode = action->getTickSnapMode();
            drawEdgesMode = action->getDrawTickOnEdgeMode();
            tickAngleRelative = action->isTickAngleRelative();
            divide = action->isDivideEntity();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            count = RS_SETTINGS->readEntry("/SliceDivideCount", "1.0");
            tickLen = RS_SETTINGS->readEntry("/SliceDivideLength", "1.0");
            tickOffset = RS_SETTINGS->readEntry("/SliceDivideOffset", "0.0");
            tickAngle = RS_SETTINGS->readEntry("/SliceDivideAngle", "90.0");
            tickSnapMode = RS_SETTINGS->readNumEntry("/SliceDivideTickSnap", 0);
            drawEdgesMode = RS_SETTINGS->readNumEntry("/SliceDivideTickEdgeMode", 0);
            circleStartAngle = RS_SETTINGS->readEntry("/SliceDivideCircleStartAngle", "0.0");
            tickAngleRelative = RS_SETTINGS->readNumEntry("/SliceDivideLengthTickAngleRel", 1) == 1;
            divide = RS_SETTINGS->readNumEntry("/SliceDivideDoDivide", 1) == 1;
            RS_SETTINGS->endGroup();
        }
        setCountToActionAndView(count);
        setTickLengthToActionAndView(tickLen);
        setTickOffsetToActionAndView(tickOffset);
        setTickAngleToActionAndView(tickAngle);
        setCircleStartAngleToActionAndView(circleStartAngle);
        setTicksSnapModeToActionAndView(tickSnapMode);
        setDrawEdgesTicksModeToActionAndView(drawEdgesMode);
        setTickAngleRelativeToActionAndView(tickAngleRelative);
        setDivideFlagToActionAndView(divide);

    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDrawSliceDivide::setAction: wrong action type");
        action = nullptr;
    }
}

void LC_SliceDivideOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/SliceDivideCount", ui->leCount->text());
    RS_SETTINGS->writeEntry("/SliceDivideLength", ui->leTickLengh->text());
    RS_SETTINGS->writeEntry("/SliceDivideOffset", ui->leTickOffset->text());
    RS_SETTINGS->writeEntry("/SliceDivideAngle", ui->leTickAngle->text());
    RS_SETTINGS->writeEntry("/SliceDivideTickSnap", ui->cbTickSnap->currentIndex());
    RS_SETTINGS->writeEntry("/SliceDivideTickEdgeMode", ui->cbEdgeTick->currentIndex());
    RS_SETTINGS->writeEntry("/SliceDivideCircleStartAngle", ui->leCircleStartAngle->text());
    RS_SETTINGS->writeEntry("/SliceDivideLengthTickAngleRel", ui->cbRelAngle->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/SliceDivideDoDivide", ui->cbDivide->isChecked()  ? 1 : 0);
    RS_SETTINGS->endGroup();
}

void LC_SliceDivideOptions::onCountEditingFinished(){
    const QString &expr = ui->leCount->text();
    setCountToActionAndView(expr);
    saveSettings();
}

void LC_SliceDivideOptions::onTickLengthEditingFinished(){
    const QString &expr = ui->leTickLengh->text();
    setTickLengthToActionAndView(expr);
    saveSettings();
}

void LC_SliceDivideOptions::onTickAngleEditingFinished(){
    const QString &expr = ui->leTickAngle->text();
    setTickAngleToActionAndView(expr);
    saveSettings();
}

void LC_SliceDivideOptions::onTickOffsetEditingFinished(){
    const QString &expr = ui->leTickOffset->text();
    setTickOffsetToActionAndView(expr);
    saveSettings();
}

void LC_SliceDivideOptions::onCircleStartAngleEditingFinished(){
    const QString &expr = ui->leCircleStartAngle->text();
    setCircleStartAngleToActionAndView(expr);
    saveSettings();
}

void LC_SliceDivideOptions::onDrawTickOnEdgesIndexChanged(int index){
    setDrawEdgesTicksModeToActionAndView(index);
    saveSettings();
}

void LC_SliceDivideOptions::onRelAngleClicked(bool checked){
    LC_ERR<<__func__<<"(): checked: "<<checked;
    setTickAngleRelativeToActionAndView(checked);
    saveSettings();
}

void LC_SliceDivideOptions::onDivideClicked(bool checked){
    setDivideFlagToActionAndView(checked);
    saveSettings();
}

void LC_SliceDivideOptions::onTickSnapIndexChanged(int index){
    setTicksSnapModeToActionAndView(index);
    saveSettings();
}

void LC_SliceDivideOptions::setDrawEdgesTicksModeToActionAndView(int index){
    action->setDrawTickOnEdgeMode(index);
    ui->cbEdgeTick->setCurrentIndex(index);
}

void LC_SliceDivideOptions::setTicksSnapModeToActionAndView(int index){
    action->setTickSnapMode(index);
    ui->cbTickSnap->setCurrentIndex(index);
}

void LC_SliceDivideOptions::setTickAngleRelativeToActionAndView(bool relative){
    action->setTickAngleRelative(relative);
    ui->cbRelAngle->setChecked(relative);
}

void LC_SliceDivideOptions::setDivideFlagToActionAndView(bool value){
    action->setDivideEntity(value);
    ui->cbDivide->setChecked(value);
}

void LC_SliceDivideOptions::setCountToActionAndView(const QString &val){
    bool ok = false;
    // fixme - ensure that only int value is there
    int value =std::abs(RS_Math::eval(val, &ok));
    if(!ok) return;

    action->setTickCount(value);
    ui->leCount->setText(QString::number(value, 'g', 6));
}

void LC_SliceDivideOptions::setTickLengthToActionAndView(const QString &val){
    bool ok = false;
    double value = std::abs(RS_Math::eval(val, &ok));
    if(!ok) return;
    if (std::abs(value) < RS_TOLERANCE) value=0.0;
    action->setTickLength(value);
    ui->leTickLengh->setText(QString::number(value, 'g', 6));
}

void LC_SliceDivideOptions::setTickAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle = RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setTickAngle(angle);
    ui->leTickAngle->setText(QString::number(angle, 'g', 6));
}

void LC_SliceDivideOptions::setTickOffsetToActionAndView(const QString &val){
    bool ok = false;
    double value =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(value) < RS_TOLERANCE) value=0.0;
    action->setTickOffset(value);
    ui->leTickOffset->setText(QString::number(value, 'g', 6));
}

void LC_SliceDivideOptions::setCircleStartAngleToActionAndView(const QString &val){
    bool ok = false;
    double angle =RS_Math::eval(val, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setCircleStartTickAngle(angle);
    ui->leCircleStartAngle->setText(QString::number(angle, 'g', 6));
}

void LC_SliceDivideOptions::languageChange(){
    ui->retranslateUi(this);
}

