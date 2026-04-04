#include "lc_relative_position_editing_widget.h"

#include <QKeyEvent>

#include "lc_actioncontext.h"
#include "lc_actioncontext.h"
#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_math.h"
#include "ui_lc_relative_position_editing_widget.h"
#include "lc_relative_point_input_widget.h"
#include "rs_previewactioninterface.h"
#include "rs_settings.h"

namespace {
    void activateParamUI(QStackedWidget* stacked, QLineEdit* edit) {
        stacked->setCurrentIndex(0);
        edit->selectAll();
        edit->setFocus();
    }

    void setupIconLabel(const char* iconName, QLabel* label) {
        const int iconSize = 22;
        const auto iconLength = QIcon(iconName);
        const auto pixmapLength = iconLength.pixmap(iconSize, iconSize);
        label->setPixmap(pixmapLength);
    }
}

LC_RelativePositionEditingWidget::LC_RelativePositionEditingWidget(LC_RelativePointInputWidget* parent, LC_GraphicViewport* viewport,
                                                                   LC_ActionContext* actionContext,
                                                                   LC_LateCompletionRequestor* lateCompletionRequestor) : QWidget(parent),
    ui(new Ui::LC_RelativePositionEditingWidget), m_viewport{viewport}, m_actionContext{actionContext},
    m_lateCompletionRequestor{lateCompletionRequestor} {
    m_formatter = m_viewport->getFormatter();
    m_relativePositionEvaluator.setViewport(viewport);
    ui->setupUi(this);

    setupLabelAndEditor(ui->lblValueLength, ui->leValueEditLength, RS2::RelativePointParam::REL_POINT_LENGTH);
    setupLabelAndEditor(ui->lblValueAngle, ui->leValueEditAngle, RS2::RelativePointParam::REL_POINT_ANGLE);
    setupLabelAndEditor(ui->lblValueDX, ui->leValueEditDX, RS2::RelativePointParam::REL_POINT_DX);
    setupLabelAndEditor(ui->lblValueDY, ui->leValueEditDY, RS2::RelativePointParam::REL_POINT_DY);
    setupLabelAndEditor(ui->lblValueX, ui->leValueEditX, RS2::RelativePointParam::REL_POINT_X);
    setupLabelAndEditor(ui->lblValueY, ui->leValueEditY, RS2::RelativePointParam::REL_POINT_Y);

    auto editors = findChildren<QLineEdit*>();
    for (const auto e : editors) {
        e->installEventFilter(this);
    }

    setupButtons(ui->pbOKLength, ui->tbSelectLength, ui->tbManualSnapLength, RS2::RelativePointParam::REL_POINT_LENGTH,
                 LC_ActionContext::InteractiveInputInfo::InputType::DISTANCE);
    setupButtons(ui->pbOKAngle, ui->tbSelectAngle, ui->tbManualSnapAngle, RS2::RelativePointParam::REL_POINT_ANGLE,
                 LC_ActionContext::InteractiveInputInfo::InputType::ANGLE);
    setupButtons(ui->pbOKDX, ui->tbSelectDX, ui->tbManualSnapDX, RS2::RelativePointParam::REL_POINT_DX,
                 LC_ActionContext::InteractiveInputInfo::InputType::DISTANCE);
    setupButtons(ui->pbOKDY, ui->tbSelectDY, ui->tbManualSnapDY, RS2::RelativePointParam::REL_POINT_DY,
                 LC_ActionContext::InteractiveInputInfo::InputType::DISTANCE);
    setupButtons(ui->pbOKX, ui->tbSelectX, ui->tbManualSnapX, RS2::RelativePointParam::REL_POINT_X,
                 LC_ActionContext::InteractiveInputInfo::InputType::POINT_X);
    setupButtons(ui->pbOKY, ui->tbSelectY, ui->tbManualSnapY, RS2::RelativePointParam::REL_POINT_Y,
                 LC_ActionContext::InteractiveInputInfo::InputType::POINT_Y);

    connect(ui->cbByOffset, &QCheckBox::toggled, this, &LC_RelativePositionEditingWidget::onByOffsetToggled);

    ui->cbByOffset->setChecked(true);

    const QString tooltip = "<p>"+tr("Relative point assistant. Use keys to activate:")+"</p><p><span style='font-weight:700;'>D</span> - "+tr("Distance")+
                "<br/><span style='font-weight:700;'>A</span> - "+tr("Angle") +
                "<br/><span style='font-weight:700;'>X</span> - "+tr("Offset/Abs X")+
                "<br/><span style='font-weight:700;'>S</span> - "+tr("Offset/Abs Y")+
                "<br/><span style='font-weight:700;'>T</span> - " + tr("Toggle delta/absolute mode")+
                "<br/><span style='font-weight:700;'>E</span> - "+tr("Pick input value from drawing")+
                "<br/><span style='font-weight:700;'>Q</span> - "+tr("Select manually with entered parameters")+
                "<br/><span style='font-weight:700;'>F, ENTER</span> - "+tr("Confirm edit and try to apply values")+
                "<br/><span style='font-weight:700;'>ESC</span> - "+tr("Cancel assistant")+"</p>";
    setToolTip(tooltip);

    setupIconLabel(":/icons/relative_len.lci", ui->lblIconLength);
    setupIconLabel(":/icons/relative_angle.lci", ui->lblIconAngle);
    setupIconLabel(":/icons/relative_dx.lci", ui->lblIconDX);
    setupIconLabel(":/icons/relative_dy.lci", ui->lblIconDY);
    setupIconLabel(":/icons/relative_x.lci", ui->lblIconX);
    setupIconLabel(":/icons/relative_y.lci", ui->lblIconY);
}

