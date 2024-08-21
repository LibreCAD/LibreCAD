#include "lc_lineoptions.h"


#include "rs_actiondrawline.h"
#include "ui_lc_lineoptions.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "lc_abstractactiondrawline.h"

/*
 *  Constructs a QG_LineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineOptions::LC_LineOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawSnakeLine, "Draw","LineSnake")
    , ui(new Ui::Ui_LineOptionsRel{})
{
    ui->setupUi(this);
    connect(ui->rbX, &QRadioButton::clicked, this, &LC_LineOptions::onXClicked);
    connect(ui->rbY, &QRadioButton::clicked, this, &LC_LineOptions::onYClicked);
    connect(ui->rbPoint, &QRadioButton::clicked, this, &LC_LineOptions::onPointClicked);
    connect(ui->rbAngle, &QRadioButton::toggled, this, &LC_LineOptions::onAngleClicked);
    connect(ui->cbRelAngle, &QCheckBox::clicked, this, &LC_LineOptions::onAngleRelativeClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineOptions::onSetAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineOptions::~LC_LineOptions(){
    action = nullptr;
    delete ui;
};

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineOptions::doSaveSettings(){
    save("Angle", ui->leAngle->text());
    save("AngleRelative", ui->cbRelAngle->isChecked());
}

void LC_LineOptions::doSetAction(RS_ActionInterface* a, bool update) {
    action = dynamic_cast<LC_ActionDrawLineSnake *>(a);

    // prevent cycle invocation
    if (inUpdateCycle){
        return;
    }
    inUpdateCycle = true;

    QString angle;
    bool angleRelative;

    ui->bClose->setEnabled(action->mayClose());
    ui->bUndo->setEnabled(action->mayUndo());
    ui->bRedo->setEnabled(action->mayRedo());
    ui->bPolyline->setEnabled(action->mayClose());

    int direction = action->getDirection();

    ui->rbPoint->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_POINT);
    ui->rbX->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_X);
    ui->rbY->setChecked(direction == LC_AbstractActionDrawLine::DIRECTION_Y);
    bool angleDirection = direction == LC_AbstractActionDrawLine::DIRECTION_ANGLE;

    setupAngleRelatedUI(angleDirection);

    if (update){
        angle = fromDouble(action->getAngle());
        angleRelative = action->isAngleRelative();
    } else {
        angle = load("Angle", "0.0");
        angleRelative = loadBool("AngleRelative", false);
    }

    setAngleToActionAndView(angle, false);
    setAngleRelativeToActionAndView(angleRelative);
    inUpdateCycle = false;
}

void LC_LineOptions::onXClicked(bool value){
    if (action != nullptr){
        setXDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onYClicked(bool value){
    if (action != nullptr){
        setYDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onPointClicked(bool value){
    if (action != nullptr){
        setPointDirectionToActionAndView(value);
    }
}

void LC_LineOptions::onAngleClicked(bool value){
    if (action != nullptr){
        setAngleDirectionToActionAndView(value);
    }
}

void LC_LineOptions::closeLine() {
    if (action != nullptr) {
        action->close();
    }
}

void LC_LineOptions::undo() {
    if (action != nullptr) {
        action->undo();
    }
}

void LC_LineOptions::redo() {
    if (action != nullptr) {
        action->redo();
    }
}

void LC_LineOptions::polyline() {
    if (action != nullptr) {
        action->polyline();
    }
}

void LC_LineOptions::onAngleRelativeClicked(bool value){
    if (action != nullptr) {
        setAngleRelativeToActionAndView(value);
    }
}

void LC_LineOptions::onSetAngle() {
    if (action != nullptr){
        setAngleToActionAndView(ui->leAngle->text(), true);
    }
}

void LC_LineOptions::start() {
    if (action) {
        action->setNewStartPointState();
    }
}

void LC_LineOptions::setXDirectionToActionAndView(bool value){
    if (value){
        action->setSetXDirectionState();
    }
    ui->rbX->setChecked(value);
}

void LC_LineOptions::setYDirectionToActionAndView(bool value){
    if (value){
        action->setSetYDirectionState();
    }
    ui->rbY->setChecked(value);
}

void LC_LineOptions::setAngleDirectionToActionAndView(bool value){
    if (value){
        action->setSetAngleDirectionState();
    }
    setupAngleRelatedUI(value);
}

void LC_LineOptions::setupAngleRelatedUI(bool value){
    ui->rbAngle->setChecked(value);
    ui->leAngle->setEnabled(value);
    ui->cbRelAngle->setEnabled(value);
}

void LC_LineOptions::setPointDirectionToActionAndView(bool value){
    if (value){
        action->setSetPointDirectionState();
    }
    ui->rbPoint->setChecked(value);
}

void LC_LineOptions::setAngleToActionAndView(const QString& val, bool affectState){
    double angle;
    if (toDoubleAngle(val, angle, 0.0, false)){
        if (affectState){
            action->setAngleValue(angle);
        }
        else {
            action->setAngle(angle);
        }
        ui->leAngle->setText(fromDouble(angle));
    }
}

void LC_LineOptions::setAngleRelativeToActionAndView(bool relative){
    action->setAngleIsRelative(relative);
    ui->cbRelAngle->setChecked(relative);
}
