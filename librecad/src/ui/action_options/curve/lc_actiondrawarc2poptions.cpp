#include "lc_actiondrawarc2poptions.h"
#include "ui_lc_actiondrawarc2poptions.h"
#include "rs_math.h"

LC_ActionDrawArc2POptions::LC_ActionDrawArc2POptions(int actionType)
    : LC_ActionOptionsWidget()
    , ui(new Ui::LC_ActionDrawArc2POptions){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_ActionDrawArc2POptions::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &LC_ActionDrawArc2POptions::onDirectionChanged);
    connect(ui->leValue, &QLineEdit::editingFinished, this, &LC_ActionDrawArc2POptions::onValueChanged);
    supportedActionType = actionType;

    ui->lRadius->setVisible(false);
    ui->lLength->setVisible(false);
    ui->lHeight->setVisible(false);
    ui->lAngle->setVisible(false);

    switch (actionType){
        case RS2::ActionDrawArc2PRadius: {
            updateTooltip(ui->lRadius);
            optionNamePrefix = "Arc2PRadius";
            break;
        }
        case RS2::ActionDrawArc2PHeight:{
            updateTooltip(ui->lHeight);
            optionNamePrefix = "Arc2PHeight";
            break;
        }
        case RS2::ActionDrawArc2PLength:{
            updateTooltip(ui->lLength);
            optionNamePrefix = "Arc2PLength";
            break;
        }
        case RS2::ActionDrawArc2PAngle:{
            updateTooltip(ui->lAngle);
            optionNamePrefix = "Arc2PAngle";
            break;
        }
        default:
            break;
    }

}

void LC_ActionDrawArc2POptions::updateTooltip( QLabel *label) const {
    QString toolTip;
    toolTip = label->toolTip();
    label->setToolTip("");
    ui->leValue->setToolTip(toolTip);
    label->setVisible(true);
}

LC_ActionDrawArc2POptions::~LC_ActionDrawArc2POptions(){
    delete ui;
}

void LC_ActionDrawArc2POptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_ActionDrawArc2POptions::doSaveSettings(){
    save("Reversed",  ui->rbNeg->isChecked());
    save("Parameter", ui->leValue->text());
}

bool LC_ActionDrawArc2POptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == supportedActionType;
}

void LC_ActionDrawArc2POptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    action->setReversed(reversed);
}

/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/
void LC_ActionDrawArc2POptions::onDirectionChanged(bool /*pos*/){
    setReversedToActionAndView( ui->rbNeg->isChecked());
}
void LC_ActionDrawArc2POptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<LC_ActionDrawArc2PointsBase *>(a);
    bool reversed;
    QString parameter;
    if (update){
        reversed = action->isReversed();
        double param = action->getParameter();
        if (supportedActionType == RS2::ActionDrawArc2PAngle){
            param = RS_Math::rad2deg(param);
        }
        parameter = fromDouble(param);
    }
    else{
        reversed = loadBool("Reversed", false);
        parameter = load("Parameter", "1.0");
    }
    setReversedToActionAndView(reversed);
    setParameterToActionAndView(parameter);
}

QString LC_ActionDrawArc2POptions::getSettingsOptionNamePrefix() {
    return optionNamePrefix;
}

void LC_ActionDrawArc2POptions::onValueChanged() {
    setParameterToActionAndView(ui->leValue->text());
}

void LC_ActionDrawArc2POptions::setParameterToActionAndView(QString val) {
    double param;
    if (supportedActionType == RS2::ActionDrawArc2PAngle){
        if (toDoubleAngle(val, param, 1.0, true)){
            ui->leValue->setText(fromDouble(param));
            param = RS_Math::deg2rad(param);
            action->setParameter(param);
        }
    }
    else if (toDouble(val, param, 1.0, true)){
        action->setParameter(param);
        ui->leValue->setText(fromDouble(param));
    }
}
