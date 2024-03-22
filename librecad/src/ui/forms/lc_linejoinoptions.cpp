#include "lc_linejoinoptions.h"
#include "ui_lc_linejoinoptions.h"
#include "rs_settings.h"

LC_LineJoinOptions::LC_LineJoinOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_LineJoinOptions)
{
    ui->setupUi(this);

    connect(ui->cbLine1EdgeMode, SIGNAL(currentIndexChanged(int)), SLOT(onEdgeModelLine1IndexChanged(int)));
    connect(ui->cbLine2EdgeMode, SIGNAL(currentIndexChanged(int)), SLOT(onEdgeModelLine2IndexChanged(int)));
    connect(ui->cbAttributesSource, SIGNAL(currentIndexChanged(int)), SLOT(onAttributesSourceIndexChanged(int)));

    connect(ui->cbPolyline, SIGNAL(clicked(bool)), this, SLOT(onUsePolylineClicked(bool)));
    connect(ui->cbRemoveOriginals, SIGNAL(clicked(bool)), this, SLOT(onRemoveOriginalsClicked(bool)));
}

LC_LineJoinOptions::~LC_LineJoinOptions(){
    saveSettings();
    delete ui;
}

void LC_LineJoinOptions::clearAction(){
  action = nullptr;
}

void LC_LineJoinOptions::languageChange(){
    ui->retranslateUi(this);
}

void LC_LineJoinOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionModifyLineJoin *>(a);

    int line1EdgeMode;
    int line2EdgeMode;
    bool usePolyline;
    int  attributesSource;
    bool removeOriginals;

    if (update){
        line1EdgeMode = action->getLine1EdgeMode();
        line2EdgeMode = action->getLine2EdgeMode();
        usePolyline = action->isCreatePolyline();
        removeOriginals = action->isRemoveOriginalLines();
    }
    else{
        RS_SETTINGS->beginGroup("/Draw");
        usePolyline = RS_SETTINGS->readNumEntry("/LineJoinPolyline", 0) == 1;
        removeOriginals = RS_SETTINGS->readNumEntry("/LineJoinRemoveOriginals", 0) == 1;
        attributesSource= RS_SETTINGS->readNumEntry("/LineJoinAttributesSource", 0);
        line1EdgeMode = RS_SETTINGS->readNumEntry("/LineJoinLine1EdgeMode", 0);
        line2EdgeMode = RS_SETTINGS->readNumEntry("/LineJoinLine2EdgeMode", 0);
        RS_SETTINGS->endGroup();
    }

    setUsePolylineToActionAndView(usePolyline);
    setRemoveOriginalsToActionAndView(removeOriginals);
    setAttributesSourceToActionAndView(attributesSource);
    setEdgeModeLine1ToActionAndView(line1EdgeMode);
    setEdgeModeLine2ToActionAndView(line2EdgeMode);

}

void LC_LineJoinOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
//    RS_SETTINGS->writeEntry("/LineJoinWidth", ui->leWidth->text());
//
    RS_SETTINGS->writeEntry("/LineJoinPolyline", ui->cbPolyline->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/LineJoinRemoveOriginals", ui->cbRemoveOriginals->isChecked()  ? 1 : 0);
    RS_SETTINGS->writeEntry("/LineJoinAttributesSource", ui->cbAttributesSource->currentIndex());
    RS_SETTINGS->writeEntry("/LineJoinLine1EdgeMode", ui->cbLine1EdgeMode->currentIndex());
    RS_SETTINGS->writeEntry("/LineJoinLine2EdgeMode", ui->cbLine2EdgeMode->currentIndex());
    RS_SETTINGS->endGroup();
}

bool LC_LineJoinOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionModifyLineJoin;
}

void LC_LineJoinOptions::onUsePolylineClicked(bool value){
    if (action != nullptr){
        setUsePolylineToActionAndView(value);
    }

}
void LC_LineJoinOptions::onRemoveOriginalsClicked(bool value){
    if (action != nullptr){
        setRemoveOriginalsToActionAndView(value);
    }
}

#define NO_CHANGE_INDEX 2
#define EXTEND_TRIM_INDEX 0

void LC_LineJoinOptions::onEdgeModelLine1IndexChanged(int index){
    if (action != nullptr){
        setEdgeModeLine1ToActionAndView(index);
        if (index == NO_CHANGE_INDEX){
            ui->cbPolyline->setEnabled(false);
        }
        else{
            ui->cbPolyline->setEnabled(ui->cbLine2EdgeMode->currentIndex() != NO_CHANGE_INDEX);
        }
    }
    bool allowRemoval = index == EXTEND_TRIM_INDEX || ui->cbLine2EdgeMode->currentIndex() == EXTEND_TRIM_INDEX;
    ui->cbRemoveOriginals->setEnabled(allowRemoval);
}

void LC_LineJoinOptions::onEdgeModelLine2IndexChanged(int index){
    if (action != nullptr){
        setEdgeModeLine2ToActionAndView(index);
        if (index == NO_CHANGE_INDEX){
            ui->cbPolyline->setEnabled(false);
        }
        else{
            ui->cbPolyline->setEnabled(ui->cbLine1EdgeMode->currentIndex() != NO_CHANGE_INDEX);
        }
    }
    bool allowRemoval = index == EXTEND_TRIM_INDEX || ui->cbLine1EdgeMode->currentIndex() == EXTEND_TRIM_INDEX;
    ui->cbRemoveOriginals->setEnabled(allowRemoval);
}

void LC_LineJoinOptions::onAttributesSourceIndexChanged(int index){
    if (action != nullptr){
        setAttributesSourceToActionAndView(index);
    }
}

void LC_LineJoinOptions::setEdgeModeLine1ToActionAndView(int index){
    action->setLine1EdgeMode(index);
    ui->cbLine1EdgeMode->setCurrentIndex(index);
}

void LC_LineJoinOptions::setAttributesSourceToActionAndView(int index){
    action->setAttributesSource(index);
    ui->cbAttributesSource->setCurrentIndex(index);
}

void LC_LineJoinOptions::setEdgeModeLine2ToActionAndView(int index){
    action->setLine2EdgeMode(index);
    ui->cbLine2EdgeMode->setCurrentIndex(index);
}

void LC_LineJoinOptions::setUsePolylineToActionAndView(bool value){
    action->setCreatePolyline(value);
    ui->cbPolyline->setChecked(value);
}

void LC_LineJoinOptions::setRemoveOriginalsToActionAndView(bool value){
   action->setRemoveOriginalLines(value);
   ui->cbRemoveOriginals->setChecked(value);
}