LC_RelativePositionEditingWidget::~LC_RelativePositionEditingWidget() {
    delete ui;
}

void LC_RelativePositionEditingWidget::setupButtons(QToolButton* btnOk, QToolButton* btnInteractivePick, QToolButton* btnManualSnap, const RS2::RelativePointParam relativePointParam, const LC_ActionContext::InteractiveInputInfo::InputType inputType) {
    const QVariant type(relativePointParam);
    btnOk->setProperty("_propType", type);
    btnInteractivePick->setProperty("_propType", type);
    btnManualSnap->setProperty("_propType", type);

    connect(btnOk, &QToolButton::clicked, this, &LC_RelativePositionEditingWidget::onOKButtonClicked);
    connect(btnManualSnap, &QToolButton::clicked, this, &LC_RelativePositionEditingWidget::onManualSnapButtonClicked);

    connectInteractiveInputButton(btnInteractivePick, inputType, relativePointParam);
}

void LC_RelativePositionEditingWidget::connectInteractiveInputButton(QToolButton* button,
                                                                     const LC_ActionContext::InteractiveInputInfo::InputType inputType, const RS2::RelativePointParam relativePointParam) {
    button->setVisible(true);
    button->setProperty("_interactiveInputButton", inputType);
    button->setProperty("_interactiveInputTag", QString::number(relativePointParam));
    button->connect(button, &QToolButton::clicked, this, &LC_RelativePositionEditingWidget::onInteractiveInputButtonClicked);
    button->setAutoRaise(true);
}

void LC_RelativePositionEditingWidget::applyInput(bool applyProjected) {
    updateByEditedValue();
    hideAssistant();
    const auto currentAction = dynamic_cast<RS_PreviewActionInterface*>(m_actionContext->getCurrentAction());
    if (currentAction != nullptr) {
        currentAction->addProjectedRelativePointToVisualSnap(m_relativePositionEvaluator.getRelativePositionData(), applyProjected);
    }
}

