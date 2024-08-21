/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include<cmath>

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

#include "qg_printpreviewoptions.h"
#include "rs_actionprintpreview.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "ui_qg_printpreviewoptions.h"

namespace {
    const int MAX_CUSTOM_RATIOS = 5;
    const double MAX_PRINT_SCALE = 1e6;
    const double MIN_PRINT_SCALE = 1.0 / 1e6;
    const double PRINT_SCALE_STEP = MIN_PRINT_SCALE;

    const char* KEY_CUSTOM_SCALE_METRIC_TEMPLATE = "CustomScaleMe%1";
    const char* KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE = "CustomScaleIm1%1";
}

/*
 *  Constructs a QG_PrintPreviewOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PrintPreviewOptions::QG_PrintPreviewOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionFilePrintPreview, "PrintPreview", "")
    , defaultScalesStartIndex{0}
    , ui(new Ui::Ui_PrintPreviewOptions{})
{
    ui->setupUi(this);

    // Connect ui actions
    connect(ui->cFixed, &QCheckBox::clicked, this, &QG_PrintPreviewOptions::onScaleFixedClicked);
    connect(ui->bScaleLineWidth, &QPushButton::toggled, this, &QG_PrintPreviewOptions::onScaleLineClicked);
    connect(ui->bBlackWhite, &QPushButton::toggled, this, &QG_PrintPreviewOptions::onBlackWhiteClicked);
    connect(ui->cbScale->lineEdit(), &QLineEdit::editingFinished, [this](){
        ui->cbScale->blockSignals(true);
            scale(ui->cbScale->currentText());
        ui->cbScale->blockSignals(false);
    });
        connect(ui->cbScale, &QComboBox::currentIndexChanged, [this](int index){
        ui->cbScale->blockSignals(true);
        scale(ui->cbScale->itemText(index));
        ui->cbScale->blockSignals(false);
    });
    connect(ui->bFit, &QPushButton::clicked, this, &QG_PrintPreviewOptions::onFitClicked);
    connect(ui->bCenter, &QPushButton::clicked, this, &QG_PrintPreviewOptions::onCenterClicked);
    connect(ui->bCalcPagesNum, &QPushButton::clicked, this, &QG_PrintPreviewOptions::onCalcPagesNumClicked);
    connect(ui->bZoomPage, &QToolButton::clicked, this, &QG_PrintPreviewOptions::onZoomToPageClicked);
    connect(ui->cbTiledPrint, &QToolButton::clicked, this, &QG_PrintPreviewOptions::onTiledPrintClicked);

    connect(ui->tbSettings, &QToolButton::clicked, this, &QG_PrintPreviewOptions::onSettingsClicked);
    connect(ui->tbPortait, &QToolButton::clicked, this, &QG_PrintPreviewOptions::onPortraitClicked);
    connect(ui->tbLandscape, &QToolButton::clicked, this, &QG_PrintPreviewOptions::onLandscapeClicked);

    connect(ui->sbPagesVertical, &QSpinBox::valueChanged, this, &QG_PrintPreviewOptions::onVerticalPagesValueChanges);
    connect(ui->sbPagessHorizontal, &QSpinBox::valueChanged, this, &QG_PrintPreviewOptions::onHorizontalPagesValueChanges);

    ui->cbTiledPrint->setChecked(false);
    ui->wTiledPrint->setVisible(false);

    //make sure user scale is accepted
    ui->cbScale->setInsertPolicy(QComboBox::NoInsert);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PrintPreviewOptions::~QG_PrintPreviewOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PrintPreviewOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_PrintPreviewOptions::doSaveSettings() {
    save("PrintScaleFixed", ui->cFixed->isChecked());
    save("ScaleLineWidth", ui->bScaleLineWidth->isChecked());
    save("BlackWhiteSet", ui->bBlackWhite->isChecked());
    save("PrintScaleValue", ui->cbScale->currentText());
    saveCustomRatios();
}

void QG_PrintPreviewOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionPrintPreview*>(a);

    bool paperScaleFixed;
    bool blackAndWhiteMode;
    bool scaleLineWidth;

    if (update) {
        paperScaleFixed = action->isPaperScaleFixed();
        double printScaleFactor = action->getScale();
        scaleLineWidth = action->isLineWidthScaling();
        blackAndWhiteMode = action->isBlackWhite();

        setScaleFixedToUI(paperScaleFixed);
        setScaleLineToUI(scaleLineWidth);
        onBlackWhiteClicked(blackAndWhiteMode);
        updateScaleBox(printScaleFactor);

        updatePageNumbers();
    } else {
        paperScaleFixed = loadBool("PrintScaleFixed", false);
        scaleLineWidth = loadBool("ScaleLineWidth", false);
        blackAndWhiteMode = loadBool("BlackWhiteSet", false);
        QString scaleFactorStr = load("PrintScaleValue", "1:1");

        initializeScaleBoxItems();

        onScaleFixedClicked(paperScaleFixed);
        onScaleLineClicked(scaleLineWidth);
        onBlackWhiteClicked(blackAndWhiteMode);

        scale(scaleFactorStr, true);
    }
    setPaperOrientation(action->isPortrait());
}

QStringList QG_PrintPreviewOptions::readCustomRatios(bool metric) {
    QStringList ratios;
    const char *prefix = metric ? KEY_CUSTOM_SCALE_METRIC_TEMPLATE : KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE;
    for (unsigned i = 0; i < MAX_CUSTOM_RATIOS; ++i) {
        QString ratio = load(QString(prefix).arg(i), "");
        if (!ratio.isEmpty()) {
            ratios.push_back(ratio);
        }
    }
    return ratios;
}

void QG_PrintPreviewOptions::saveCustomRatios() {
    bool metric = !isUseImperialScales();
    int existingScalesCount = ui->cbScale->count();
    int max = defaultScalesStartIndex + MAX_CUSTOM_RATIOS;
    max = std::min(max, existingScalesCount);
    int propertyIndex = 0;
    for (int i = defaultScalesStartIndex; i<max;i++){
        const char *prefix = metric ? KEY_CUSTOM_SCALE_METRIC_TEMPLATE : KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE;
        QString ratio = ui->cbScale->itemText(i);
        save(QString(prefix).arg(propertyIndex), ratio);
        propertyIndex++;
    }
}

void QG_PrintPreviewOptions::initializeScaleBoxItems() {
    ui->cbScale->setDuplicatesEnabled(false);
    bool useImperialScales = isUseImperialScales();    
    if (useImperialScales){
        QStringList imperialScales = {"1:1","1:2","1:4","1:8","1:16","1:32","1:64","1:128","1:256",
                                      "2:1", "4:1", "16:1", "32:1", "64:1", "128:1", "256:1"};

        addScalesToCombobox(imperialScales);
        defaultScalesStartIndex = ui->cbScale->count();

        QStringList customScales = readCustomRatios(false);
        addScalesToCombobox(customScales);
    } else {
        QStringList metricScales = {
            "1:1","1:2","1:5","1:10","1:20","1:25","1:50","1:75","1:100","1:125","1:150","1:175","1:200","1:250",
            "1:500","1:750","1:1000","1:2500","1:5000","1:7500","1:10000","2:1","5:1","10:1","20:1","25:1","50:1",
            "75:1","100:1","125:1","150:1","175:1","200:1","250:1","500:1","750:1","1000:1"};

        addScalesToCombobox(metricScales);
        defaultScalesStartIndex = ui->cbScale->count();

        QStringList customScales = readCustomRatios(true);
        addScalesToCombobox(customScales);
    }
}

void QG_PrintPreviewOptions::addScalesToCombobox(QStringList &scales){
    for (const QString& scale: scales){
        if (scale != nullptr && !scale.isEmpty()){
            addScaleToScaleCombobox(scale);
        }
    }
}

bool QG_PrintPreviewOptions::addScaleToScaleCombobox(const QString& scaleString){
    bool parseOk;
    double factor = parseScaleString(scaleString, parseOk);
    if (parseOk){
        ui->cbScale->blockSignals(true);
        ui->cbScale->addItem(scaleString, QVariant(factor));
        ui->cbScale->blockSignals(false);
    }
    return parseOk;
}

void QG_PrintPreviewOptions::onCenterClicked() {
   action->center();
}

void QG_PrintPreviewOptions::onZoomToPageClicked() {
    action->zoomToPage();
}

void QG_PrintPreviewOptions::onTiledPrintClicked(){
    bool enabled = ui->cbTiledPrint->isChecked();
    ui->wTiledPrint->setVisible(enabled);
    if (!enabled){
        if (action != nullptr) {
            if (ui->cFixed) {
                action->calcPagesNum(false);
            } else {
                fitPage();
            }
        }
    }
}

void QG_PrintPreviewOptions::onSettingsClicked(){
    action->invokeSettingsDialog();
}

void QG_PrintPreviewOptions::onPortraitClicked(){
    bool portrait = ui->tbPortait->isChecked();
    setPaperOrientation(portrait);
    action->setPaperOrientation(portrait);
}

void QG_PrintPreviewOptions::onLandscapeClicked(){
    bool portrait = !ui->tbLandscape->isChecked();
    setPaperOrientation(portrait);
    action->setPaperOrientation(portrait);
}

void QG_PrintPreviewOptions::setPaperOrientation(bool isPortait){
     ui->tbLandscape->blockSignals(true);
     ui->tbPortait->blockSignals(true);
     ui->tbPortait->setChecked(isPortait);
     ui->tbLandscape->setChecked(!isPortait);
    ui->tbLandscape->blockSignals(false);
    ui->tbPortait->blockSignals(false);
}

void QG_PrintPreviewOptions::onVerticalPagesValueChanges(int value) {
    action->setPagesNumVertical(value);
}

void QG_PrintPreviewOptions::onHorizontalPagesValueChanges(int value) {
    action->setPagesNumHorizontal(value);
}

void QG_PrintPreviewOptions::onScaleLineClicked(bool state) {
    setScaleLineToUI(state);
    action->setLineWidthScaling(state);
}

void QG_PrintPreviewOptions::setScaleLineToUI(bool state) {
    if(ui->bScaleLineWidth->isChecked() != state) {
        ui->bScaleLineWidth->setChecked(state);
    }
}

void QG_PrintPreviewOptions::onBlackWhiteClicked(bool on) {
    if(ui->bBlackWhite->isChecked() != on) {
        ui->bBlackWhite->setChecked(on);
    }
    action->setBlackWhite(on);
}

void QG_PrintPreviewOptions::onFitClicked() {
    if (!ui->cFixed->isChecked()) {
        fitPage();
    }
}

void QG_PrintPreviewOptions::fitPage() {
    action->fit();
    updateScaleBox(action->getScale());
}

/** print scale fixed to saved value **/
void QG_PrintPreviewOptions::onScaleFixedClicked(bool fixed){
    action->setPaperScaleFixed(fixed);
    setScaleFixedToUI(fixed);
}

