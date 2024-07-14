#include "lc_modifymirroroptions.h"
#include "ui_lc_modifymirroroptions.h"

LC_ModifyMirrorOptions::LC_ModifyMirrorOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyMirror,"/Modify", "/Mirror")
    , ui(new Ui::LC_ModifyMirrorOptions), action(nullptr){
    ui->setupUi(this);

    connect(ui->cbMirrorToLine, SIGNAL(clicked(bool)), this, SLOT(onMirrorToLineClicked(bool)));
}

LC_ModifyMirrorOptions::~LC_ModifyMirrorOptions(){
    delete ui;
    action = nullptr;
}

void LC_ModifyMirrorOptions::doSaveSettings() {
    save("ToLine", ui->cbMirrorToLine->isChecked());
}

void LC_ModifyMirrorOptions::onMirrorToLineClicked(bool clicked){
    setMirrorToLineLineToActionAndView(clicked);
}

void LC_ModifyMirrorOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyMirror *>(a);
    bool useLine;
    if (update){
        useLine = action->isMirrorToExistingLine();
    }
    else{
        useLine = loadBool("ToLine", false);
    }
    setMirrorToLineLineToActionAndView(useLine);
}

void LC_ModifyMirrorOptions::setMirrorToLineLineToActionAndView(bool value){
    action->setMirrorToExistingLine(value);
    ui->cbMirrorToLine->setChecked(value);
}

void LC_ModifyMirrorOptions::languageChange() {
    ui->retranslateUi(this);
}