void LC_RelativePositionEditingWidget::setupLabelAndEditor(QLabel* label, QLineEdit* lineEdit, const RS2::RelativePointParam relativePointParam) {
    label->setFocusPolicy(Qt::StrongFocus);
    label->setCursor(Qt::PointingHandCursor);
    label->setProperty("_propType", QVariant(relativePointParam));
    lineEdit->setProperty("_propType", QVariant(relativePointParam));

    label->installEventFilter(this);

    connect(lineEdit, &QLineEdit::returnPressed, this, &LC_RelativePositionEditingWidget::onEditingReturnPressed);
}

void LC_RelativePositionEditingWidget::onEditingReturnPressed() {
    onOKButtonClicked(true);
}

void LC_RelativePositionEditingWidget::onOKButtonClicked(bool checked) {
    applyInput(true);
}

void LC_RelativePositionEditingWidget::onInteractiveInputButtonClicked(bool checked) {
    const auto senderButton = dynamic_cast<QToolButton*>(sender());
    if (senderButton != nullptr) {
        const auto property = senderButton->property("_interactiveInputButton");
        if (property.isValid()) {
            const auto inputType = static_cast<LC_ActionContext::InteractiveInputInfo::InputType>(property.toInt());
            const auto tagProperty = senderButton->property("_interactiveInputTag");
            const QString tag = tagProperty.toString();
            m_actionContext->interactiveInputStart(inputType, m_lateCompletionRequestor, tag);
        }
    }
}

void LC_RelativePositionEditingWidget::onManualSnapButtonClicked(bool checked) {
    applyInput(false);
}

void LC_RelativePositionEditingWidget::onByOffsetToggled(const bool checked) {
    changeParamVisibility(RS2::RelativePointParam::REL_POINT_X, !checked);
    changeParamVisibility(RS2::RelativePointParam::REL_POINT_Y, !checked);
    changeParamVisibility(RS2::RelativePointParam::REL_POINT_DX, checked);
    changeParamVisibility(RS2::RelativePointParam::REL_POINT_DY, checked);
    switch (m_currentParam) {
        case RS2::REL_POINT_LENGTH:
        case RS2::REL_POINT_ANGLE: {
            break;
        }
        case RS2::REL_POINT_DX: {
            activateParamEditor(RS2::REL_POINT_X);
            break;
        }
        case RS2::REL_POINT_DY: {
            activateParamEditor(RS2::REL_POINT_Y);
            break;
        }
        case RS2::REL_POINT_X: {
            activateParamEditor(RS2::REL_POINT_DX);
            break;
        }
        case RS2::REL_POINT_Y: {
            activateParamEditor(RS2::REL_POINT_DY);
        }
        default:
            break;
    }
    LC_SET_ONE("RelativePositionAssistant", "LastInvocationOffsetMode", checked);
}

bool LC_RelativePositionEditingWidget::tryProcessActivationKeyMnemonic(QKeyEvent* event, const bool activate) {
    if (event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        switch (key) {
            case Qt::Key_D: {
                if (activate) {
                    activateParamEditor(RS2::RelativePointParam::REL_POINT_LENGTH);
                }
                event->accept();
                return true;
            }
            case Qt::Key_A: {
                if (activate) {
                    activateParamEditor(RS2::RelativePointParam::REL_POINT_ANGLE);
                }
                event->accept();
                return true;
            }
            case Qt::Key_X: {
                if (activate) {
                    if (ui->cbByOffset->isChecked()) {
                        activateParamEditor(RS2::RelativePointParam::REL_POINT_DX);
                    }
                    else {
                        activateParamEditor(RS2::RelativePointParam::REL_POINT_X);
                    }
                }
                event->accept();
                return true;
            }
            case Qt::Key_S: {
                if (activate) {
                    if (ui->cbByOffset->isChecked()) {
                        activateParamEditor(RS2::RelativePointParam::REL_POINT_DY);
                    }
                    else {
                        activateParamEditor(RS2::RelativePointParam::REL_POINT_Y);
                    }
                }
                event->accept();
                return true;
            }
            case Qt::Key_T: {
                if (activate) {
                    ui->cbByOffset->toggle();
                }
                event->accept();
                return true;
            }
            case Qt::Key_E: {
                if (activate) {
                    QToolButton* btn = getCurrentParamButton(BTN_PICK);
                    if (btn != nullptr) {
                        btn->click();
                    }
                }
                event->accept();
                return true;
            }
            case Qt::Key_Q: {
                if (activate) {
                    QToolButton* btn = getCurrentParamButton(BTN_MANUAL_SNAP);
                    if (btn != nullptr) {
                        btn->click();
                    }
                }
                event->accept();
                return true;
            }
            case Qt::Key_V: {
                if (activate) {
                    QToolButton* btn = getCurrentParamButton(BTN_OK);
                    if (btn != nullptr) {
                        btn->click();
                    }
                }
                event->accept();
                return true;
            }
            default:
                return false;
        }
    }
    return false;
}

