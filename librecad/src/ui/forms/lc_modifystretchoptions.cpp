#include "lc_modifystretchoptions.h"
#include "ui_lc_modifystretchoptions.h"

LC_ModifyStretchOptions::LC_ModifyStretchOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyStretch, "/Modify", "/Stretch")
    , ui(new Ui::LC_ModifyStretchOptions){
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ModifyStretchOptions::onKeepOriginalsClicked);
}

LC_ModifyStretchOptions::~LC_ModifyStretchOptions(){
    delete ui;
    action = nullptr;
}

void LC_ModifyStretchOptions::doSaveSettings() {
    save("KeepOriginals", ui->cbKeepOriginals->isChecked());
}

void LC_ModifyStretchOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyStretch *>(a);
    bool keepOriginals;
    if (update){
        keepOriginals = !action->isRemoveOriginals();
    }
    else{
        keepOriginals = loadBool("KeepOriginals", false);
    }
    setKeepOriginalsToActionAndView(keepOriginals);
}

void LC_ModifyStretchOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_ModifyStretchOptions::onKeepOriginalsClicked(bool val) {
    setKeepOriginalsToActionAndView(val);
}

void LC_ModifyStretchOptions::setKeepOriginalsToActionAndView(bool val) {
    ui->cbKeepOriginals->setChecked(val);
    action->setRemoveOriginals(!val);
}
