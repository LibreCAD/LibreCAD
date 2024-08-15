#include "lc_infodist2options.h"
#include "ui_lc_infodist2options.h"

LC_InfoDist2Options::LC_InfoDist2Options()
    : LC_ActionOptionsWidget(nullptr)
    , ui(new Ui::LC_InfoDist2Options){
    ui->setupUi(this);
    connect(ui->cbOnEntity, &QCheckBox::clicked, this, &LC_InfoDist2Options::onOnEntityClicked);
}

LC_InfoDist2Options::~LC_InfoDist2Options(){
    delete ui;
    action = nullptr;
}

void LC_InfoDist2Options::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionInfoDist2 *>(a);

    bool onEntity;
    if (update){
        onEntity = action->isUseNearestPointOnEntity();
    } else {
        onEntity = loadBool("NearestIsOnEntity", true);
    }
    setOnEntitySnapToActionAndView(onEntity);
}

QString LC_InfoDist2Options::getSettingsOptionNamePrefix(){
    return "/InfoDist2";
}

void LC_InfoDist2Options::doSaveSettings(){
    save("NearestIsOnEntity", ui->cbOnEntity->isChecked());
}

void LC_InfoDist2Options::onOnEntityClicked(bool value){
    if (action != nullptr){
        setOnEntitySnapToActionAndView(ui->cbOnEntity->isChecked());
    }
}

void LC_InfoDist2Options::setOnEntitySnapToActionAndView(bool value){
    action->setUseNearestPointOnEntity(value);
    ui->cbOnEntity->setChecked(value);
}

void LC_InfoDist2Options::languageChange(){
    ui->retranslateUi(this);
}

bool LC_InfoDist2Options::checkActionRttiValid(RS2::ActionType actionType){
        return actionType ==RS2::ActionInfoDistEntity2Point || actionType == RS2::ActionInfoDistPoint2Entity;
}
