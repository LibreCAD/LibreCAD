#include "lc_optionswidgetsholder.h"
#include "ui_lc_optionswidgetsholder.h"
#include "rs_debug.h"

LC_OptionsWidgetsHolder::LC_OptionsWidgetsHolder(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_OptionsWidgetsHolder){
    ui->setupUi(this);
    ui->snapOptionsHolder->setLocatedOnLeft(true);
}

LC_OptionsWidgetsHolder::~LC_OptionsWidgetsHolder(){
    delete ui;
}

// fixme - lineOrt2 action - no trigger

LC_SnapOptionsWidgetsHolder *LC_OptionsWidgetsHolder::getSnapOptionsHolder() {
    return ui->snapOptionsHolder;
}

#define DEBUG_WIDGETS_COUNT_

void LC_OptionsWidgetsHolder::addOptionsWidget(QWidget *optionsWidget) {
    if (optionsWidget != nullptr) {
        ui->wOptionsWidgetsContainer->layout()->addWidget(optionsWidget);
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &list = ui->wOptionsWidgetsContainer->children();
        if (!list.isEmpty()) {
            int size = list.size();
            LC_ERR << "OPTION WIDGETS: " << size;
        }
#endif
    }
}

void LC_OptionsWidgetsHolder::removeOptionsWidget(QWidget *optionsWidget) {
    if (optionsWidget != nullptr) {
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &list = ui->wOptionsWidgetsContainer->children();
        LC_ERR << "OPTION WIDGETS BEFORE: " << list.size();
#endif

        ui->wOptionsWidgetsContainer->layout()->removeWidget(optionsWidget);
        optionsWidget->deleteLater();
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &listAfter = ui->wOptionsWidgetsContainer->children();
        LC_ERR << "OPTION WIDGETS AFTER: " << listAfter.size();
#endif
    }
}
