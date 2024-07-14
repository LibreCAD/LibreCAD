#include "lc_moveoptions.h"
#include "ui_lc_moveoptions.h"

LC_MoveOptions::LC_MoveOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionModifyMove, "/Modify", "/Move")
    , ui(new Ui::LC_MoveOptions)
{
    ui->setupUi(this);
}

LC_MoveOptions::~LC_MoveOptions(){
    delete ui;
}

void LC_MoveOptions::doSaveSettings() {
//    if (ui->cbMultipleCopies->isChecked()){
//
//    }
}

void LC_MoveOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionModifyMove *>(a);
    bool useMultipleCopies;
    int copiesNumber;
    int mode;
    if (update){

    }
    else{
        useMultipleCopies = loadInt("Mode", 0) == 2;
        copiesNumber = loadInt("Copies", 1);
    }
    setUseMultipleCopiesToActionAndView(useMultipleCopies);
    setCopiesNumberToActionAndView(copiesNumber);
}

void LC_MoveOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_MoveOptions::setCopiesNumberToActionAndView(int number) {

}

void LC_MoveOptions::setUseMultipleCopiesToActionAndView(bool copies) {

}