void LC_RelativePositionEditingWidget::updateForPoints(const RS_Vector& wcsPos, const RS_Vector& baseWCSPoint, const bool baseIsRelativePoint) {
    m_relativePositionEvaluator.update(wcsPos, baseWCSPoint);
    updateUIByData(baseIsRelativePoint, m_currentParam);
    LC_GROUP("RelativePositionAssistant");
    {
        bool showInOffsetMode = false;
        const bool rememberMode = LC_GET_BOOL("RememberCoordinatesMode", false);
        if (rememberMode) {
            showInOffsetMode = LC_GET_BOOL("LastInvocationOffsetMode", true);
        }
        else {
            showInOffsetMode = LC_GET_BOOL("StartInOffsetMode", true);
        }
        ui->cbByOffset->setChecked(showInOffsetMode);
    }
    LC_GROUP_END();
}

void LC_RelativePositionEditingWidget::updateUIByData(const bool baseIsRelativePoint, RS2::RelativePointParam currentParam) {
    const LC_RelativePositionData* data = m_relativePositionEvaluator.getRelativePositionData();
    const QString basePoint = m_formatter->formatWCSVector(data->wcsBasePoint);
    ui->lblBasePoint->setText(basePoint);

    m_baseIsRelativePoint = baseIsRelativePoint;

    const QString basePointLabel = baseIsRelativePoint ? tr("Relative zero:"): tr("Base point:");
    ui->lblBasePointType->setText(basePointLabel);

    const double length = data->length;
    const QString lengthStr = m_formatter->formatLinear(length);
    ui->lblValueLength->setText(lengthStr);
    ui->leValueEditLength->setText(lengthStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_LENGTH) {
        m_editingStartValueString = lengthStr;
        ui->leValueEditLength->selectAll();
    }

    const double angle = data->wcsAngle;
    QString angleStr = m_formatter->formatWCSAngleDegrees(angle);
    ui->lblValueAngle->setText(angleStr);
    const QString editAngleStr = angleStr.remove(QChar(0xB0));
    ui->leValueEditAngle->setText(editAngleStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_ANGLE) {
        m_editingStartValueString = editAngleStr;
        ui->leValueEditAngle->selectAll();
    }
    const RS_Vector ucsProjectionPoint = m_viewport->toUCS(data->wcsProjection);
    const RS_Vector ucsBasePoint = m_viewport->toUCS(data->wcsBasePoint);
    const auto ucsDelta = RS_Vector(ucsProjectionPoint - ucsBasePoint);

    const QString dxStr = m_formatter->formatLinear(ucsDelta.x);
    ui->lblValueDX->setText(dxStr);
    ui->leValueEditDX->setText(dxStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_DX) {
        m_editingStartValueString = dxStr;
        ui->leValueEditDX->selectAll();
    }

    const QString dyStr = m_formatter->formatLinear(ucsDelta.y);
    ui->lblValueDY->setText(dyStr);
    ui->leValueEditDY->setText(dyStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_DY) {
        m_editingStartValueString = dyStr;
        ui->leValueEditDY->selectAll();
    }

    const QString projectedPoint = m_formatter->formatWCSVector(data->wcsProjection);
    ui->lblProjected->setText(projectedPoint + (data->isSingleSolution ? "" : "(?)"));

    const QString xStr = m_formatter->formatLinear(ucsProjectionPoint.x);
    ui->lblValueX->setText(xStr);
    ui->leValueEditX->setText(xStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_X) {
        m_editingStartValueString = xStr;
        ui->leValueEditX->selectAll();
    }

    const QString yStr = m_formatter->formatLinear(ucsProjectionPoint.y);
    ui->lblValueY->setText(yStr);
    ui->leValueEditY->setText(yStr);
    if (currentParam == RS2::RelativePointParam::REL_POINT_Y) {
        m_editingStartValueString = yStr;
        ui->leValueEditY->selectAll();
    }

    ui->lblExplicitLength->setText(data->explicitLength ? "!" :"");
    ui->lblExplicitAngle->setText(data->explicitAngle ? "!" :"");
    ui->lblExplicitDX->setText(data->explicitDX ? "!" :"");
    ui->lblExplicitDY->setText(data->explicitDY ? "!" :"");
    ui->lblExplicitX->setText(data->explicitDX ? "!" :"");
    ui->lblExplicitY->setText(data->explicitDY ? "!" :"");
}

