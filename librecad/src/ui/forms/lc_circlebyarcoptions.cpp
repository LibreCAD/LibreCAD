#include "lc_circlebyarcoptions.h"
#include "ui_lc_circlebyarcoptions.h"
#include "rs_settings.h"

LC_CircleByArcOptions::LC_CircleByArcOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_CircleByArcOptions)
{
    ui->setupUi(this);
    connect(ui->cbReplace, SIGNAL(clicked(bool)), this, SLOT(onReplaceClicked(bool)));
}

LC_CircleByArcOptions::~LC_CircleByArcOptions(){
    delete ui;
}

void LC_CircleByArcOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionDrawCircleByArc *>(a);
    bool replace;
    if (update){
        replace = action->isReplaceArcByCircle();
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        replace = RS_SETTINGS->readNumEntry("/CircleByArcReplaceArc", 0) == 1;
        RS_SETTINGS->endGroup();
    }
    setReplaceArcToActionAndView(replace);
}

void LC_CircleByArcOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/CircleByArcReplaceArc", ui->cbReplace->isChecked()  ? 1: 0);
    RS_SETTINGS->endGroup();
}

void LC_CircleByArcOptions::clearAction(){
    action = nullptr;
}

bool LC_CircleByArcOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawCircleByArc;
}

void LC_CircleByArcOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_CircleByArcOptions::onReplaceClicked(bool value){
    if (action != nullptr){
        setReplaceArcToActionAndView(value);
    }
}

void LC_CircleByArcOptions::setReplaceArcToActionAndView(bool value){
    action->setReplaceArcByCircle(value);
    ui->cbReplace->setChecked(value);
}

