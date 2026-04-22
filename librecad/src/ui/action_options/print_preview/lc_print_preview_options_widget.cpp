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
#include "lc_print_preview_options_widget.h"

#include <QLineEdit>
#include <QPushButton>
#include<cmath>

#include "lc_guarded_signals_blocker.h"
#include "rs_actionprintpreview.h"
#include "rs_debug.h"
#include "ui_lc_print_preview_options_widget.h"

namespace {
    constexpr int MAX_CUSTOM_RATIOS = 5;
    constexpr double MAX_PRINT_SCALE = 1e6;
    constexpr double MIN_PRINT_SCALE = 1.0 / 1e6;
    constexpr double PRINT_SCALE_STEP = MIN_PRINT_SCALE;

    constexpr auto KEY_CUSTOM_SCALE_METRIC_TEMPLATE = "CustomScaleMe%1";
    constexpr auto KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE = "CustomScaleIm1%1";
}

/*
 *  Constructs a LC_PrintPreviewOptionsWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_PrintPreviewOptionsWidget::LC_PrintPreviewOptionsWidget():ui(new Ui::LC_PrintPreviewOptionsWidget{}) {
    ui->setupUi(this);

    // Connect ui actions
    connect(ui->cFixed, &QCheckBox::clicked, this, &LC_PrintPreviewOptionsWidget::onScaleFixedClicked);
    connect(ui->bScaleLineWidth, &QPushButton::toggled, this, &LC_PrintPreviewOptionsWidget::onScaleLineClicked);
    connect(ui->bBlackWhite, &QPushButton::toggled, this, &LC_PrintPreviewOptionsWidget::onBlackWhiteClicked);
    /*connect(ui->cbScale->lineEdit(), &QLineEdit::editingFinished, [this] {
        ui->cbScale->blockSignals(true);
        scale(ui->cbScale->currentText());
        ui->cbScale->blockSignals(false);
    });*/
    /*connect(ui->cbScale, &QComboBox::currentIndexChanged, [this](const int index) {
        ui->cbScale->blockSignals(true);
        scale(ui->cbScale->itemText(index));
        ui->cbScale->blockSignals(false);
    });*/

    connect(ui->cbScale, &QComboBox::currentIndexChanged, this, &LC_PrintPreviewOptionsWidget::onScaleIndexChanged);
    connect(ui->bFit, &QPushButton::clicked, this, &LC_PrintPreviewOptionsWidget::onFitClicked);
    connect(ui->bCenter, &QPushButton::clicked, this, &LC_PrintPreviewOptionsWidget::onCenterClicked);
    connect(ui->bCalcPagesNum, &QPushButton::clicked, this, &LC_PrintPreviewOptionsWidget::onCalcPagesNumClicked);
    connect(ui->bZoomPage, &QToolButton::clicked, this, &LC_PrintPreviewOptionsWidget::onZoomToPageClicked);
    // connect(ui->cbTiledPrint, &QToolButton::clicked, this, &LC_PrintPreviewOptionsWidget::onTiledPrintClicked);

    connect(ui->tbSettings, &QToolButton::clicked, this, &LC_PrintPreviewOptionsWidget::onSettingsClicked);
    connect(ui->tbPortait, &QToolButton::clicked, this, &LC_PrintPreviewOptionsWidget::onPortraitClicked);
    connect(ui->tbLandscape, &QToolButton::clicked, this, &LC_PrintPreviewOptionsWidget::onLandscapeClicked);

    connect(ui->sbPagesVertical, &QSpinBox::valueChanged, this, &LC_PrintPreviewOptionsWidget::onVerticalPagesValueChanges);
    connect(ui->sbPagessHorizontal, &QSpinBox::valueChanged, this, &LC_PrintPreviewOptionsWidget::onHorizontalPagesValueChanges);

    // ui->cbTiledPrint->setChecked(false);
    ui->wTiledPrint->setVisible(true);

    //make sure user scale is accepted
    ui->cbScale->setInsertPolicy(QComboBox::NoInsert);

    ui->leDrawingUnits->setEnabled(false);
    ui->lePrintedUnits->setEnabled(false);
}