void QG_PrintPreviewOptions::setScaleFixedToUI(bool fixed) {
    ui->cbScale->setDisabled(fixed);
    ui->bFit->setVisible(!fixed);
    if(ui->cFixed->isChecked() != fixed) {
        ui->cFixed->setChecked(fixed);
    }
    ui->sbPagesVertical->setEnabled(!fixed);
    ui->sbPagessHorizontal->setEnabled(!fixed);
}

void QG_PrintPreviewOptions::onCalcPagesNumClicked() {
    if (action != nullptr) {
        action->calcPagesNum(true);
    }
}

void QG_PrintPreviewOptions::scale(const QString &newScale, bool force) {
    QString scaleToUse;
    if (force){
        scaleToUse = newScale;
    }
    else {
        if (ui->cFixed->isChecked()) {
            scaleToUse = ui->cbScale->currentText();
        } else {
            scaleToUse = newScale;
        }
    }
    bool parseOk;
    double factor = parseScaleString(scaleToUse, parseOk);
    if (action != nullptr) {
        if (!parseOk) {
            action->printWarning(tr("Invalid scale provided"));
        } else {
            if (std::abs(factor - action->getScale()) > PRINT_SCALE_STEP) {
                if (action->setScale(factor, true)) {
                    updateScaleBox(factor);
                }
            }
            else{
                updateScaleBox(factor);
            }
        }
    }
}

