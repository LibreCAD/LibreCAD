#ifndef LC_RELATIVE_POSITION_EDITING_WIDGET_H
#define LC_RELATIVE_POSITION_EDITING_WIDGET_H

#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>

#include "lc_actioncontext.h"
#include "lc_relative_position_evaluator.h"
#include "rs.h"

struct LC_RelativePositionData;
class LC_RelativePointInputWidget;
class LC_GraphicViewport;
class LC_Formatter;
class RS_Vector;
class QLineEdit;
class QLabel;

namespace Ui {
    class LC_RelativePositionEditingWidget;
}

class LC_RelativePositionEditingWidget : public QWidget {
    Q_OBJECT public:
    explicit LC_RelativePositionEditingWidget(LC_RelativePointInputWidget* parent, LC_GraphicViewport* viewport,
                                              LC_ActionContext* actionContext, LC_LateCompletionRequestor* lateCompletionRequestor);
    ~LC_RelativePositionEditingWidget() override;
    void updateForPoints(const RS_Vector& wcsPos, const RS_Vector& baseWCSPoint, bool baseIsRelativePoint);
    void activateParamEditor(RS2::RelativePointParam param, bool forceParam = false);
    void hideAssistant() const;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void focusCurrentParam();
    void updateByInteractiveInput(RS2::RelativePointParam paramType, double value);
    bool toDouble(const QString& strValue, double& res, double notMeaningful, bool positiveOnly);

public slots:
    void onEditingReturnPressed();
    void onOKButtonClicked(bool checked);
    void onInteractiveInputButtonClicked(bool checked);
    void onManualSnapButtonClicked(bool checked);
    void onByOffsetToggled(bool checked);
signals:
    void onRelativeInputCompleted(LC_RelativePositionData* data, bool manualPick);

protected:
    enum ButtonType {
        BTN_PICK,
        BTN_OK,
        BTN_MANUAL_SNAP
    };

    bool tryProcessActivationKeyMnemonic(QKeyEvent* event, bool activate);

private:
    RS2::RelativePointParam getNextPointParam() const;
    QToolButton* getCurrentParamButton(ButtonType type) const;
    RS2::RelativePointParam getPreviousPointParam() const;
    void changeParamVisibility(QLabel* icon, QLabel* explicitMark, QLabel* name, QStackedWidget* stacked, bool visible) const;
    void changeParamVisibility(RS2::RelativePointParam param, bool show) const;
    Ui::LC_RelativePositionEditingWidget* ui;
    LC_GraphicViewport* m_viewport{nullptr};
    LC_ActionContext* m_actionContext{nullptr};
    LC_LateCompletionRequestor* m_lateCompletionRequestor{nullptr};
    LC_Formatter* m_formatter{nullptr};
    RS2::RelativePointParam m_currentParam{RS2::REL_POINT_LENGTH};
    LC_RelativePositionEvaluator m_relativePositionEvaluator;
    bool m_baseIsRelativePoint {false};
    QString m_editingStartValueString;
    void setupLabelAndEditor(QLabel* label, QLineEdit* lineEdit, RS2::RelativePointParam relativePointParam);
    void setupButtons(QToolButton* btnOk, QToolButton* btnInteractivePick, QToolButton* btnManualSnap,
                      RS2::RelativePointParam relativePointParam, LC_ActionContext::InteractiveInputInfo::InputType);
    void connectInteractiveInputButton(QToolButton* button, LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                       RS2::RelativePointParam relativePointParam);
    void applyInput(bool cond);
    void updateUIByData(bool baseIsRelativePoint, RS2::RelativePointParam currentParam);
    void refreshPreview();
    void updateByEditedValue();
};

#endif
