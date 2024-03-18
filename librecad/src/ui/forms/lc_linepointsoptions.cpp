#include "lc_linepointsoptions.h"
#include "ui_lc_linepointsoptions.h"
#include "rs_settings.h"
#include "rs_math.h"

LC_LinePointsOptions::LC_LinePointsOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_LinePointsOptions){
    ui->setupUi(this);
    connect(ui->lePointsCount, &QLineEdit::editingFinished, this, &LC_LinePointsOptions::onPointsCountEditingFinished);
    connect(ui->cbEdgePoints, SIGNAL(currentIndexChanged(int)), SLOT(onEdgePointsModeIndexChanged(int)));
}

LC_LinePointsOptions::~LC_LinePointsOptions(){
    delete ui;
}

void LC_LinePointsOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionDrawLinePoints *>(a);
    QString pointsCount;
    int edgePointMode;
    if (update){
        pointsCount = QString::number(action->getPointsCount(), 'g', 6);
        edgePointMode = action->getEdgePointsMode();
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        pointsCount = RS_SETTINGS->readEntry("/LinePointsCount", "1");
        edgePointMode = RS_SETTINGS->readNumEntry("/LinePointsEdgeMode", 1);
        RS_SETTINGS->endGroup();
    }
    setPointsCountActionAndView(pointsCount);
    setEdgePointsModeToActionAndView(edgePointMode);
}

void LC_LinePointsOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/LinePointsCount", ui->lePointsCount->text());
    RS_SETTINGS->writeEntry("/LinePointsEdgeMode", ui->cbEdgePoints->currentIndex());
    RS_SETTINGS->endGroup();
}

void LC_LinePointsOptions::clearAction(){
    action = nullptr;
}

void LC_LinePointsOptions::onPointsCountEditingFinished(){
    if (action != nullptr){
        setPointsCountActionAndView(ui->lePointsCount->text());
    }
}

void LC_LinePointsOptions::languageChange(){
    ui->retranslateUi(this);
}

bool LC_LinePointsOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLinePoints;
}

void LC_LinePointsOptions::setPointsCountActionAndView(QString val){
    bool ok = false;
    // fixme - ensure that only int value is there
    int value =std::abs(RS_Math::eval(val, &ok));
    if(!ok) return;
    if (value < 1){
        value = 1;
    }
    action->setPointsCount(value);
    ui->lePointsCount->setText(QString::number(value, 'g', 6));
}

void LC_LinePointsOptions::onEdgePointsModeIndexChanged(int index){
    if (action != nullptr){
        setEdgePointsModeToActionAndView(index);
    }
}

void LC_LinePointsOptions::setEdgePointsModeToActionAndView(int index){
    action->setEdgePointsMode(index);
    ui->cbEdgePoints->setCurrentIndex(index);
}