double QG_PrintPreviewOptions::parseScaleString(const QString &scaleText, bool &parseOk) const {
    double factor = 1.0;
    parseOk = false;
    int colonIndex = scaleText.indexOf(':');
    if (colonIndex > 0) {// ratio like '1:2'
        bool ok1 = false;
        bool ok2 = false;

        double numerator = RS_Math::eval(scaleText.left(colonIndex), &ok1);
        double denominator = RS_Math::eval(scaleText.mid(colonIndex + 1), &ok2);

        if (ok1 && ok2 && denominator > 1.0e-6) {
            factor = numerator / denominator;
            parseOk = true;
        }
    }
    else{
        // fixme - this code is for direct values only
        // FIXME _ IMPERIAL UNITS ARE NOT SUPPORTED :(
        // Fixme - support imperial units
        // direct ratio like '=0.5'
        int equalSignIndex = scaleText.indexOf('=');
        if (equalSignIndex > 0){
            bool ok = false;
            double denominator = RS_Math::eval(scaleText.mid(equalSignIndex + 1), &ok);
            if (ok && denominator > 1.0e-6) {
                factor = 1.0 / denominator;
                parseOk = true;
            }
        }
        else{
            bool ok = false;
            double f = RS_Math::eval(scaleText, &ok);
            if (ok) {
                factor = f;
                parseOk = true;
            }
        }
    }

    factor = std::abs(factor); // do we need negative factor at all?
    if (factor > MAX_PRINT_SCALE){
        action->printWarning(tr("Paper scale factor larger than max print ratio"));
        factor = MAX_PRINT_SCALE;
    }
    else if (factor < MIN_PRINT_SCALE){
        action->printWarning(tr("Paper scale factor smaller than min print ratio"));
        factor = MIN_PRINT_SCALE;
    }
    return factor;
}

