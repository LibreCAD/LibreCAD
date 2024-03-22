#include "lc_linefrompointtolineoptions.h"
#include "ui_lc_linefrompointtolineoptions.h"
#include "rs_settings.h"
#include "rs_math.h"

LC_LineFromPointToLineOptions::LC_LineFromPointToLineOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_LineFromPointToLineOptions)
{
    ui->setupUi(this);

    connect(ui->cbOrthogonal, SIGNAL(clicked(bool)), this, SLOT(onOrthogonalClicked(bool)));
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineFromPointToLineOptions::onAngleEditingFinished);
    connect(ui->cbSizeMode, SIGNAL(currentIndexChanged(int)), SLOT(onSizeModeIndexChanged(int)));
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineFromPointToLineOptions::onLengthEditingFinished);
    connect(ui->cbSnap, SIGNAL(currentIndexChanged(int)), SLOT(onSnapModeIndexChanged(int)));
}

LC_LineFromPointToLineOptions::~LC_LineFromPointToLineOptions(){
    saveSettings();
    delete ui;
}

void LC_LineFromPointToLineOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineFromPointToLineOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("LinePointToLineOrthogonal", ui->cbOrthogonal->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("LinePointToLineAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("LinePointToLineLength", ui->leLength->text());
    RS_SETTINGS->writeEntry("LinePointToLineSnapMode", ui->cbSnap->currentIndex());
    RS_SETTINGS->writeEntry("LinePointToLineSizeMode", ui->cbSizeMode->currentIndex());
    RS_SETTINGS->endGroup();
}

void LC_LineFromPointToLineOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionDrawLineFromPointToLine *>(a);
    bool ortho = false;
    QString angle = QString();
    int sizeMode = 0;
    QString length = QString();
    int snap = 0;

    if (update){
        ortho = action->getOrthogonal();
        sizeMode = action->getSizeMode();
        snap = action->getLineSnapMode();
        angle = QString::number(action->getAngle(), 'g', 6);
        length = QString::number(action->getLength(), 'g', 6);
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        ortho = RS_SETTINGS->readNumEntry("LinePointToLineOrthogonal", 1) == 1;
        angle = RS_SETTINGS->readEntry("LinePointToLineAngle", "90");
        length = RS_SETTINGS->readEntry("LinePointToLineLength", "100");
        snap = RS_SETTINGS->readNumEntry("LinePointToLineSnapMode", 0);
        sizeMode = RS_SETTINGS->readNumEntry("LinePointToLineSizeMode", 0);
        RS_SETTINGS->endGroup();
    }
    setOrthogonalToActionAndView(ortho);
    setAngleToActionAndView(angle);
    setSizeModelIndexToActionAndView(sizeMode);
    setLengthToActionAndView(length);
    setSnapModeToActionAndView(snap);
    
}

void LC_LineFromPointToLineOptions::clearAction(){
    action = nullptr;
}

void LC_LineFromPointToLineOptions::onSnapModeIndexChanged(int index){
    if (action != nullptr){
        setSnapModeToActionAndView(index);
    }
}

void LC_LineFromPointToLineOptions::onSizeModeIndexChanged(int index){
    if (action != nullptr){
        setSizeModelIndexToActionAndView(index);
    }
}

void LC_LineFromPointToLineOptions::onOrthogonalClicked(bool value){
    if (action != nullptr){
        setOrthogonalToActionAndView(value);
    }
}


bool LC_LineFromPointToLineOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineFromPointToLine;
}

void LC_LineFromPointToLineOptions::onAngleEditingFinished(){
    if (action != nullptr){
        setAngleToActionAndView(ui->leAngle->text());
    }
}

void LC_LineFromPointToLineOptions::onLengthEditingFinished(){
    if (action != nullptr){
        setLengthToActionAndView(ui->leLength->text());
    }
}

void LC_LineFromPointToLineOptions::setSnapModeToActionAndView(int index){
    action->setLineSnapMode(index);
    ui->cbSnap->setCurrentIndex(index);
}

void LC_LineFromPointToLineOptions::setSizeModelIndexToActionAndView(int index){
    action->setSizeMode(index);
    ui->cbSizeMode->setCurrentIndex(index);
    bool intersectionMode = index == 0;
    ui->frmLength->setVisible(!intersectionMode);
}

void LC_LineFromPointToLineOptions::setAngleToActionAndView(QString value){
    bool ok = false;
    double angle =RS_Math::eval(value, &ok);
    if(!ok) return;
    if (std::abs(angle) < RS_TOLERANCE_ANGLE) angle=0.0;

    // ensure angle in 0..180
    double angleRad = RS_Math::deg2rad(angle);
    double correctedAngle = RS_Math::correctAngle3(angleRad);
    angle = RS_Math::rad2deg(correctedAngle);

    action->setAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_LineFromPointToLineOptions::setLengthToActionAndView(QString value){
    bool ok = false;
    double len =RS_Math::eval(value, &ok);
    if(!ok) return;
    if (std::abs(len) < RS_TOLERANCE) len=1.0;
    action->setLength(len);
    ui->leLength->setText(QString::number(len, 'g', 6));
}

void LC_LineFromPointToLineOptions::setOrthogonalToActionAndView(bool value){
    action->setOrthogonal(value);
    ui->cbOrthogonal->setChecked(value);

    ui->lblAngle->setEnabled(!value);
    ui->leAngle->setEnabled(!value);
}