void LC_RelativePositionEditingWidget::refreshPreview() {
    const auto currentAction = dynamic_cast<RS_PreviewActionInterface*>(m_actionContext->getCurrentAction());
    if (currentAction != nullptr) {
        currentAction->moveMouseToRefreshPreview(m_relativePositionEvaluator.getRelativePositionData()->wcsProjection);
    }
}

void LC_RelativePositionEditingWidget::activateParamEditor(const RS2::RelativePointParam param, bool forceParam) {
    updateByEditedValue();
    refreshPreview();

    auto stackedList = findChildren<QStackedWidget*>();
    for (QStackedWidget* sw : stackedList) {
        sw->setCurrentIndex(1);
    }

    switch (param) {
        case RS2::REL_POINT_LENGTH: {
            activateParamUI(ui->swLength, ui->leValueEditLength);
            break;
        }
        case RS2::REL_POINT_ANGLE: {
            activateParamUI(ui->swAngle, ui->leValueEditAngle);
            break;
        }
        case RS2::REL_POINT_DX: {
            if (forceParam && !ui->cbByOffset->isChecked()) {
                ui->cbByOffset->setChecked(true);
            }
            activateParamUI(ui->swDX, ui->leValueEditDX);
            break;
        }
        case RS2::REL_POINT_DY: {
            if (forceParam && !ui->cbByOffset->isChecked()) {
                ui->cbByOffset->setChecked(true);
            }
            activateParamUI(ui->swDY, ui->leValueEditDY);
            break;
        }
        case RS2::REL_POINT_X: {
            if (forceParam  && ui->cbByOffset->isChecked()) {
                ui->cbByOffset->setChecked(false);
            }
            activateParamUI(ui->swX, ui->leValueEditX);
            break;
        }
        case RS2::REL_POINT_Y: {
            if (forceParam  && ui->cbByOffset->isChecked()) {
                ui->cbByOffset->setChecked(false);
            }
            activateParamUI(ui->swY, ui->leValueEditY);
            break;
        }
        default:
            break;
    }
    m_currentParam = param;
    updateUIByData(m_baseIsRelativePoint, m_currentParam);
}

void LC_RelativePositionEditingWidget::hideAssistant() {
    const auto holder = dynamic_cast<LC_RelativePointInputWidget*>(parent());
    if (holder != nullptr) {
        holder->hide();
    }
}