void QG_PrintPreviewOptions::updateScaleBox(double factor){
    //    std::cout<<"void QG_PrintPreviewOptions::updateScaleBox() f="<<f<<std::endl;
    int scaleIndex;
    int existingScalesCount = ui->cbScale->count();
    for (scaleIndex = 0; scaleIndex < existingScalesCount; scaleIndex++) {
        QVariant itemData = ui->cbScale->itemData(scaleIndex);
        if (itemData.isValid() && !itemData.isNull()) {
            double itemFactor = itemData.toDouble();
            if (std::abs(factor - itemFactor) < PRINT_SCALE_STEP) break;
        }
        else{
            LC_ERR << "Item with no data: " << scaleIndex << ui->cbScale->itemText(scaleIndex);
        }
    }

    if (scaleIndex < existingScalesCount) { // item found with the same factor
        ui->cbScale->blockSignals(true);
        ui->cbScale->setCurrentIndex(scaleIndex);
        ui->cbScale->blockSignals(false);
    }
    else { // new item should be added
        QString newScaleText("");
        if (factor > 1.) {
            newScaleText = QString("%1:1").arg(factor);
        } else {
            newScaleText = QString("1:%1").arg(1. / factor);
        }

        if (existingScalesCount > defaultScalesStartIndex) {
            scaleIndex = defaultScalesStartIndex;
            ui->cbScale->insertItem(defaultScalesStartIndex, newScaleText, QVariant(factor));
        } else {
            ui->cbScale->addItem(newScaleText, QVariant(factor));
            scaleIndex = existingScalesCount;
        }
        ui->cbScale->blockSignals(true);
        ui->cbScale->setCurrentIndex(scaleIndex);
        ui->cbScale->blockSignals(false);

        int maxItems = defaultScalesStartIndex + MAX_CUSTOM_RATIOS;
        existingScalesCount = ui->cbScale->count();
        if (existingScalesCount > maxItems){
            for (int i = existingScalesCount; i>=maxItems;i--){
                ui->cbScale->removeItem(i);
            }
        }
        saveSettings();
    }
}

bool QG_PrintPreviewOptions::isUseImperialScales() {
    RS2::Unit u = action->getUnit();
    bool result = u == RS2::Inch || u == RS2::Foot || u == RS2::Microinch || u ==  RS2::Mil || u == RS2::Yard;
    return result;
}

void QG_PrintPreviewOptions::updateUI(int mode) {
    switch (mode){
        case MODE_UPDATE_ORIENTATION: {
            setPaperOrientation(action->isPortrait());
            break;
        }
        case MODE_UPDATE_PAGE_NUMBERS:{
            updatePageNumbers();
            break;
        }
        default:
            break;
    }
}

void QG_PrintPreviewOptions::updatePageNumbers() {
    ui->sbPagessHorizontal->blockSignals(true);
    ui->sbPagesVertical->blockSignals(true);
    ui->sbPagessHorizontal->setValue(action->getPagesNumHorizontal());
    ui->sbPagesVertical->setValue(action->getPagesNumVertical());
    ui->sbPagessHorizontal->blockSignals(false);
    ui->sbPagesVertical->blockSignals(false);
}