void LC_PrintPreviewOptionsWidget::onScaleIndexChanged(int index) {
    LC_GuardedSignalsBlocker({ui->cbScale, ui->leDrawingUnits, ui->lePrintedUnits});
    const bool customScale = index == 0;
    ui->leDrawingUnits->setEnabled(customScale);
    ui->lePrintedUnits->setEnabled(customScale);

    if (customScale) {
    }
    else{
        const QString scaleText = ui->cbScale->currentText();
        const int colonIndex = scaleText.indexOf(':');

        bool ok1 = false;
        bool ok2 = false;

        const auto printed = scaleText.left(colonIndex);
        const auto drawing = scaleText.mid(colonIndex + 1);

        ui->lePrintedUnits->setText(printed);
        ui->leDrawingUnits->setText(drawing);

        const double numerator = RS_Math::eval(printed, &ok1);
        const double denominator = RS_Math::eval(drawing, &ok2);

        if (ok1 && ok2 && denominator > 1.0e-6) {
            const double factor = numerator / denominator;
            if (std::abs(factor - m_action->getScale()) > PRINT_SCALE_STEP) {
                if (m_action->setScale(factor, true)) {
                    updateScaleBox(factor);
                }
            }
            else {
                updateScaleBox(factor);
            }
        }
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_PrintPreviewOptionsWidget::~LC_PrintPreviewOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_PrintPreviewOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_PrintPreviewOptionsWidget::doSaveSettings() {
    /*save("PrintScaleFixed", ui->cFixed->isChecked());
    save("ScaleLineWidth", ui->bScaleLineWidth->isChecked());
    save("BlackWhiteSet", ui->bBlackWhite->isChecked());
    save("PrintScaleValue", ui->cbScale->currentText());*/
    saveCustomRatios();
}

void LC_PrintPreviewOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<RS_ActionPrintPreview*>(a);

    LC_GuardedSignalsBlocker({
        ui->cbScale,
        ui->leDrawingUnits,
        ui->lePrintedUnits,
        ui->cFixed
    });

    bool paperScaleFixed = false;
    bool blackAndWhiteMode = false;
    bool scaleLineWidth = false;

    initializeScaleBoxItems();

    const bool update = true;
    if (update) {
        paperScaleFixed = m_action->isPaperScaleFixed();
        const double printScaleFactor = m_action->getScale();
        scaleLineWidth = m_action->isLineWidthScaling();
        blackAndWhiteMode = m_action->isBlackWhite();

        setScaleFixedToUI(paperScaleFixed);
        setScaleLineToUI(scaleLineWidth);
        onBlackWhiteClicked(blackAndWhiteMode);
        updateScaleBox(printScaleFactor);

        updatePageNumbers();
    } /* else {

        // fixme - sand - files - review this. Switch between previews should retain different print scales in different documents.
        paperScaleFixed = loadBool("PrintScaleFixed", false);
        scaleLineWidth = loadBool("ScaleLineWidth", false);
        blackAndWhiteMode = loadBool("BlackWhiteSet", false);
        const QString scaleFactorStr = load("PrintScaleValue", "1:1");

        initializeScaleBoxItems();

        onScaleFixedClicked(paperScaleFixed);
        onScaleLineClicked(scaleLineWidth);
        onBlackWhiteClicked(blackAndWhiteMode);

        scale(scaleFactorStr, true);
    }*/
    setPaperOrientation(m_action->isPortrait());
}

QStringList LC_PrintPreviewOptionsWidget::readCustomRatios([[maybe_unused]]const bool metric) {
    QStringList ratios;
    // const char* prefix = metric ? KEY_CUSTOM_SCALE_METRIC_TEMPLATE : KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE;
    // for (unsigned i = 0; i < MAX_CUSTOM_RATIOS; ++i) {
    //     QString ratio = load(QString(prefix).arg(i), "");
    //     if (!ratio.isEmpty()) {
    //         ratios.push_back(ratio);
    //     }
    // }
    return ratios;
}

void LC_PrintPreviewOptionsWidget::saveCustomRatios() {
    /*const bool metric = !m_action->isUseImperialScales();
    const int existingScalesCount = ui->cbScale->count();
    int max = m_defaultScalesStartIndex + MAX_CUSTOM_RATIOS;
    max = std::min(max, existingScalesCount);
    int propertyIndex = 0;
    for (int i = m_defaultScalesStartIndex; i < max; i++) {
        const char* prefix = metric ? KEY_CUSTOM_SCALE_METRIC_TEMPLATE : KEY_CUSTOM_SCALE_IMPERIAL_TEMPLATE;
        const QString ratio = ui->cbScale->itemText(i);
        save(QString(prefix).arg(propertyIndex), ratio);
        propertyIndex++;
    }*/
}

void LC_PrintPreviewOptionsWidget::initializeScaleBoxItems() {
    ui->cbScale->setDuplicatesEnabled(false);
    ui->cbScale->clear();
    ui->cbScale->addItem(tr("Custom"));
    QStringList scales = m_action->getStandardPrintScales();
    addScalesToCombobox(scales);
    m_defaultScalesStartIndex = ui->cbScale->count();

    QStringList customScales = readCustomRatios(false);
    addScalesToCombobox(customScales);
}

void LC_PrintPreviewOptionsWidget::addScalesToCombobox(QStringList& scales) {
    for (const QString& scale : scales) {
        if (scale != nullptr && !scale.isEmpty()) {
            addScaleToScaleCombobox(scale);
        }
    }
}

bool LC_PrintPreviewOptionsWidget::addScaleToScaleCombobox(const QString& scaleString) const {
    bool parseOk = false;
    const double factor = parseScaleString(scaleString, parseOk);
    if (parseOk) {
        ui->cbScale->addItem(scaleString, QVariant(factor));
    }
    return parseOk;
}

void LC_PrintPreviewOptionsWidget::onCenterClicked() const {
    m_action->center();
}

void LC_PrintPreviewOptionsWidget::onZoomToPageClicked() const {
    m_action->zoomToPage();
}

void LC_PrintPreviewOptionsWidget::onTiledPrintClicked() {
    // const bool enabled = ui->cbTiledPrint->isChecked();
    // ui->wTiledPrint->setVisible(enabled);
    /*if (!enabled){
        if (m_action != nullptr) {
            if (ui->cFixed) {
                m_action->calcPagesNum(false);
            } else {
                fitPage();
            }
        }
    }*/
}

void LC_PrintPreviewOptionsWidget::onSettingsClicked() const {
    m_action->invokeSettingsDialog();
}

void LC_PrintPreviewOptionsWidget::onPortraitClicked() {
    const bool portrait = ui->tbPortait->isChecked();
    setPaperOrientation(portrait);
    m_action->setPaperOrientation(portrait);
}

void LC_PrintPreviewOptionsWidget::onLandscapeClicked() {
    const bool portrait = !ui->tbLandscape->isChecked();
    setPaperOrientation(portrait);
    m_action->setPaperOrientation(portrait);
}

void LC_PrintPreviewOptionsWidget::setPaperOrientation(const bool isPortait) const {
    ui->tbLandscape->blockSignals(true);
    ui->tbPortait->blockSignals(true);
    ui->tbPortait->setChecked(isPortait);
    ui->tbLandscape->setChecked(!isPortait);
    ui->tbLandscape->blockSignals(false);
    ui->tbPortait->blockSignals(false);
}

void LC_PrintPreviewOptionsWidget::onVerticalPagesValueChanges(const int value) const {
    m_action->setPagesNumVertical(value);
}

void LC_PrintPreviewOptionsWidget::onHorizontalPagesValueChanges(const int value) const {
    m_action->setPagesNumHorizontal(value);
}

void LC_PrintPreviewOptionsWidget::onScaleLineClicked(const bool state) {
    setScaleLineToUI(state);
    m_action->setLineWidthScaling(state);
}

void LC_PrintPreviewOptionsWidget::setScaleLineToUI(const bool state) const {
    if (ui->bScaleLineWidth->isChecked() != state) {
        ui->bScaleLineWidth->setChecked(state);
    }
}

void LC_PrintPreviewOptionsWidget::onBlackWhiteClicked(const bool on) const {
    if (ui->bBlackWhite->isChecked() != on) {
        ui->bBlackWhite->setChecked(on);
    }
    m_action->setBlackWhite(on);
}

void LC_PrintPreviewOptionsWidget::onFitClicked() {
    if (!ui->cFixed->isChecked()) {
        fitPage();
    }
}

void LC_PrintPreviewOptionsWidget::fitPage() {
    m_action->fit();
    updateScaleBox(m_action->getScale());
}

/** print scale fixed to saved value **/
void LC_PrintPreviewOptionsWidget::onScaleFixedClicked(const bool fixed) {
    m_action->setPaperScaleFixed(fixed);
    setScaleFixedToUI(fixed);
}

void LC_PrintPreviewOptionsWidget::setScaleFixedToUI(const bool fixed) const {
    ui->cbScale->setDisabled(fixed);
    ui->bFit->setVisible(!fixed);
    if (ui->cFixed->isChecked() != fixed) {
        ui->cFixed->setChecked(fixed);
    }
    ui->sbPagesVertical->setEnabled(!fixed);
    ui->sbPagessHorizontal->setEnabled(!fixed);
}

void LC_PrintPreviewOptionsWidget::onCalcPagesNumClicked() const {
    if (m_action != nullptr) {
        m_action->calcPagesNum(true);
    }
}

void LC_PrintPreviewOptionsWidget::scale(const QString& newScale, const bool force) {
    QString scaleToUse;
    if (force) {
        scaleToUse = newScale;
    }
    else {
        if (ui->cFixed->isChecked()) {
            scaleToUse = ui->cbScale->currentText();
        }
        else {
            scaleToUse = newScale;
        }
    }
    bool parseOk = false;
    const double factor = parseScaleString(scaleToUse, parseOk);
    if (m_action != nullptr) {
        if (!parseOk) {
            m_action->printWarning(tr("Invalid scale provided"));
        }
        else {
            if (std::abs(factor - m_action->getScale()) > PRINT_SCALE_STEP) {
                if (m_action->setScale(factor, true)) {
                    updateScaleBox(factor);
                }
            }
            else {
                updateScaleBox(factor);
            }
        }
    }
}

double LC_PrintPreviewOptionsWidget::parseScaleString(const QString& scaleText, bool& parseOk) const {
    double factor = 1.0;
    parseOk = false;
    const int colonIndex = scaleText.indexOf(':');
    if (colonIndex > 0) {
        // ratio like '1:2'
        bool ok1 = false;
        bool ok2 = false;

        const double numerator = RS_Math::eval(scaleText.left(colonIndex), &ok1);
        const double denominator = RS_Math::eval(scaleText.mid(colonIndex + 1), &ok2);

        if (ok1 && ok2 && denominator > 1.0e-6) {
            factor = numerator / denominator;
            parseOk = true;
        }
    }
    else {
        // fixme - this code is for direct values only
        // FIXME _ IMPERIAL UNITS ARE NOT SUPPORTED :(
        // Fixme - support imperial units
        // direct ratio like '=0.5'
        const int equalSignIndex = scaleText.indexOf('=');
        if (equalSignIndex > 0) {
            bool ok = false;
            const double denominator = RS_Math::eval(scaleText.mid(equalSignIndex + 1), &ok);
            if (ok && denominator > 1.0e-6) {
                factor = 1.0 / denominator;
                parseOk = true;
            }
        }
        else {
            bool ok = false;
            const double f = RS_Math::eval(scaleText, &ok);
            if (ok) {
                factor = f;
                parseOk = true;
            }
        }
    }

    factor = std::abs(factor); // do we need negative factor at all?
    if (factor > MAX_PRINT_SCALE) {
        m_action->printWarning(tr("Paper scale factor larger than max print ratio"));
        factor = MAX_PRINT_SCALE;
    }
    else if (factor < MIN_PRINT_SCALE) {
        m_action->printWarning(tr("Paper scale factor smaller than min print ratio"));
        factor = MIN_PRINT_SCALE;
    }
    return factor;
}

void LC_PrintPreviewOptionsWidget::updateScaleBox(const double factor) {
    //    std::cout<<"void LC_PrintPreviewOptionsWidget::updateScaleBox() f="<<f<<std::endl;
    int scaleIndex = 0;
    int existingScalesCount = ui->cbScale->count();
    for (scaleIndex = 0; scaleIndex < existingScalesCount; scaleIndex++) {
        QVariant itemData = ui->cbScale->itemData(scaleIndex);
        if (itemData.isValid() && !itemData.isNull()) {
            const double itemFactor = itemData.toDouble();
            if (std::abs(factor - itemFactor) < PRINT_SCALE_STEP) {
                break;
            }
        }
        else {
            LC_ERR << "Item with no data: " << scaleIndex << ui->cbScale->itemText(scaleIndex);
        }
    }

    if (scaleIndex < existingScalesCount) {
        // item found with the same factor
        ui->cbScale->blockSignals(true);
        ui->cbScale->setCurrentIndex(scaleIndex);
        ui->cbScale->blockSignals(false);
    }
    else {
        // new item should be added
        QString newScaleText("");
        if (factor > 1.) {
            newScaleText = QString("%1:1").arg(factor);
        }
        else {
            newScaleText = QString("1:%1").arg(1. / factor);
        }

        if (existingScalesCount > m_defaultScalesStartIndex) {
            scaleIndex = m_defaultScalesStartIndex;
            ui->cbScale->insertItem(m_defaultScalesStartIndex, newScaleText, QVariant(factor));
        }
        else {
            ui->cbScale->addItem(newScaleText, QVariant(factor));
            scaleIndex = existingScalesCount;
        }
        ui->cbScale->blockSignals(true);
        ui->cbScale->setCurrentIndex(scaleIndex);
        ui->cbScale->blockSignals(false);

        const int maxItems = m_defaultScalesStartIndex + MAX_CUSTOM_RATIOS;
        existingScalesCount = ui->cbScale->count();
        if (existingScalesCount > maxItems) {
            for (int i = existingScalesCount; i >= maxItems; i--) {
                ui->cbScale->removeItem(i);
            }
        }
        // saveSettings();
    }
}

void LC_PrintPreviewOptionsWidget::updateUI(const int mode, [[maybe_unused]]const QVariant* value) {
    switch (mode) {
        case MODE_UPDATE_ORIENTATION: {
            setPaperOrientation(m_action->isPortrait());
            break;
        }
        case MODE_UPDATE_PAGE_NUMBERS: {
            updatePageNumbers();
            break;
        }
        default:
            break;
    }
}

void LC_PrintPreviewOptionsWidget::updatePageNumbers() const {
    ui->sbPagessHorizontal->blockSignals(true);
    ui->sbPagesVertical->blockSignals(true);
    ui->sbPagessHorizontal->setValue(m_action->getPagesNumHorizontal());
    ui->sbPagesVertical->setValue(m_action->getPagesNumVertical());
    ui->sbPagessHorizontal->blockSignals(false);
    ui->sbPagesVertical->blockSignals(false);
}