bool LC_RelativePositionEditingWidget::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        const auto keyEvent = static_cast<QKeyEvent*>(event);

        RS2::RelativePointParam nextParamType;
        const int key = keyEvent->key();
        if (key == Qt::Key_Up) {
            nextParamType = getPreviousPointParam();
            activateParamEditor(nextParamType);
            return true;
        }
        if (key == Qt::Key_Tab) {
            nextParamType = getNextPointParam();
            activateParamEditor(nextParamType);
            return true;
        }
        if (key == Qt::Key_Down) {
            nextParamType = getNextPointParam();
            activateParamEditor(nextParamType);
            return true;
        }
        if (key == Qt::Key_Escape) {
            hideAssistant();
            return true;
        }
        if (tryProcessActivationKeyMnemonic(keyEvent, true)) {
            return true;
        }
    }
    if (event->type() == QEvent::KeyRelease) {
        const auto keyEvent = static_cast<QKeyEvent*>(event);
        if (tryProcessActivationKeyMnemonic(keyEvent, false)) {
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        const auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const auto lbl = dynamic_cast<QLabel*>(watched);
            if (lbl != nullptr) {
                const QVariant data = lbl->property("_propType");
                if (data.isValid()) {
                    bool ok;
                    int intValue = data.toInt(&ok);
                    if (ok) {
                        const auto paramType = static_cast<RS2::RelativePointParam>(intValue);
                        activateParamEditor(paramType);
                        event->accept();
                        return true;
                    }
                }
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        const auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const auto lbl = dynamic_cast<QLabel*>(watched);
            if (lbl != nullptr) {
                event->accept();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LC_RelativePositionEditingWidget::focusCurrentParam() {
    activateParamEditor(m_currentParam);
}

void LC_RelativePositionEditingWidget::updateByInteractiveInput(const RS2::RelativePointParam paramType, const double value) {
    double valueToSet;
    RS2::RelativePointParam paramTypeToSet;
    switch (paramType) {
        case RS2::REL_POINT_LENGTH: {
            paramTypeToSet = RS2::REL_POINT_LENGTH;
            break;
        }
        case RS2::REL_POINT_ANGLE: {
            paramTypeToSet = RS2::REL_POINT_ANGLE;
            break;
        }
        case RS2::REL_POINT_DX: {
            paramTypeToSet = RS2::REL_POINT_DX;
            break;
        }
        case RS2::REL_POINT_DY: {
            paramTypeToSet = RS2::REL_POINT_DY;
            break;
        }
        case RS2::REL_POINT_X: {
            paramTypeToSet = RS2::REL_POINT_X;
            break;
        }
        case RS2::REL_POINT_Y: {
            paramTypeToSet = RS2::REL_POINT_Y;
            break;
        }
        default: Q_ASSERT_X(false, "LC_RelativePositionEditingWidget::updateByInteractiveInput", "Unexpected type");
            break;
    }
    m_relativePositionEvaluator.setPositionParam(paramTypeToSet, value, true);
    updateUIByData(m_baseIsRelativePoint, paramTypeToSet);
    refreshPreview();
}

bool LC_RelativePositionEditingWidget::toDouble(const QString& strValue, double& res, const double notMeaningful, const bool positiveOnly) {
    bool ok = false;
    const double x = RS_Math::eval(strValue, &ok);
    if (ok) {
        res = LC_LineMath::getMeaningful(x, notMeaningful);
        if (positiveOnly) {
            res = std::abs(res);
        }
    }
    return ok;
}

void LC_RelativePositionEditingWidget::updateByEditedValue() {
    double valueToSet;
    RS2::RelativePointParam paramTypeToSet = m_currentParam;
    bool hasValue = false;
    switch (m_currentParam) {
        case RS2::REL_POINT_LENGTH: {
            const QString txt = ui->leValueEditLength->text();
            if (txt != m_editingStartValueString) {
                if (toDouble(txt, valueToSet, 1.0, true)) {
                    hasValue = true;
                }
            }
            break;
        }
        case RS2::REL_POINT_ANGLE: {
            const QString txt = ui->leValueEditAngle->text();
            if (txt != m_editingStartValueString) {
                double ucsAngleDegrees;
                const bool ok = LC_Convert::parseToToDoubleAngleDegrees(txt, ucsAngleDegrees, 0.0, false);
                if (ok) {
                    hasValue = true;
                    const double ucsAngleRad = RS_Math::deg2rad(ucsAngleDegrees);
                    const double ucsBaseAngle = m_viewport->toAbsUCSAngle(ucsAngleRad);
                    const double wcsAngle = m_viewport->toWorldAngle(ucsBaseAngle);
                    valueToSet = wcsAngle;
                }
            }
            break;
        }
        case RS2::REL_POINT_DX: {
            const QString txt = ui->leValueEditDX->text();
            if (txt != m_editingStartValueString) {
                double ucsDXValue;
                if (toDouble(txt, ucsDXValue, 0.0, false)) {
                    hasValue = true;
                    valueToSet = ucsDXValue;
                }
            }
            paramTypeToSet = RS2::REL_POINT_DX;
            break;
        }
        case RS2::REL_POINT_DY: {
            const QString txt = ui->leValueEditDY->text();
            if (txt != m_editingStartValueString) {
                double ucsDYValue;
                if (toDouble(txt, ucsDYValue, 0.0, false)) {
                    hasValue = true;
                    valueToSet = ucsDYValue;
                }
            }
            paramTypeToSet = RS2::REL_POINT_DY;
            break;
        }
        case RS2::REL_POINT_X: {
            const QString txt = ui->leValueEditX->text();
            if (txt != m_editingStartValueString) {
                double ucsXValue;
                if (toDouble(txt, ucsXValue, 0.0, false)) {
                    hasValue = true;
                    valueToSet = ucsXValue;
                }
            }
            paramTypeToSet = RS2::REL_POINT_X;
            break;
        }
        case RS2::REL_POINT_Y: {
            const QString txt = ui->leValueEditY->text();
            if (txt != m_editingStartValueString) {
                double ucsYValue;
                if (toDouble(txt, ucsYValue, 0.0, false)) {
                    hasValue = true;
                    valueToSet = ucsYValue;
                }
            }
            paramTypeToSet = RS2::REL_POINT_Y;
            break;
        }
        default:
            Q_ASSERT_X(false, "LC_RelativePositionEditingWidget::updateByInteractiveInput", "Unexpected type");
    }
    if (hasValue) {
        m_relativePositionEvaluator.setPositionParam(paramTypeToSet, valueToSet, false);
    }
}

RS2::RelativePointParam LC_RelativePositionEditingWidget::getNextPointParam() const {
    switch (m_currentParam) {
        case RS2::REL_POINT_LENGTH: {
            return RS2::REL_POINT_ANGLE;
        }
        case RS2::REL_POINT_ANGLE: {
            return ui->cbByOffset->isChecked() ? RS2::REL_POINT_DX : RS2::REL_POINT_X;
        }
        case RS2::REL_POINT_DX: {
            return RS2::REL_POINT_DY;
        }
        case RS2::REL_POINT_DY: {
            return RS2::REL_POINT_LENGTH;
        }
        case RS2::REL_POINT_X: {
            return RS2::REL_POINT_Y;
        }
        case RS2::REL_POINT_Y: {
            return RS2::REL_POINT_LENGTH;
        }
        default:
            return RS2::REL_POINT_LENGTH;
    }
}

QToolButton* LC_RelativePositionEditingWidget::getCurrentParamButton(const ButtonType type) const {
    switch (m_currentParam) {
        case RS2::REL_POINT_LENGTH: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectLength;
                }
                case BTN_OK: {
                    return ui->pbOKLength;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapLength;
                }
                default:
                    return nullptr;
            }
        }
        case RS2::REL_POINT_ANGLE: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectAngle;
                }
                case BTN_OK: {
                    return ui->pbOKAngle;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapAngle;
                }
                default:
                    return nullptr;
            }
        }
        case RS2::REL_POINT_DX: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectDX;
                }
                case BTN_OK: {
                    return ui->pbOKDX;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapDX;
                }
                default:
                    return nullptr;
            }
        }
        case RS2::REL_POINT_DY: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectDY;
                }
                case BTN_OK: {
                    return ui->pbOKDY;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapDY;
                }
                default:
                    return nullptr;
            }
        }
        case RS2::REL_POINT_X: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectX;
                }
                case BTN_OK: {
                    return ui->pbOKX;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapX;
                }
                default:
                    return nullptr;
            }
        }
        case RS2::REL_POINT_Y: {
            switch (type) {
                case BTN_PICK: {
                    return ui->tbSelectY;
                }
                case BTN_OK: {
                    return ui->pbOKY;
                }
                case BTN_MANUAL_SNAP: {
                    return ui->tbManualSnapDY;
                }
                default:
                    return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

RS2::RelativePointParam LC_RelativePositionEditingWidget::getPreviousPointParam() const {
    switch (m_currentParam) {
        case RS2::REL_POINT_LENGTH: {
            return ui->cbByOffset->isChecked() ? RS2::REL_POINT_DY : RS2::REL_POINT_Y;
        }
        case RS2::REL_POINT_ANGLE: {
            return RS2::REL_POINT_LENGTH;
        }
        case RS2::REL_POINT_DX: {
            return RS2::REL_POINT_ANGLE;
        }
        case RS2::REL_POINT_DY: {
            return RS2::REL_POINT_DX;
        }
        case RS2::REL_POINT_X: {
            return RS2::REL_POINT_ANGLE;
        }
        case RS2::REL_POINT_Y: {
            return RS2::REL_POINT_X;
        }
        default:
            return RS2::REL_POINT_LENGTH;
    }
}

void LC_RelativePositionEditingWidget::changeParamVisibility(QLabel* icon, QLabel* explicitMark, QLabel* name, QStackedWidget* stacked, const bool visible) const {
    icon->setVisible(visible);
    explicitMark->setVisible(visible);
    name->setVisible(visible);
    stacked->setVisible(visible);
}

void LC_RelativePositionEditingWidget::changeParamVisibility(const RS2::RelativePointParam param, const bool show) const {
    switch (param) {
        case RS2::REL_POINT_LENGTH: {
            changeParamVisibility(ui->lblIconLength, ui->lblExplicitLength, ui->lblNameLength, ui->swLength, show);
            break;
        }
        case RS2::REL_POINT_ANGLE: {
            changeParamVisibility(ui->lblIconAngle, ui->lblExplicitAngle, ui->lblNameAngle, ui->swAngle, show);
            break;
        }
        case RS2::REL_POINT_DX: {
            changeParamVisibility(ui->lblIconDX, ui->lblExplicitDX, ui->lblNameDX, ui->swDX, show);
            break;
        }
        case RS2::REL_POINT_DY: {
            changeParamVisibility(ui->lblIconDY, ui->lblExplicitDY, ui->lblNameDY, ui->swDY, show);
            break;
        }
        case RS2::REL_POINT_X: {
            changeParamVisibility(ui->lblIconX, ui->lblExplicitX, ui->lblNameX, ui->swX, show);
            break;
        }
        case RS2::REL_POINT_Y: {
            changeParamVisibility(ui->lblIconY, ui->lblExplicitY, ui->lblNameY, ui->swY, show);
        }
        default:
            break;
    }
}

// fixme - absolute x and y mode - bugs!
// persistent visual snap - survive between actions!
